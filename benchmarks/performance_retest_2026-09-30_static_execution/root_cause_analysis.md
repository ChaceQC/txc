# 最新性能报告的根因诊断

日期：2026-09-30。基于本目录 `FULL_REPORT.md`，正式测试提交为 `f1cc9a5`，当前归档提交为 `5aa6d02`。本次只新增诊断报告和 `tx_build/` 下的诊断产物，没有修改编译器、运行时、标准库或正式基准。

## 结论

主要问题是静态信息没有完整贯穿优化决策和对象表示，而且新分析层让部分既有快速路径失效。纯标量运算已经接近 C++；报告中的综合墙钟倍率为 1.75，并非所有程序都慢几十倍。

已定位三处明确的优化失效：默认堆的借用和 GC 安全点消除被挡住，快照迭代器的直接游标读取被挡住，锁卫士因不同作用域同名变量而无法使用栈存储。另确认 serde 的新路径覆盖了原有双字段专用编解码器，但其跨轮耗时变化不能全部归因于这一处。

## 1. 无竞争锁：按名字扫描整个函数，导致栈上 guard 优化失效

正式报告：100000 次操作，TX 20.517 ms，C++ 0.4811 ms，42.65 倍。

`concurrency.tx:37` 在循环中声明 `guard`，`:41` 在循环外再次声明 `guard`。这两个局部变量属于不同作用域。

- `src/backend/llvm/codegen_sync_local.cpp:30` 要求 `confined_guard_local` 成立。
- `src/backend/llvm/codegen_local_object_analysis.cpp:253` 从整个当前函数体检查 guard。
- 同文件 `statement_safe` 在 156 行判断：非候选声明必须与候选名称不同。这使用的是名字，未使用 SSA 中已存在的绑定身份。
- 因此两处 guard 相互阻止优化。正式二进制保留 `txrt_sync_lock_i64`；新导出的 IR 也确认没有调用局部 guard 入口。
- 回退后的 `src/backend/cpp/sync_lock_abi.cpp:85` 每次通过 `make_shared` 创建 guard，再包装为拥有句柄；get/set/close 分别经过 any 解包和状态检查，退出时再销毁。

定向对照保持 100000 次 lock/get/set/close，只把循环外变量改为 `final_guard`。同一个诊断程序中，两组分别使用独立函数，校验结果均为 100000。

| 变体 | 三次内部耗时 ms | 中位数 ms |
| --- | --- | --- |
| 重用 guard 名字 | 20.639 / 19.745 / 20.609 | 20.609 |
| 循环外使用 final_guard | 1.447 / 1.258 / 1.276 | 1.276 |

改名组生成 `txrt_sync_local_lock_i64`，约快 16.15 倍。这是编译器优化适用性缺陷；修复应使用绑定身份和作用域，不应要求用户规避合法的局部变量同名。

## 2. 默认 heap：被保守标为 captured，恢复大量临时句柄和安全点

正式报告：7.925 ms 上升至 29.891 ms，比上一轮增加 277.2%；对 C++ 为 9.87 倍。校准和专项也出现相同方向的大幅退化。

调用链：

1. `src/frontend/ast/call_properties.cpp:42` 为支持自定义比较器，将 heap 排除在普通标量容器效果之外；其调用效果默认保守。
2. `src/backend/analysis/call_analysis.cpp:55` 将 `saves_arguments` 传播为输入对象的 captured 状态。诊断分析 JSON 中，`bench_heap` 的堆分配根 0 确实进入 `captured_roots`。
3. `src/backend/llvm/codegen_local_object_analysis.cpp:222` 新增的 `current_analysis_->confined` 前置检查拒绝该局部。
4. `default_heap_local` 同文件 286 行也依赖这个检查，即使构造明确没有自定义比较器，仍无法标记为默认堆。
5. `codegen_container.cpp:156` 的接收者借用及 `codegen_gc.cpp:209` 的默认堆安全点省略同时失效。

