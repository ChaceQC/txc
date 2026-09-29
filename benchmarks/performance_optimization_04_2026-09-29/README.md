# 性能优化 04：普通值句柄去除引用锁

日期：2026-09-29。04.1～04.4 已实施，结构、定向行为和包兼容检查通过。格式化负载尚未显示稳定的整体提速。

## 范围与构建身份

实施时 HEAD 为 `e5068a35ac660647cf789971e7bd3c8ce84a3dd2`，工作树已有尚未提交的 02、03 改动。本项将 `src/backend/cpp/runtime_abi_internal.hpp` 的记录拆为 `value_handle_record` 和 `text_handle_record`；`runtime_abi.cpp`、`text_reference.cpp` 改用对应类型，`docs/send_sync.md` 明确同步归属。新增 `tests/stdlib/handle_lifecycle.cpp` 覆盖根链表、退出清理、错误保留和跨线程文本最后释放。

完整执行 `scripts/build.ps1`，编译器、静态库和三个预编译 TX 桥接对象均已更新；成功后临时 `build/` 已清理。`tx/package.compat` 的 ABI 指纹为 `f2dd26d847ca38364a699f518820052060011a07efd038bcf7a27ae3565e5501`。

| 产物 | SHA-256 |
| --- | --- |
| 03 对照 `tx/txc.exe`（构建前） | `cc545644d6ccce4607ec4b2f84225e05db4431159a085b53a03610eab9d49770` |
| 04 `tx/txc.exe` | `1c3bbed2e3b5447b6af006181ca471715f406f90998ecc154763dfc5b4eca877` |
| 03 对照 `tx/libtxstdlib.a`（构建前） | `8f5c580da075a7fb6ffc9fe079d0f84486544a8996e95801df9008be96def6d4` |
| 04 `tx/libtxstdlib.a` | `fe2f270668df342bb3fc9b54eeea6312f150cc6d6a9e43dd1599ff44eb2db43c` |

## 结构与行为

普通 `std::any` 根记录只保留共同链表节点和载荷，不再含引用计数及 `std::mutex`。文本记录继续持有根引用数、内部引用数及互斥量，跨线程内部引用的最后释放仍按原同步规则执行。`make_handle` 根据静态类型选择记录；`destroy_handle`、`cleanup_live_handles` 和 `text_reference` 均通过 C++ 基类 `static_cast` 还原记录，不按旧偏移重解释。根链表仍由所属线程维护；`mutex`、`channel`、`cancel_token` 等载荷的自身同步没有改动。普通值的最后释放与退出清理保留 `error_cleanup_guard`，用户析构覆盖不了已有错误。

以同一份 `benchmarks/diverse_performance.tx` 编译的 03/04 程序检查优化后机器码：`txrt_value_box_i64` 的记录分配从 64 字节变为 40 字节，03 的创建路径调用 `pthread_mutex_init`，04 不再调用；`destroy_handle<std::any>` 的 03 路径调用 `pthread_mutex_destroy`，04 不再调用。04 的释放路径仍调用 `unregister_handle`、`error_cleanup_guard` 和 `std::any` 析构。文本句柄继续保留互斥量。

## 定向验证

| 检查 | 结果与覆盖 |
| --- | --- |
| `scripts/build.ps1` | 整体构建成功，三个 TX 桥接对象重新生成 |
| `tests/stdlib/handle_lifecycle.cpp` 原生编译运行 | 整数装箱及根链表、普通值析构和退出清理时保留原始错误；短、长文本在根释放/退出清理后由另一线程最后释放 |
| `scripts/check_concurrency_errors.py` | TX 与原生回调中的线程/任务错误、后续正常任务通过 |
| `tests/stdlib/send_sync_move.tx`、`thread_lifecycle.tx`、`concurrency_results.tx` | 跨线程文本与移动、分离线程生命周期、线程/任务复合结果通过 |
| `tests/containers/behavior.tx` | 容器返回、装箱、字段和深拷贝通过 |
| `scripts/check_package_compatibility.py` | 当前包及四种错配场景通过 |

原生用例使用 `g++ -std=c++23 -O2 -pthread -Isrc tests/stdlib/handle_lifecycle.cpp tx/libtxstdlib.a -Ltx/link -lwinhttp -lws2_32 -ldnsapi -ladvapi32 -lbcrypt -lcrypt32 -lncrypt -lshell32 -luser32 -liconv -o tx_build/handle_lifecycle_04.exe` 编译，随后运行该程序。机器码检查还确认 `txrt_value_none`、`txrt_value_box_bool` 和 `txrt_value_box_f64` 均无根记录的互斥量调用。

## 同轮性能

03 和 04 程序均由未变的 `benchmarks/diverse_performance.tx` 编译，源码 SHA-256 为 `2e940bff73a793788cf78c37c6a34c98cab38fb1c1aaacc184042cf89f13b499`。各预热一次，再按 03/04、04/03 交替测 5 轮；每轮校验值均为 `488890`。下表为程序内部计时中位数，原始顺序、样本与程序哈希见 [samples.json](samples.json)。

| 项目 | 03 | 04 | 03/04 |
| --- | ---: | ---: | ---: |
| format_literal | 13.163 ms | 13.751 ms | 0.957 |
| format_dynamic | 61.535 ms | 61.311 ms | 1.004 |

这两个完整格式化负载包含大量文本和格式化工作，样本也有明显波动；本轮不能据此声称整体提速。去锁与记录缩小由目标机器码直接证实，句柄相关行为由定向检查覆盖。本项未测内存峰值，也未运行外部 mini-filesystem 负载。02 中 map 性能回退仍按其原记录跟踪。
