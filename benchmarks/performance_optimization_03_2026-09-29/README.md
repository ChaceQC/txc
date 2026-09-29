# 性能优化 03：调用效果、借用与 GC 判定

日期：2026-09-29。03.1～03.4 已实施，定向结构、行为和同会话性能检查通过。02 的 map 对照波动仍按 02 记录单独跟踪。

## 范围与构建身份

实施时 HEAD 为 `e5068a35ac660647cf789971e7bd3c8ce84a3dd2`，工作树同时包含尚未提交的 02 改动。03 修改了 `src/common/call_properties.hpp`、`src/frontend/ast/call_properties.cpp`、`src/frontend/sema/sema_call_properties.cpp` 和 `src/backend/llvm/codegen_gc.cpp`；新增 `tests/containers/call_effects.tx`、`scripts/check_call_effects.py` 与本目录的测量脚本、原始样本。计划状态同步于 `docs/performance_optimization_plan.md`。`codegen_statement.cpp` 和 `codegen_call_borrow.cpp` 继续使用统一属性，无需改动。

完整执行 `scripts/build.ps1` 后，编译器、标准库和三个预编译 TX 桥接对象均已更新，成功构建后临时 `build/` 已清理。产物标识：

| 项目 | SHA-256 |
| --- | --- |
| `tx/txc.exe` | `cc545644d6ccce4607ec4b2f84225e05db4431159a085b53a03610eab9d49770` |
| `tx/libtxstdlib.a` | `8f5c580da075a7fb6ffc9fe079d0f84486544a8996e95801df9008be96def6d4` |
| 02 对照 `tx_build/call_borrowing_02.exe` | `41530dc71586c5219aca7a81862ae3b985a5a2c1c711ed794f2fbda7d340b61b` |
| 03 程序 `tx_build/call_borrowing_03.exe` | `a82ddb50225567a83ac982b57346d67d8240ed2f7b7723865a4008570124f61d` |

`tx/package.compat` 中的兼容指纹为 `d7d3974415c9ac9e34f2d34a91d52e33846e9196760c5861bd19c62a671c0eec`。03 未修改运行时 C ABI、对象布局或公开 `.txh` 签名；重新构建仍使包内产物和兼容清单配套。

## 结构与语义

调用属性现在由完成类型和重载绑定后的语义标注提供，分别描述普通分配、GC 节点登记、用户代码、用户对象释放、参数修改和保存。未知入口仍为保守值。借用与安全点读取同一调用属性；对普通 TX 函数，后端继续递归分析函数体，并在具名/展开绑定、异步调用、隐式参数转换、复杂接收者等情形保守保留安全点。

`dictionary.contains`、标量 map/set 查询、标量 queue/deque 的 `front/back/pop` 和 `cancel.status` 可在整条语句无其他危险效果时省略轮询。标量容器构造和可能扩容的修改仍按分配处理；文本结果复制句柄也标为分配。对象键比较、对象元素 `pop`、返回值清理、旧引用覆盖以及可能重绑定接收者的实参仍走完整路径。查询中的越界、非法键、取消状态和必要同步没有被移除，也未将查询标成可常量折叠的纯函数。

同一基准源码生成的 IR 中，各函数静态安全点调用数如下；这些计数包含计时区外的初始化和输出，不能直接当作循环每次执行数。

| 函数 | 02 IR | 03 IR | 说明 |
| --- | ---: | ---: | --- |
| `bench_dictionary` | 4 | 3 | `contains` 后轮询消除 |
| `bench_set` | 5 | 4 | 标量 `contains` 后轮询消除 |
| `bench_queue` | 6 | 4 | 标量 `front/pop` 后轮询消除，`push` 仍轮询 |
| `bench_cancel` | 5 | 4 | `status` 后轮询消除 |
| `bench_map` | 3 | 4 | 新安全点在计时前的 `values[7] = 3`；索引读取循环仍无安全点 |

定向 IR 用例的安全查询函数 `safe_queries` 与标量 `safe_pop` 中均无 `txrt_gc_safepoint_context`、接收者 `txrt_value_clone` 或标量键 `txrt_value_box_*`；对象键 `contains`、对象 `pop` 和返回文本的 `front` 均保留安全点。

## 定向验证

| 命令 | 结果 |
| --- | --- |
| `python -X utf8 -B scripts/check_call_effects.py` | 安全查询/标量 pop 的 IR，回调/析构/文本结果的 IR，以及行为和析构报错通过 |
| `python -X utf8 -B scripts/check_call_borrowing.py` | 重绑定、别名、混合键、NaN、正负零、叶子错误、取消、回调和析构；查询无克隆/装箱通过 |
| `python -X utf8 -B scripts/check_package_compatibility.py` | 当前包和四种错配场景通过 |

## 同会话性能

负载为未修改的 `benchmarks/call_borrowing.tx`，SHA-256 `6e77763d77f435006016519a717723d14337336312d4589a8836345d9882bfa8`。02 与 03 程序各预热一次，按 02/03、03/02 交替测 5 轮；每轮核对相同校验值。表中为程序内部微秒计时换算的中位数，单位 ms。逐轮顺序、原始数值、校验值、时间与程序哈希见 [samples.json](samples.json)，可用 `python -X utf8 -B benchmarks/performance_optimization_03_2026-09-29/measure.py` 重测。

| 项目 | 02 | 03 | 02/03 |
| --- | ---: | ---: | ---: |
| dictionary | 4.986 | 3.536 | 1.41 |
| set | 4.154 | 2.857 | 1.45 |
| queue | 1.804 | 1.593 | 1.13 |
| cancel | 6.133 | 4.920 | 1.25 |
| map | 3.800 | 3.532 | 1.08 |

map 的计时循环没有结构变化，因此这组较小的改善不能归因于 03，也不关闭 02 中 map 回退的未决问题。本专项没有相同语义的 C++ 成对程序，不计算等价 C++ 倍率；本轮也未测内存峰值或可比较的编译耗时。构建前后没有增加运行时字段、C ABI 参数或外部依赖。下一项 04 可独立推进，依赖 03 的后续优化可以使用本轮统一属性，未知入口仍须逐项确认。