当前 IR 中，push、top、pop 都是 `txrt_value_clone -> heap 操作 -> txrt_value_release -> txrt_gc_safepoint_context`。50000 次 push、50000 次 top、50000 次 pop，共增加 150000 对句柄复制/释放，以及这些操作后的安全点检查。安全点检查不等于每次都执行完整 GC，但句柄分配、根登记和引用计数成本仍然真实存在。

修复方向：在效果分析中保留“此实例为默认比较器堆”的事实，并区分接收者修改、参数保存和真正逃逸，恢复已证明安全的借用和安全点消除。

## 3. snapshot iterator：内建调用分类不完整，直接游标读取回退

正式报告：0.165 ms 上升至 1.468 ms，比上一轮增加 789.7%；对 C++ 为 31.91 倍。

- `src/backend/analysis/ir_expression.cpp:66` 的内建容器识别只覆盖 vector 和 typed_container；`src/common/common.hpp:154` 的 typed_container 不包含 iterator、option。
- 这些方法没有普通函数 target 时，iterator.next/close、option.value 会进入 unknown-target 的保守处理。
- `call_analysis.cpp` 在未知调用处传播保存、修改及返回别名；诊断分析中 `cursor` 的根 29 和 `item` 的根 40 均被 captured，cursor 还与输入 vector 根混入同一保守别名集合。
- `src/backend/llvm/codegen_iterator.cpp:81` 的 `cache_iterator_cursor` 依赖 `confined_local`，因此不再缓存游标。
- 当前 IR 与正式二进制均使用 `txrt_iterator_next_scalar_i64`，而非直接从 `%tx_iterator_cursor` 读取。100000 次 next 都经过 `iterator_abi.cpp` 的 iterator/vector any 解包、状态检查及受检运行时边界。

标量 option 仍有直接表示，不能将本项描述为“每次 next 都重新堆分配 option”。真实退化点是丢失了直接游标读取。

## 4. ThinLTO 并非上述两项退化的主要原因

使用同一当前工具链重新编译原始 features.tx 的 `--no-lto` 变体，与本轮正式 LTO 二进制交替执行。一个预热轮、三个记录轮，核对校验值。

| 项目 | LTO 三次 ms | 无 LTO 三次 ms | 校验值 |
| --- | --- | --- | --- |
| heap_push_pop | 30.256 / 31.378 / 29.290 | 33.578 / 32.477 / 31.768 | 24975000 |
| iterator_snapshot | 1.462 / 1.465 / 1.452 | 1.438 / 1.443 / 1.419 | 49950000 |

关闭 LTO 没有恢复原有性能。该对照同时切换 bitcode/原生静态库及其 C++ 编译方式，并非孤立的 LTO 算法实验；但足以排除“仅关闭 LTO 就能解决这两项”的判断。源码分析和生成 IR 则独立定位了快速路径失效机制。

## 5. serde：字段级生成替代了更强的双字段专用路径

`payload` 是 int+str 两字段对象。`src/backend/llvm/codegen_serde.cpp:175` 起在 `direct` 成立时，把 schema 的 `specialized` 指针设为 null，同时填写生成的 write/read 字段函数。

这覆盖了原来的 `tx_serde_pair_i64_str`。当前诊断 IR 的 schema 常量确认是 `ptr null, ptr @.serde_schema.0.write, ptr @.serde_schema.0.read`。

`src/stdlib/serde_pair.cpp` 原本用两个字段的直接比较、位标记和类型化读写处理整段结构。被覆盖后，`serde_struct_reader.cpp` 仍执行通用字段查找、seen 数组、版本值装入 any、收尾检查，再调生成函数；生成函数仅负责选定字段后的存储和类型化解码，并未替代整个解析控制流。

这是确定的路径选择问题，但当前证据不能断言它独自造成主测 +41.1%：同轮 GCC 配对组的 TX 短文本反而变化 -1.6%，clang 配对组 +13.9%。需保留更强的既有专用路径，或让新生成器覆盖整段 schema 解析，再做同条件对照。

## 6. 长期成本：对象表示、短生命周期物化和完整语义

| 报告项目 | 已确认的实现成本 | 解释边界 |
| --- | --- | --- |
| deinit 21.75 倍 | `class_abi.cpp:217` 创建共享类对象、固定槽位、拥有句柄；tracked 的 array 字段使对象进入 GC 登记；释放经引用计数、析构回调和错误状态保护 | 此项是 20000 次完整构造/销毁，不能当成单独一次 deinit 调用耗时 |
| deep_copy 10.71 倍、cycle_gc 21.09 倍 | `deep_copy.cpp` 保留动态 any 种类判断、身份映射、共享/循环图复制；`cycle_gc.cpp` 保留图扫描及析构复活处理 | 简化 C++ 参考不覆盖全部语义；同类通用图复制契约为 3.17 倍，更适合衡量剩余实现差距 |
| format 及 encoding | 即使只消费 len，仍创建输出内容及拥有句柄；`format_static_abi.cpp` 最终发布文本根，`encoding_memory_abi.cpp:36` 校验 UTF-8、复制为 byte_storage、发布 any 句柄 | 静态格式模板已预解析，已知编码也进入专用入口；不能再把主要原因说成每次解析模板/编码名称 |
| property/parameterized | `checked_callback.hpp` 虽缓存目标和 context，但每次仍保存/恢复错误传播标记，经函数指针调用并检查错误状态 | 参考回调未复制完整 TX 测试失败上下文；没有采样证据证明具体成本占比 |

共同问题是小操作的有效计算很少，对象分配、拥有关系和受检边界占比很高。已有静态字段布局和直接调用并不意味着所有对象都已获得接近原生 C++ 的存储和生命周期。

## 7. 后端策略项与尚未查实的部分

- TLS 的 5.69 倍不是同一加密后端的纯编译器对照。TX 使用 mbedTLS，并在 `tls_stream_handshake.cpp:125` 起为每条连接初始化 RNG/config/SSL、解析身份并验证证书；C++ 使用 OpenSSL。可以确认额外工作和后端差异，不能把整个倍率归因于某一步或断言 mbedTLS 算法本身慢 5.69 倍。
- SQLite pool 的 TX 和 C++ 参考都采用归还时关闭、下次重开的策略；`db_pool_return.cpp:8` 确认 TX 没有复用 SQLite native connection。因此该策略解释了连接成本，却不能独自解释 TX/C++ 的 3.12 倍。池包装、驱动配置等差异需要单独拆分。
- dictionary_int_hit、外部题各热点、随机数、字符串转换等尚未逐项做受控成本分解，不为它们编造已证实的占比或统一根因。
- 亚毫秒项目、网络和跨轮机器状态存在扰动；本报告不把所有跨轮变化直接当作代码因果。

## 建议处理顺序

1. 修复 guard 按名字判断作用域的问题。
2. 完善内建容器效果与返回别名规则，恢复默认 heap 和 snapshot iterator 快速路径；保留自定义比较器、真实逃逸和异常路径的保守处理。
3. 调整 serde 新旧专用路径的选择，避免弱特化覆盖强特化。
4. 继续减少已证明局部的对象/文本/bytes 物化、句柄和错误边界重复工作；保留动态值检查及原有失败语义。
5. 后端、外部题及其余差距另按具体负载剖析，不以替换库或关闭 LTO 代替根因修复。

## 本次验证与产物

只执行了针对 features 和 guard 的小范围对照，没有重跑全量性能套件。工作区源码未改。

- `tx_build/performance_root_cause_features.ll`、`.analysis.json`：默认堆及快照迭代器的当前 IR 和分析结果。
- `tx_build/performance_root_cause_features_nolto.exe`：同源无 LTO 对照。
- `tx_build/performance_root_cause_concurrency.ll`：正式锁基准的当前 IR。
- `tx_build/performance_root_cause_guard.tx`、`.ll`、`.exe`：仅改变局部名称的因果对照。
- `tx_build/performance_root_cause_diverse.ll`：当前 format/encoding/serde 路径证据。
