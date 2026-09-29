# 性能问题定位与分项修改方案

日期：2026-09-29。

状态：原始分析与方案记录时尚未实施任何项。2026-09-29 已完成第 01 项的基线归档和定向校准，验收见[记录](../benchmarks/performance_baseline_2026-09-29/README.md)；02.1～02.4 已实施，02.5 评估后暂不分区，但 map 对照存在回退，整项性能验收尚未通过，见[02 记录](../benchmarks/performance_optimization_02_2026-09-29/README.md)；03 已实施并完成定向验收，见[03 记录](../benchmarks/performance_optimization_03_2026-09-29/README.md)；04 已实施并完成定向结构与行为验收，格式化负载尚无稳定整体收益，见[04 记录](../benchmarks/performance_optimization_04_2026-09-29/README.md)；05 已实施并完成定向验收，见[05 记录](../benchmarks/performance_optimization_05_2026-09-29/README.md)；06 已实施并完成定向验收，见[06 记录](../benchmarks/performance_optimization_06_2026-09-29/README.md)；07 已实施并完成定向验收，动态模板整体收益较小，见[07 记录](../benchmarks/performance_optimization_07_2026-09-29/README.md)；08 已实施并完成定向验收，见[08 记录](../benchmarks/performance_optimization_08_2026-09-29/README.md)；09～16 仍待实施。01 没有修改编译器、运行时、标准库或发布工具链产物。

## 1. 目标、依据与适用范围

本计划针对最新性能复测中 TX 相比 C++ 超过 5 倍的热点，以及 9 月 27 日至 29 日出现的性能回退。目标分为两层：先消除新增的公共开销，再让编译期已知的类型、布局和调用信息真正进入生成代码。

依据：

- [2026-09-29 全量性能复测](../benchmarks/performance_retest_2026-09-29.md)。
- [模块、网络、标准库组合及外部题原始样本](../tx_build/performance_retest_20260929.json)。
- [综合负载原始样本](../tx_build/diverse_performance_results_20260929.json)。
- [语言特性基准及比较边界](../benchmarks/language_features/README.md)。
- [既有运行时优化记录](runtime_optimization.md)及其中的 9 月 27 日验收报告。
- 当前工作树源码、Git 历史，以及本地保留的新旧程序反汇编。

本次分析时，HEAD 为 `bb97baa`，工作树包含未提交改动。性能报告中的产物哈希与分析时本地产物一致：

| 产物 | SHA-256 |
| --- | --- |
| `tx/txc.exe` | `58ff0459fcee33b82a2a887f73109c3a71143cd10135b6ac23edb32161c771d3` |
| `tx/libtxstdlib.a` | `ec5f0804addcca0997178d951ab0e6e5a108fc12481a38bf4994eca6bb3519c1` |

上述哈希用于标识本轮候选产物，不能仅用 HEAD 复原该工作树。后续实施从当时实际工作区继续，保留已有改动，并重新记录基线。原始 JSON、旧程序和审计脚本位于被 Git 忽略的 `tx_build/`，克隆仓库不会自动取得这些文件；本计划第 01 项负责补齐可复现证据。

### 1.1 证据等级

- **已确认结构问题**：可以从源码及现有机器码确认，多余操作确实存在。
- **有同轮数据支持的回退归因**：新旧程序在同一轮交替测量，变化与代码路径一致；仍不等于逐项消融后得到的独立耗时。
- **待验证收益**：具体实现方案及预期消除的成本，尚未实现和测量，不能写成已获得的提速。

本文不将静态分析换算为 CPU 占比，不承诺修改后必然达到某个倍数。后续每项均须把结构变化、正确性和实测收益分别记录。

### 1.2 已有优化应继续复用

以下能力已经存在，应在其基础上继续完善：

1. TX 普通函数隐式传递运行时上下文，诊断帧在栈上连接。
2. 基础容器调用借用、动态字典标量键专用 ABI。
3. 局部标量数组、稳定向量存储头和满足证明条件的 foreach。
4. 局部标量 option 的栈表示。
5. 静态 serde schema、基础类型字面量 format 计划、常量编码名选路。
6. 均值专用补偿累计、UTF-8 直接校验和部分库内缓冲复用。

## 2. 性能现状与原因归类

### 2.1 最新数据

22 项语言特性中有 13 项 TX/C++ 超过 5；18 项综合负载中也有 13 项超过 5。纯整数控制流、部分向量索引、数学计算和文件 I/O 已经接近 C++。应优先处理具体热点，不能据此认定所有模块都有同等程度的问题。

下表倍率由本轮对应中位数相除得到；不同套件之间不能横向相加比较。带有语义差异的项目须结合第 2.3 节解读。

| 来源与项目 | TX/C++ | 主要待处理成本 |
| --- | ---: | --- |
| 模块审计：dictionary_contains / set_contains | 19.48 / 22.83 | 查询后的 GC 安全点、TLS 与全局锁 |
| 模块审计：queue_push_pop / vector_push | 43.54 / 11.24 | 逐操作安全点、ABI 边界及实际扩容 |
| 模块审计：heap_push_pop | 14.19 | 保守持有、比较器通用路径、调整路径临时分配 |
| 模块审计：function_value / closure_bind | 36.69 / 65.82 | 目标恢复、回调包装、上下文查询和安全点 |
| 模块审计：iterator_snapshot | 35.98 | 逐元素 ABI、状态和错误检查 |
| 综合负载：parse_valid / parse_invalid | 206.66 / 228.89 | 字符串复制、完整结果及错误对象构造、GC |
| 综合负载：format_literal / format_dynamic | 21.41 / 62.63 | 临时字符串、分段 ABI、动态参数和模板处理 |
| 综合负载：serde_short_text / serde_long_text | 87.71 / 44.37 | 中间动态树与对象创建；C++ 对照语义较少 |
| 综合负载：encoding_literal / encoding_dynamic | 6.86 / 8.52 | 对象和缓冲构造、调用外围成本 |
| 语言特性：struct_operators / class_methods | 9.98 / 10.92 | 动态字段布局、字段 ABI 和句柄 |
| 语言特性：module_call / runtime_cast | 9.26 / 8.59 | 结构体创建与访问、动态类型恢复 |
| 语言特性：deep_copy / deinit / cycle_gc | 19.54 / 20.22 / 21.53 | 通用对象图、分配、析构；部分 C++ 对照不等价 |
| 语言特性：recursion / variadic_unpack | 15.19 / 5.41 | 溢出与诊断约束、动态展开处理 |

18 项综合负载的 TX 各项中位数合计为 428.922 ms，其中解析为 168.189 ms、格式化为 80.314 ms、serde 为 57.804 ms，三组占合计的 71.4%。这是基准内部各项中位数的汇总，不是进程墙钟占比，也不是采样剖析结果。

### 2.2 近期公共回退的证据

9 月 28 日提交 `09d18f5` 引入并发支持后：

1. GC 登记从上下文内的数据结构改为带互斥量的全局表；安全点在检查分配阈值前读取全局表。
2. 普通 `std::any` 句柄与文本句柄共同携带 `reference_mutex`。
3. `txrt_str_clone` 从引用计数递增改为创建新句柄并复制文本。

本地反汇编确认：

- 新程序的 `gc_safepoint()` 正常路径含线程上下文、并发深度两次 emulated TLS 查询，以及登记表的 `pthread_mutex_lock/unlock`；旧版本该函数没有登记表锁。外层错误包装的上下文查询另计。
- dictionary/set 查询循环、queue 操作循环和 cancel 查询循环中有安全点；专项 map 标量索引读取循环中没有该安全点。
- 新程序的 `txrt_value_box_i64` 调用 `pthread_mutex_init`，对应 any 句柄销毁调用 `pthread_mutex_destroy`；旧程序装箱没有这项互斥量初始化。

复核这些结论使用的本地产物为 `tx_build/call_borrowing_committed.exe`、`tx_build/call_borrowing_20260929.exe`、`tx_build/language_features_tx.exe`、`tx_build/diverse_performance.exe` 与 `tx_build/perf_audit_20260927/features_retest.exe`。重点符号包括 `tx_fn_m0_bench_dictionary_0`、`tx_fn_m0_bench_map_0`、`txrt_value_box_i64`、`gc_safepoint()` 和 `tx_fn_m0_bench_parse_paths_0`；复核方式为对现有文件执行 nm/objdump 只读检查，不执行基准程序。

报告里的同轮旧/新数据按操作次数折算如下：

| 专项 | 次数 | 旧程序 ms | 新程序 ms | 新增 ns/操作 |
| --- | ---: | ---: | ---: | ---: |
| dictionary.contains | 1,000,000 | 12.985 | 38.238 | 25.25 |
| set.contains | 1,000,000 | 13.645 | 38.161 | 24.52 |
| queue push/front/pop 总计 | 300,000 | 4.669 | 12.504 | 26.12 |
| cancel.status | 1,000,000 | 15.484 | 40.931 | 25.45 |

四项均增加约 25 ns/操作，且 map 索引专项没有回退，支持优先修复公共安全点路径。该折算不表示每种业务操作完全等价，也不能把全部差值单独归给 mutex；TLS、包装调用和其余实现变化仍须逐项隔离。

### 2.3 C++ 对照需要修正的边界

1. [综合负载 C++](../benchmarks/diverse_performance.cpp) 的 `fixed_json_roundtrip` 只拼接并搜索固定 JSON 形状，不执行完整 schema、重复字段、未知字段和通用嵌套处理。
2. 综合负载的 C++ 格式化、编码函数只覆盖固定场景；应保留它们作为特定算法参考，另加等价契约对照。
3. 解析对照使用 `from_chars` 并读取成功标记；TX 还处理完整结果结构、前后空白、正号、错误信息等规则。需要分别测“纯解析核心”和“公开接口”。
4. statistics 的 C++ 均值采用普通 `accumulate`；TX 使用扩展精度补偿累计与有限值、范围检查。
5. 深拷贝 C++ 实现知道固定对象形状，循环回收对照只识别单节点自环；不能据此设定通用对象图算法的硬性倍率。
6. 部分 C++ 容器采用 32 位 int，而 TX int 为 64 位。当前 C++ 使用 GCC Release，TX 程序使用 clang -O3；同编译器、同宽度对照应单列，历史结果仍保留。
7. 不能通过减少 TX 必须执行的错误检查、精度保证、稳定顺序或 GC 语义来迎合倍率。

## 3. 实施约束与顺序

### 3.1 通用约束

- 保持公开语法、接口签名、求值顺序、共享引用、错误类别及析构语义。确需调整语言规则时，先单独修改语言文档与示例，再实施。
- 普通结果结构字段仍可按原规则访问和修改；局部优化必须在别名、地址暴露和逃逸边界物化。
- 不将“可借用”“不会触发 GC”“不会失败”“无副作用”混为同一属性。cancel.status 等查询仍能观察动态状态。
- 不删除并发安全所需的同步。GC owner 过滤、执行门、线程本地根登记、跨线程发布和最后释放必须保持正确。
- 不整体关闭 GC、溢出检查或诊断栈，不默认启用 fast-math，不改变随机数序列。
- 新实现按语义分析、LLVM 生成、运行时和标准库职责拆分；不把全部逻辑加入一个通用源文件。
- C ABI、布局或类型描述改变时，检查兼容指纹覆盖范围，整体重建编译器、静态库和三个预编译 TX 桥接对象，再核对包兼容性。
- 下列“验收”按各项实际实施时间执行并记录；01～08 的实际状态和限制见各项记录，09～16 尚未执行。

### 3.2 本计划的阶段

这里的阶段编号只用于本计划，不覆盖旧文档中的阶段完成记录。

| 阶段 | 修改项 | 前置条件 | 退出条件 |
| --- | --- | --- | --- |
| 0：固定基线 | 01 | 无 | 现有证据可复现，语义等价与参考对照分开 |
| 1：消除公共回退 | 02、03、04 | 01 | 普通热路径无不必要登记表锁和 any 句柄互斥量；相关错误与线程行为保持 |
| 2：解析与文本热点 | 05、06、07、08 | 02～04；06 和 08 可先做局部专用表示 | 解析、格式化、serde 的主要中间分配得到消除，收益分别记录 |
| 3：静态表示和调用 | 09、10、11、12、13 | 对应调用属性与所有权契约稳定 | 静态字段、闭包、堆和迭代器只保留必要动态工作 |
| 4：对象图与其余热点 | 14、15、16 | 对应前置表示已稳定；热点复测仍有问题 | 每项有独立结论，未达标项和原因明确记录 |

以依赖决定先后，不把所有 ABI 改动合并成一次大重构。每个编号可独立形成可审查的小改动；影响同一 ABI 的连续小点可以一次整体构建验收。

## 4. 逐项修改方案

### 01. 固定证据与等价基线

涉及文件：[性能报告](../benchmarks/performance_retest_2026-09-29.md)、[语言特性](../benchmarks/language_features/README.md)、[综合负载脚本](../scripts/run_diverse_performance.py)、[综合负载 TX](../benchmarks/diverse_performance.tx)、[综合负载 C++](../benchmarks/diverse_performance.cpp)。

- [x] **01.1 保存本轮基线。** 记录 HEAD、工作树差异、工具链及基准源码哈希、CPU/系统、编译参数、运行顺序、校验值、原始样本。旧可执行文件与其 DLL 作为同一套产物保留，避免新 DLL 覆盖后把旧程序当作原基线。
- [x] **01.2 将必要审计材料纳入可复现目录。** 从本地 tx_build 选择本计划实际使用的审计源码和采样脚本，审查后放入 benchmarks 对应目录；同时保留精简、可追溯的原始样本。不得直接提交整个临时构建目录。归档应逐文件核对哈希，不静默改写负载。
- [x] **01.3 补齐等价对照。** C++ 容器使用 int64_t；解析分别测核心函数、完整结果构造；serde 使用完整 schema 与校验；statistics 使用相同补偿策略；堆保留相同稳定性规则。历史简化对照单独标注。
- [x] **01.4 固定测量口径。** 同源码、同输入、同校验值，新旧程序在同一会话交替测量。正式热点通常沿用 1 次预热、5～7 次测量；短项只在信噪比不足时等量放大两边循环，并保留原负载结果。CPU 状态或样本分布异常时先记录原因，不自动反复跑全套。
- [x] **01.5 制定达标规则。** 首批简单查询和静态调用以等价 C++ 对照低于 5 倍为阶段目标；恢复旧性能只作为回退修复证据。语义不同的项目先完成对照校准，不能直接套用 5 倍门槛。

完成判据：其他开发者可从版本化材料重建指定负载；报告能区分内部耗时、墙钟、内存采样和不同对照语义。

01 验收：[版本化源码、清单与原始样本](../benchmarks/performance_baseline_2026-09-29/README.md)。现存历史样本已归档；旧 DLL 的初始配对及语言特性逐轮原始数据无法从旧材料独立证明，外部题仍依赖仓库外数据，均在验收记录中明确限制。

### 02. GC 安全点的无锁快速返回与上下文传递

涉及文件：[cycle_gc.cpp](../src/backend/cpp/cycle_gc.cpp)、[cycle_gc.hpp](../src/backend/cpp/cycle_gc.hpp)、[runtime_context.hpp](../src/backend/cpp/runtime_context.hpp)、[runtime_context.cpp](../src/backend/cpp/runtime_context.cpp)、[codegen.cpp](../src/backend/llvm/codegen.cpp)、[codegen_declarations.cpp](../src/backend/llvm/codegen_declarations.cpp)。

- [x] **02.1 调整阈值判断顺序。** 在不改变 collection_interval 的情况下，先判断 collecting、工作线程限制，以及 allocations_since_collection 是否低于最小阈值 64，再访问全局登记表。程序退出和显式 collect_cycles 路径保持独立，不能套用自动轮询的提前返回。
- [x] **02.2 向内部安全点显式传上下文。** LLVM 已持有的 tx_context 直接传给新的内部入口；不在入口重新查询 TLS。将 concurrent_depth 纳入上下文，执行作用域使用同一计数，保证嵌套进入、异常退出和回调退出成对恢复。
- [x] **02.3 保持错误传递。** 真正回收会运行 deinit，安全点不能标成不运行用户代码的叶子操作。快速返回仍按现有约定保留并报告待传播错误，不能无条件清空错误状态。
- [x] **02.4 处理高分配量但无循环节点的轮询。** 当分配计数超过最小阈值而登记表为空时，仅前移判断还不够。可增加受正确发布规则约束的原子节点数/登记版本提示，使“确定无节点”能够无锁返回；进入回收前仍取得锁和真实快照。只用提示优化调度，不能用提示代替对象图同步，也不能因缓存失效永久漏扫新节点。
- [x] **02.5 独立评估登记表分区。** 前四点完成后，若登记/快照仍是热点，再设计按 owner 的登记段与批量合并。当前 inspect_graph(owner) 先复制全局表后过滤，应减少无关节点复制；保留主线程整体扫描和工作线程 owner 扫描的引用数算法，不在第一步直接撤掉全局执行门。本轮查询专项不再因安全点访问登记表；尚无登记/快照仍是热点的独立证据，暂不分区。

完成判据：无回收工作且无待传播错误时，安全点快速路径不获取登记表互斥量；显式传上下文入口不再查询 TLS。达到阈值后的回收、deinit 复活、工作线程退出及 join 后整体扫描保持有效。记录 dictionary/set/queue/cancel 专项，同时用 map 索引确认其他路径没有退化。

02 实施与定向验收见[记录](../benchmarks/performance_optimization_02_2026-09-29/README.md)。前四项结构与行为检查通过；map 的同轮计时约慢 0.4 ms，尚未满足整项性能完成判据。

### 03. 统一调用副作用、借用和 GC 判定

涉及文件：[公共调用属性](../src/common/call_properties.hpp)、[AST 调用属性](../src/frontend/ast/call_properties.cpp)、[语义标注](../src/frontend/sema/sema_call_properties.cpp)、[GC 判定](../src/backend/llvm/codegen_gc.cpp)、[语句生成](../src/backend/llvm/codegen_statement.cpp)、[调用借用](../src/backend/llvm/codegen_call_borrow.cpp)。

- [x] **03.1 统一属性来源。** 以完成类型和重载绑定后的签名、接收者类型及构造来源为依据，维护分配、可能登记 GC 对象、用户回调、用户析构、参数修改和保存等属性。未知入口保持保守；不要再为借用与 GC 各自维护互相矛盾的名称白名单。
- [x] **03.2 区分普通分配和 GC 可达图变化。** 标量容器扩容虽然不创建循环节点，仍计入分配触发次数，必要时轮询回收；只读 contains/size 等无分配查询则无需逐次轮询。文本结果复制目前会分配，必须按实际实现更新属性和旧注释。
- [x] **03.3 检查整条语句的效果。** 除目标调用外，还计算接收者、实参求值、隐式转换、返回值清理、旧值覆盖和析构的效果。只有整体证明安全，才去掉语句后的安全点；保留危险实参重绑定时的临时持有。
- [x] **03.4 首批覆盖有证据的入口。** 先覆盖 dictionary 标量 contains、基础 map/set 查询、基础 queue/deque front/back/pop 与 cancel.status；pop 是否安全取决于元素能否触发用户析构。查询仍保留边界/状态检查和必要同步，不标为可常量折叠的纯函数。

完成判据：安全查询循环没有 txrt_gc_safepoint、接收者 clone 或标量键装箱；可能执行用户代码的同名方法仍保留完整路径。借用、别名、NaN/正负零、回调重绑定和析构报错的既有语义保持。

03 实施与定向验收见[记录](../benchmarks/performance_optimization_03_2026-09-29/README.md)。

### 04. 去掉普通值句柄不需要的互斥量

涉及文件：[runtime_abi_internal.hpp](../src/backend/cpp/runtime_abi_internal.hpp)、[runtime_abi.cpp](../src/backend/cpp/runtime_abi.cpp)、[value_abi.cpp](../src/backend/cpp/value_abi.cpp)、[text_reference.cpp](../src/backend/cpp/text_reference.cpp)、[跨线程边界](send_sync.md)。

- [x] **04.1 拆分句柄记录。** 区分普通 any 根句柄和需要文本内部引用计数的状态。保持 register_handle/unregister_handle 的共同链表接口，避免每个 value 记录都构造 reference_mutex。
- [x] **04.2 明确同步归属。** 普通根句柄只在所属线程登记/注销；mutex、channel、cancel 等可共享值的载荷继续由自身实现同步。去除根句柄的无用互斥量不代表去除被装箱载荷的锁。
- [x] **04.3 更新创建、释放和错误清理。** make_handle、destroy_handle、cleanup_live_handles 必须使用正确记录类型；最后释放仍允许用户析构，error_cleanup_guard 继续保留最初错误。检查继承布局和转换，不依赖旧偏移强行重解释。
- [x] **04.4 检查所有边界。** 标量装箱、容器返回、线程结果、任务结果和原生回调均纳入句柄布局核对；ABI 指纹与产物整体重建同步进行。

完成判据：整数/布尔/普通 any 句柄的创建和销毁机器码中不再出现仅用于根记录的 pthread_mutex_init/destroy；文本跨线程最后释放、错误退出及析构错误不受影响。

04 的实现、机器码核对、定向边界与同轮性能见[记录](../benchmarks/performance_optimization_04_2026-09-29/README.md)。普通 any 去锁已确认；格式化负载尚无稳定整体提速。

### 05. 不可变文本共享与安全借用

涉及文件：[text_reference.hpp](../src/backend/cpp/text_reference.hpp)、[text_reference.cpp](../src/backend/cpp/text_reference.cpp)、[runtime_abi.cpp](../src/backend/cpp/runtime_abi.cpp)、[字符串借用生成](../src/backend/llvm/codegen_string_borrow.cpp)、[codegen.cpp](../src/backend/llvm/codegen.cpp)、[文本向量 ABI](../src/backend/cpp/vector_text_abi.cpp)、[字符串向量 ABI](../src/backend/cpp/vector_str_abi.cpp)、[容器 ABI](../src/backend/cpp/container_abi_internal.hpp)。

- [x] **05.1 定义根与内容的寿命。** 每个线程拥有独立根句柄和清理链表节点，不可变文本内容独立共享。容器内部引用持有内容，不借用可能先被注销的根节点。记录创建、克隆、容器保存、线程传递和最后释放的所有权规则。
- [x] **05.2 分开可变构建与不可变发布。** 格式化、拼接、解码先写入独占 builder，完成后发布不可变内容。已发布内容不能被后续 append 修改，避免为复用存储破坏字符串值语义。
- [x] **05.3 消除克隆中的内容复制。** clone 创建必要的本地根并共享内容；同线程已证明安全的短时读取直接借用。首先保证长文本不因传参而复制全部字节，再评估小字符串优化与控制块开销，不能只优化长文本导致短文本普遍退化。
- [x] **05.4 扩展借用范围。** 优先覆盖 parse 输入、只读格式化参数、字典字符串查询及只读 bytes/encoding 输入。禁止保存借用地址；后续实参可能重绑定原变量、回调或释放对象时继续持有。
- [x] **05.5 迁移原生边界。** 当前许多 ABI 将 void 指针直接视为 std::string，需要先列出实际读写点，再按模块迁移到明确的只读视图/构建接口。保留跨线程发布与原子最后释放的正确顺序，不能直接把旧根计数换成 atomic 后宣称完成。

完成判据：不逃逸的只读借用没有文本 clone；需要持有时，长文本 clone 不复制内容。短/长文本、容器别名、线程退出、错误清理和 deinit 读取文本均保持行为；复测字符串转换、格式化、解析以及 mini-filesystem 对应输入。

05 的实现、边界行为、同轮样本及未覆盖内容见[验收记录](../benchmarks/performance_optimization_05_2026-09-29/README.md)。

### 06. 解析结果标量化与整数解析核心

涉及文件：[parse.txh](../tx/stdlib/parse.txh)、[error.txh](../tx/stdlib/error.txh)、[parse_abi.cpp](../src/backend/cpp/parse_abi.cpp)、[error_result.hpp](../src/backend/cpp/error_result.hpp)、[error_abi.cpp](../src/backend/cpp/error_abi.cpp)、[parse.cpp](../src/stdlib/parse.cpp)、[表达式值生成](../src/backend/llvm/codegen_expression_values.cpp)、[局部 option 实现参考](../src/backend/llvm/codegen_native_option.cpp)。

- [x] **06.1 固定公开契约。** try_parse_int 返回 errors.int_result，字段为可按原规则访问的 ok/value/error；不更换返回类型，不要求用户迁移为内置 result<int>。成功值、失败默认值、错误 kind/code/message 以及可修改字段均需保留。
- [x] **06.2 新增轻量内部返回。** 解析核心通过类型明确的结果保存成功标记、标量值和可延迟展开的错误描述。对不会逃逸的局部结果，以栈槽/SSA 保存；只有读取完整错误、产生共享别名、进入 any、传给未知调用或跨原有 ABI 时才物化原结构。
- [x] **06.3 处理可写字段和别名。** 首批只接受用途能完整分析的局部结果。字段写入、别名赋值和异常路径合流必须维持一致表示；无法证明时退回完整结构。不能只优化 .ok 读取而使之后的 .value/.error 或字段修改失效。
- [x] **06.4 让错误信息延迟构造。** 成功路径不分配空 error_info；失败路径先保存与原契约一致的描述，观察错误字段时再生成字符串及对象。省掉本身不会产生用户副作用的临时构造，仍保留发生错误时的源码位置、类别与优先级。
- [x] **06.5 优化数字扫描。** 借用输入视图；将溢出界限分解为预先计算的 cutoff/cutlim，避免逐字符除法。常量十进制可走专门核心；溢出后仍完成必要的非法字符判断，保留最小负数、前后空白、正号及 2～36 进制规则。

完成判据：本地只检查 .ok 的成功循环不创建 result/error_info 动态对象、不为结果登记 GC、不复制输入文本。失败仅检查 .ok 时也不构造不被观察的完整错误对象；读取、修改或传递完整结果的原用法行为不变。分别报告解析核心与公开接口结果，不能把 C++ 简化对照直接视为完整契约目标。

06 的内部表示、物化边界、行为/IR 检查与解析核心、公开接口的独立采样见[验收记录](../benchmarks/performance_optimization_06_2026-09-29/README.md)。首批优化直接位置实参初始化的 int/float 局部结果；字段写入即物化，命名/展开实参保持原路径。整数使用通用 cutoff/cutlim 核心，未额外引入十进制专用实现。

### 07. 格式化直接写入与静态计划融合

涉及文件：[codegen_format.cpp](../src/backend/llvm/codegen_format.cpp)、[codegen_format_plan.cpp](../src/backend/llvm/codegen_format_plan.cpp)、[format_static_abi.cpp](../src/backend/cpp/format_static_abi.cpp)、[format_direct_abi.cpp](../src/backend/cpp/format_direct_abi.cpp)、[format.cpp](../src/stdlib/format.cpp)、[format_spec.cpp](../src/stdlib/format_spec.cpp)。

- [x] **07.1 增加直接写入接口。** 基于已有静态计划，将整数、布尔和文本直接 append 到同一 builder；整数 to_chars 的结果直接进入目标缓冲，避免每个字段先构造完整临时 string。
- [x] **07.2 合并初始化和容量安排。** 静态字面量长度可预先累计；动态字段使用可证明的长度/上界或按需增长，检查容量溢出，避免盲目按最大宽度超量预分配。一次完成 builder 创建，减少 begin/literal/append 之间的 ABI 往返。
- [x] **07.3 生成常见格式专用路径。** 对默认整数格式、原样文本、无宽度/精度的字段省略通用格式分派和不必要的 Unicode 长度计算；复杂 spec 保留原实现。常量合法性可预解析，但无效格式仍在原执行路径产生可捕获错误。
- [x] **07.4 保持实参行为。** 所有实参按源码顺序求值，未引用实参仍求值；后续实参修改前面的值时，使用原有持有规则。静态文本字节在允许的接口直接借用，运行时文本遵循第 05 项。
- [x] **07.5 优化真正动态模板。** 先对可证明不变的局部模板传播常量；仍动态的模板使用容量受限、键包含完整模板内容和必要模式的计划缓存。缓存不保存本次参数或借用地址，处理重入与并发，并保持逐次参数校验及错误行为。

完成判据：基础字面量格式化不构造动态实参容器、不创建每字段临时文本；缓存计划不改变动态参数和错误结果。复测 literal/dynamic 两条路径，另覆盖转义、命名字段、Unicode 宽度、精度和错误实参的必要场景。

07 验收：[实现、边界、工具链标识与同轮原始样本](../benchmarks/performance_optimization_07_2026-09-29/README.md)。原综合负载的局部模板属于可传播常量，另以函数参数传入并轮换模板测量真正动态路径；其本轮整体耗时仅下降 6.85%，不将静态化收益归给缓存。

### 08. serde 直接编解码，消除整棵中间动态树

涉及文件：[codegen_serde.cpp](../src/backend/llvm/codegen_serde.cpp)、[serde.hpp](../src/stdlib/serde.hpp)、[serde.cpp](../src/stdlib/serde.cpp)、[serde_value.cpp](../src/stdlib/serde_value.cpp)、[serde_schema.cpp](../src/stdlib/serde_schema.cpp)、[serde_abi.cpp](../src/backend/cpp/serde_abi.cpp)、[JSON 解析器](../src/stdlib/json_parser.cpp)、[JSON 流接口](../src/backend/cpp/json_stream_abi.cpp)。

- [x] **08.1 复用静态 schema。** 当前 schema 已由编译器生成，不再做一轮“缓存 schema JSON”的优化。为现有描述补充必要的字段访问、写入和解码入口，字段名、类型、版本、默认值和未知字段策略均在编译期绑定。
- [x] **08.2 直接序列化字段。** 从实际结构体字段读取静态类型值，直接写 JSON/CBOR builder，不先转成 tx_dict。沿用既有数字、转义、字段输出顺序和版本字段约定；保留环检测及深度、大小限制。
- [x] **08.3 直接解析到临时字段槽。** 复用解析器词法/值读取能力，按字段描述分派，使用位图或同等结构检查重复和缺失字段。默认值在相应槽位填入；避免“JSON 动态树 → schema 校验 → 第二个对象”的完整中转。
- [x] **08.4 原子提交结果。** 任一字段失败时释放已经构造的字段，不能向调用方暴露半初始化对象；全部成功后才创建或提交最终结构体。借用输入文本的字段必须在输入释放前取得符合寿命要求的所有权。
- [x] **08.5 分层处理复杂类型。** 先覆盖 int/float/bool/str 与静态嵌套结构，再接入 option/vector/bytes 等当前支持类型。unknown=preserve 只为实际未知内容保留动态表示；不能通过快路径跳过重复字段、未知字段、版本或限额检查。

完成判据：固定字段结构体往返不再构造整棵中间 tx_dict/any 树，失败清理与旧路径一致。用相同 schema、相同校验能力的 C++ 对照报告倍率；原 44～88 倍仅作为历史参考。

08 验收：[实现、结构与失败清理、ABI 标识和同轮原始样本](../benchmarks/performance_optimization_08_2026-09-29/README.md)。成功路径直接编解码字段；失败后复用原验证路径保持错误优先级。C++ 对照使用相同 schema 的完整契约原生调用，与候选共享编解码器；字段仍采用现有 any 槽，静态内存布局留给 09。

### 09. 结构体和类的静态字段布局

涉及文件：[类型定义](../src/common)、[AST](../src/frontend/ast/ast.hpp)、[类语义分析](../src/frontend/sema/sema_class.cpp)、[codegen_class.cpp](../src/backend/llvm/codegen_class.cpp)、[codegen_fields.cpp](../src/backend/llvm/codegen_fields.cpp)、[value_format.hpp](../src/backend/cpp/value_format.hpp)、[value_abi.cpp](../src/backend/cpp/value_abi.cpp)、[class_abi.cpp](../src/backend/cpp/class_abi.cpp)、[class 语义](classes.md)。

- [ ] **09.1 先定义内部布局契约。** 为静态结构体/类建立不可变类型描述，包含字段类型、偏移、对齐、继承关系、虚表、析构和 GC 扫描信息。复用规范化模块的唯一符号，避免不同模块同名类型碰撞；类型描述不得在每个实例中复制名称字符串。
- [ ] **09.2 先迁移仅含标量的结构体。** 以 point/pair 等为首批，保持对象共享身份，但将 int/float/bool 放进固定槽位；构造直接初始化字段，读写生成 GEP/load/store。any 恢复和原生入口负责必要类型检查，静态内部字段访问不重复检查。
- [ ] **09.3 再迁移混合字段和类。** 文本、字节、容器与对象字段保留有类型的引用槽；类保留既有多继承/接口视图和初始化规则。字段地址、虚槽及继承偏移由编译期描述决定，不把类型已知的字段反复还原成 std::any。
- [ ] **09.4 按逃逸证明消除局部堆对象。** 仅对无别名观察、无动态装箱、无地址逃逸且析构可保持的局部对象实施栈分配或标量替换。普通赋值仍是共享引用；不能因为字段全是整数就把对象改成按值复制。
- [ ] **09.5 优化虚调用和转换。** 确定目标直接调用；真实虚分派读固定虚表槽。运行时 as 使用类型描述身份或静态生成的祖先/接口转换表，保留失败错误及多继承调整；类型 ID 在链接后的程序范围内一致。
- [ ] **09.6 接入析构、GC 和其他消费者。** 能证明永不形成环的类型不登记循环节点；含 any/动态引用的类型保守扫描。保持最派生到祖先的 deinit 顺序、最后引用释放、析构错误和复活。同步适配 serde、deep_copy、打印及跨模块 ABI，避免新旧布局被混用。

完成判据：标量字段热循环不再调用 txrt_struct_field_* 或 txrt_class_field_*；仅标量对象不再为每个字段创建 any。module_call 验收同时看对象创建和字段访问，不能只以函数被内联判为完成。共享别名、多继承、跨模块同名类型、析构和复活行为保持。

### 10. 函数值和闭包直接传递上下文

涉及文件：[codegen_expression_calls.cpp](../src/backend/llvm/codegen_expression_calls.cpp)、[codegen_callback.cpp](../src/backend/llvm/codegen_callback.cpp)、[codegen_closure.cpp](../src/backend/llvm/codegen_closure.cpp)、[closure_abi.cpp](../src/backend/cpp/closure_abi.cpp)、[closure.hpp](../src/stdlib/closure.hpp)、[函数值契约](function_values.md)。

- [ ] **10.1 区分 TX 内部与原生回调入口。** TX 内部函数值采用显式 context + environment + 静态参数的调用形式；只有线程、任务、库回调等真正原生边界取得线程上下文。保留边界适配器，不能让外部 C 回调错误地复用创建闭包线程的 context。
- [ ] **10.2 减少目标恢复。** 直接函数值且目标可证明时生成直接调用；否则从稳定、类型明确的闭包头读 code/environment。只有环境不会重绑定或被用户回调影响时，才把读取提升到循环外。
- [ ] **10.3 类型化 bind 环境。** 捕获字段类型和顺序由编译器确定，标量捕获直接读取；多层 bind 的包装仅在求值顺序和父环境寿命可证明时合并。跨线程 move、共享捕获、独立 deep_copy 与循环关系保持原契约。
- [ ] **10.4 传播调用效果。** 已解析到实际函数的函数值使用其效果摘要，消除不必要安全点；未知函数值继续保守。调用失败仍保留逻辑调用位置、参数清理和原错误，不因减少包装而吞掉错误。

完成判据：内部已知函数值循环不再逐次执行 txrt_closure_code 或回调边界 TLS 查询；动态函数值保留真实间接调用，闭包跨模块返回、错误、跨线程捕获与循环回收保持行为。

### 11. 堆操作的借用与无临时堆分配路径

涉及文件：[typed_heap.hpp](../src/stdlib/typed_heap.hpp)、[ordered_compare.hpp](../src/stdlib/ordered_compare.hpp)、[heap_abi.cpp](../src/backend/cpp/heap_abi.cpp)、[调用属性](../src/frontend/ast/call_properties.cpp)、[容器调用生成](../src/backend/llvm/codegen_container.cpp)、[容器语义](typed_containers.md)。

- [ ] **11.1 精确识别默认比较器。** 不能只因 heap<int> 就认定不执行用户代码，该类型也支持自定义比较器。以构造来源和对象别名证明默认比较器属性，并在无法追踪的调用/赋值边界保守失效。
- [ ] **11.2 对安全操作开放借用。** 默认比较器、基础元素且无危险实参时，push/top/pop 借用接收者，避免 clone/release。对象元素、用户比较器、可能触发析构的操作保留持有和错误检查。
- [ ] **11.3 消除 parents/children 的动态分配。** 调整路径长度最多为索引位宽量级，使用由 size_t 位数约束的栈缓冲或同等无分配结构。先完成所有可能失败的比较，再提交移动，维持失败后堆内容不变的保证。
- [ ] **11.4 专门化比较过程。** 默认标量比较直接比较值，再按原稳定序号处理相等元素；保留 NaN 检查、升降序和序号耗尽错误。自定义比较器调用顺序、重入保护及错误传播保持不变。

完成判据：默认标量堆操作没有接收者临时句柄和路径 vector 分配；稳定顺序、自定义比较器报错后状态、重入错误及元素析构保持。记录算法成本与公共 ABI 成本各自的变化。

### 12. 迭代器与剩余向量循环

涉及文件：[codegen_iterator.cpp](../src/backend/llvm/codegen_iterator.cpp)、[codegen_native_option.cpp](../src/backend/llvm/codegen_native_option.cpp)、[codegen_vector_loop.cpp](../src/backend/llvm/codegen_vector_loop.cpp)、[codegen_vector_reference.cpp](../src/backend/llvm/codegen_vector_reference.cpp)、[iterator_abi.cpp](../src/backend/cpp/iterator_abi.cpp)、[iterator.hpp](../src/stdlib/iterator.hpp)、[迭代规则](iterators.md)。

- [ ] **12.1 保留现有标量 option 路径。** 当前 snapshot_iter 的局部标量 next 已返回 present + value，先确认机器码仍走该路径，不重复引入已完成的去装箱。
- [ ] **12.2 增加稳定游标快路径。** 对同步、无用户回调且不会经别名 close/next 的局部游标，借用类型明确的状态并直接推进 index。snapshot 可稳定其长度和数据；live_iter 必须遵守修改后读取与永久耗尽规则，不能套用快照不变量。
- [ ] **12.3 精确移动检查。** 循环证明覆盖范围时才提升 closed/边界检查；循环中调用、别名改变或提前错误使证明失效时退回逐次检查。原来的每次推进即使后续用户代码报错也已生效，不能在循环末才统一提交索引。
- [ ] **12.4 分析小向量扫描的剩余差距。** 先统一 C++ 的 int64 宽度，分别检查外层循环持有、溢出检查和向量化。只在值域及别名可证明时减少检查；复测 1k/100k 两种尺度，不能仅针对某一固定长度写优化。

完成判据：稳定 snapshot 标量循环不再逐元素跨 ABI/TLS 取值；复杂迭代器仍执行完整状态语义。已接近 C++ 的 vector_index 和大向量扫描保持性能。

### 13. 算术、递归、诊断与参数展开

涉及文件：[codegen_expression_ops.cpp](../src/backend/llvm/codegen_expression_ops.cpp)、[codegen_context.cpp](../src/backend/llvm/codegen_context.cpp)、[codegen_functions.cpp](../src/backend/llvm/codegen_functions.cpp)、[codegen_unpack.cpp](../src/backend/llvm/codegen_unpack.cpp)、[codegen_call_binding.cpp](../src/backend/llvm/codegen_call_binding.cpp)、[call_abi.cpp](../src/backend/cpp/call_abi.cpp)、[语法规则](syntax.md)。

- [ ] **13.1 直接生成安全除法。** 非零常量整数除数且排除最小整数除 -1 时直接生成 sdiv；一般情况先检查零和溢出条件，正常分支使用原生指令，错误分支沿用现有错误处理。浮点除法按当前实际错误契约生成 fdiv 与必要检查，不默认使用 fast-math。
- [ ] **13.2 建立保守值域证明。** 优先利用字面量、循环边界和局部控制流证明加减乘不溢出；无证明保留现有 overflow intrinsic。带溢出语义的递归不能简单改成可能在不同位置溢出的求和公式。
- [ ] **13.3 优化诊断维护而保留可观察栈。** 先消除同一位置的重复写入，保留错误和显式 stack_trace 可观察到的逻辑帧。若要做更强的惰性帧或内联帧恢复，应作为独立设计与验收项，不把关闭诊断当作默认优化。
- [ ] **13.4 静态展开直接绑定。** 对编译期已知形状的数组/字典实参展开，生成确定的参数顺序和必要容器；若 callee 观察到可变参数容器的独立身份/修改，则保留物化。实参求值、重复键冲突、缺失参数和多余参数错误保持原规则。
- [ ] **13.5 动态展开减少重复工作。** 真正动态的展开仍检查类型和键；复用已经完成的签名绑定信息，避免重新按名称查找已知目标。减少临时装箱和重复复制前先证明调用方、callee 之间没有共享可变容器行为差异。

完成判据：安全除法正常路径没有相应运行时算术 ABI；递归和展开收益来自相同语义下生成代码的变化。递归、除零、边界溢出、错误栈和展开失败均保持原错误类别与源码位置。

### 14. 深拷贝、析构与循环对象图

涉及文件：[deep_copy.cpp](../src/backend/cpp/deep_copy.cpp)、[deep_copy.hpp](../src/backend/cpp/deep_copy.hpp)、[cycle_gc.cpp](../src/backend/cpp/cycle_gc.cpp)、[class_abi.cpp](../src/backend/cpp/class_abi.cpp)、[value_format.hpp](../src/backend/cpp/value_format.hpp)、[回收契约](garbage_collection.md)。

- [ ] **14.1 为已知类型生成复制和扫描描述。** 复用第 09 项类型描述，直接复制标量及有类型字段；动态 any 才走通用分派。通用入口优先识别常见纯值，减少每个整数先尝试多种容器/资源类型的判断。
- [ ] **14.2 保持身份表算法。** 对可能共享或成环的对象先登记新对象再递归填充；重复引用指向同一份新对象。只有证明整个子图无环且无可观察共享关系时，才省掉该部分身份表，不能按单次样例形状硬编码。
- [ ] **14.3 减少图扫描和登记垃圾。** 无环类型跳过登记；扫描只访问描述中可能持有对象引用的字段。按第 02 项的 owner/全局边界减少无关快照，复用工作区时清除所有强引用，禁止扫描工作区本身把垃圾保活。
- [ ] **14.4 保留析构与复活。** 不可达类仍先析构再重判可达性，确保一次 deinit、字段可读和复活后对象存活；清理阶段保持最初错误。阈值调大只能作为独立延迟/内存权衡，不能用来掩盖分配和扫描成本。

完成判据：固定布局复制减少动态类型分派，但任意别名、自环、互环、共享子图和复活均保持。补齐等价通用 C++ 对照后再确定倍率目标，同时记录峰值内存与回收延迟。

### 15. bytes、编码、统计与字符串的剩余库内成本

涉及文件：[bytes.cpp](../src/stdlib/bytes.cpp)、[bytes_abi.cpp](../src/backend/cpp/bytes_abi.cpp)、[encoding_memory_abi.cpp](../src/backend/cpp/encoding_memory_abi.cpp)、[codegen_encoding.cpp](../src/backend/llvm/codegen_encoding.cpp)、[statistics_internal.hpp](../src/stdlib/statistics_internal.hpp)、[statistics_core.cpp](../src/stdlib/statistics_core.cpp)、[字符串直接接口](../src/backend/cpp/text_direct_abi.cpp)。

- [ ] **15.1 先复测公共修复后的余量。** bytes_hex、编码和字符串转换先享受第 02～05 项收益；只有仍存在库内热点才继续改算法。保留已实现的 UTF-8 直接校验和常量编码选路。
- [ ] **15.2 直接写目标缓冲。** 十六进制输出在检查 2×长度溢出后一次确定大小并直接填充；解码先完成必要长度/字符检查，输出缓冲只构造一次，失败不交付部分数据。短输入与大输入分别比较，避免为短字符串引入复杂设备级优化。
- [ ] **15.3 UTF-8 不可变内容复用。** 在编码严格校验通过且表示、寿命、连续存储允许时，评估 str/bytes 共享同一不可变内容的路径；若需要转换或布局不兼容则一次分配复制。保留非法编码、切片边界和输出值不可变语义。
- [ ] **15.4 统计保持精度目标。** 与同等补偿累计的 C++ 先对齐；针对向量一次读取连续存储、提升可证明不变的检查。只有证明误差、范围和消费迭代器语义满足原契约时才考虑分块累计，不直接用普通 double 求和替换。
- [ ] **15.5 字符串转换按实际热点处理。** 核对现有 to_chars/from_chars 使用范围及错误路径，避免重写已经直接转换的核心。优先处理结果句柄、借用、容量和重复转换，不改变 Unicode 长度、数值格式与解析契约。

完成判据：bytes/encoding 的多余缓冲分配减少；合法/非法输入、Unicode 边界、统计正负抵消与极值均保持。统计和编码的专用 C++ 参考与等价契约结果分别呈现。

### 16. 内联、跨库优化与收尾

涉及文件：[CMakeLists.txt](../CMakeLists.txt)、[build.ps1](../scripts/build.ps1)、[driver/main.cpp](../src/driver/main.cpp)、[兼容验证](../src/driver/compatibility.cpp)、[原生后端说明](native_backend.md)。

- [ ] **16.1 先确认剩余 ABI 边界。** 用最终候选的优化后 IR/反汇编查明哪些短函数仍无法内联；不把源码里存在 call 当成优化后机器码仍存在 call。module_call 的对象成本已在第 09 项处理。
- [ ] **16.2 有条件评估一致工具链和 LTO。** 当前标准库与 TX 目标程序构建方式不同，不能只给链接器增加 flto。若实测仍受跨库调用限制，再选择一套能端到端处理兼容 IR、异常模型和 C++ 运行库的构建方案，小范围验证后迁移。保留普通构建路径作为对照。
- [ ] **16.3 核对成本与兼容性。** 同时记录编译耗时、链接耗时、包体积和运行性能。不得为短调用收益破坏第三方库 ABI、线程库配对、原生回调及包兼容校验。
- [ ] **16.4 形成最终状态表。** 每项记录已实现、结构检查、定向行为、性能结果和仍未完成内容。只处理本计划实际热点；未建立独立基线的模块继续标记未覆盖，不以“全量优化完成”概括。

完成判据：跨库优化有独立收益证据且产物兼容；如果前序改动后已无相应热点，记录“评估后不实施”及证据，不为完成清单强行引入构建复杂度。

## 5. 定向验证与性能验收

### 5.1 每项最小闭环

1. 修改前记录对应基准源码与产物标识。修改后先做编译和生成代码检查，确认目标操作确实消除。
2. 复用已有行为场景，只补本次优化新增边界。优先使用下表列出的相关检查，不重复运行无关模块的全量套件。
3. 影响 ABI 的阶段整体构建一次，再执行相关组；修复了实际失败或新增影响范围时才扩展验证。
4. 结构与行为检查通过后，同机交替测量旧/新程序，保留每轮耗时、校验值、异常样本和中位数。不要用跨日单值证明收益。
5. 短项噪声大或新增路径仍解释不了差距时，针对单个热点做采样/计数；不为所有模块常驻加入性能计数器。
6. 最终候选冻结后做一次覆盖本计划受影响套件的汇总，确认整体收益和未解决项，不在每个小点后重跑全量基准。

### 5.2 现有检查与重点补充边界

下表是后续实现时的选择清单，不表示本次已执行，也不要求每次修改把整行所有场景全部重跑。若一个脚本覆盖面明显大于改动，优先从其已有用例中选取相关场景。

| 修改项 | 可复用材料 | 必须关注的边界 | 性能负载 |
| --- | --- | --- | --- |
| 02～03 GC 与属性 | [runtime_context 检查](../scripts/check_runtime_context.py)、[借用检查](../scripts/check_call_borrowing.py)、[复活示例](../examples/cycle_resurrection.tx) | 已有待传播错误、阈值、借用失效、deinit 复活、worker/join | call_borrowing，map 索引对照 |
| 04～05 句柄与文本 | [跨线程文本](../tests/stdlib/send_sync_text.tx)、[线程生命周期](../tests/stdlib/thread_lifecycle.tx)、[并发错误](../scripts/check_concurrency_errors.py) | 根/内部引用、跨线程释放、错误退出、短长文本 | string_conversion、format、mini-filesystem |
| 06 解析结果 | [局部结果检查](../scripts/check_parse_scalar.py)、[解析错误检查](../scripts/check_parse_errors.py)、[解析用例](../tests/stdlib/parse_errors.tx) | 成功也可读取 error、字段修改、别名、INT64_MIN、混合非法与溢出 | [局部/完整结果与核心采样](../benchmarks/performance_optimization_06_2026-09-29/README.md) |
| 07 格式化 | [格式化定向检查](../scripts/check_format_optimized.py)、[静态运行时用例](../tests/containers/static_runtime.tx) | 实参顺序、模板错误、Unicode 宽度、缓存并发重入 | format_literal / format_dynamic |
| 08 serde | [直接编解码检查](../scripts/check_serde_direct.py)、[行为](../tests/serde/behavior.tx)、[跨模块](../tests/serde/module.tx)、[迁移](../tests/formats/serde_migration.tx) | 重复/未知字段、版本、默认值、嵌套限额、失败清理 | serde_short_text / serde_long_text，serde_json |
| 09 对象布局 | [类示例](../examples/advanced_classes.tx)、[跨模块类](../examples/advanced_class_module/main.tx)、[内存示例](../examples/memory_management.tx) | 别名、多继承、初始化、动态恢复、析构与复活 | struct_operators / class_methods / module_call / runtime_cast / deinit |
| 10 闭包 | [闭包错误](../tests/stdlib/closure_error.tx)、[闭包环](../tests/stdlib/closure_cycle.tx)、[跨模块闭包](../tests/stdlib/closure_module/main.tx) | 捕获寿命、move、包装失败、递归回调 | function_value / closure_bind |
| 11 堆 | [类型化容器检查](../scripts/check_typed_containers.py) | 稳定性、用户比较器抛错/重入、NaN、空堆 | heap_push_pop |
| 12 迭代器与向量 | [关闭迭代器](../tests/stdlib/iterator_closed.tx)、[跨模块迭代器](../tests/stdlib/iterator_module/main.tx)、[向量检查](../scripts/check_vectors.py) | snapshot/live、别名 close/next、永久耗尽、异常后推进 | iterator_snapshot、1k/100k 扫描、vector_index |
| 13 算术与展开 | [数值边界](../examples/numeric_bounds.tx)、[递归](../examples/recursion.tx)、[参数展开](../examples/variadic_unpack.tx) | 除零、INT64_MIN/-1、溢出位置、栈轨迹、键冲突 | recursion / float_arithmetic / variadic_unpack |
| 14 深拷贝与 GC | [循环回收](../examples/cycle_collection.tx)、[复活](../examples/cycle_resurrection.tx)、[闭包环](../tests/stdlib/closure_cycle.tx) | 共享子图、自环互环、跨 owner 图、一次析构 | deep_copy / copy_cycle / cycle_gc |
| 15 库内热点 | [静态运行时检查](../scripts/check_static_runtime.py)、[统计](../tests/statistics/basic.tx) | 非法字节、长度上限、Unicode、数值抵消和极值 | bytes_hex / encoding / statistics_mean |
| ABI 改动及 16 | [包兼容检查](../scripts/check_package_compatibility.py)、受影响的原生回调 | 指纹变化、桥接对象、DLL 配对、旧包拒绝 | 受影响热点及编译耗时 |

### 5.3 性能门槛的使用

- **公共回退修复**：先确认约 25 ns/操作的公共新增成本显著减少，并与保留的 9 月 27 日程序同轮比较；恢复历史值不能替代与 C++ 的后续目标。
- **等价简单操作**：以低于 C++ 5 倍为阶段目标，同时报告绝对耗时、样本分布和输入规模。未达到则继续定位剩余成本，不能通过缩减循环或调整校验绕过。
- **复杂契约**：serde、统计、通用 GC/深拷贝以完整契约对照为准；等价对照建立前只验收结构改善、TX 前后耗时和行为保持。
- **防止转移成本**：字符串共享、缓存和 GC 分区同时记录内存与生命周期影响；不能将工作简单移到计时范围外便视为总性能改善。
- **已快项目保护**：scalar_control、while_logic、向量索引、math 和文件 I/O 作为最终相关套件中的观察项，不要求每个小改动都单独重复运行。

## 6. 状态与交接

当前已完成 01 的归档、校准和定向验收；02.1～02.4 已实施且 02.5 完成条件性评估，但 02 的 map 性能观察仍待定位；03 已完成定向验收；04 已完成定向结构与行为验收，格式化负载尚无稳定整体收益；05、06 已完成定向验收；07 已完成定向验收，真正动态模板仍有明显容器和调用成本；08 已完成直接编解码定向验收；09～16 仍按前置条件推进。复选框表示各小点实施状态，整项验收还须满足其完成判据。

| 修改项 | 优先级 | 主要前置 | 当前状态 |
| --- | --- | --- | --- |
| 01 基线 | P0 | 无 | 已完成；限制见验收记录 |
| 02 GC 快速路径 | P0 | 01 | 已实施；map 性能回退待定位，整项待验收 |
| 03 调用效果统一 | P0 | 01；结合 02 | 已完成定向验收；等价 C++ 对照缺失 |
| 04 值句柄去无用锁 | P0 | 01 | 已完成定向结构与行为验收；格式化负载未见稳定整体收益 |
| 05 文本共享与借用 | P1 | 03、04 | 已完成定向验收；剩余热点交由 06～08 |
| 06 解析结果与核心 | P1 | 03～05；首批可用局部专用表示 | 已完成局部结构与行为验收；完整成功路径本轮回退 11.05%，见采样记录 |
| 07 格式化 | P1 | 03～05 | 已完成定向验收；字面量 2.02 倍、局部常量模板 10.60 倍、真正动态模板 1.07 倍，见独立记录 |
| 08 serde | P1 | 02～05；与 09 对接字段访问 | 已完成定向验收；JSON 短/长文本 1.87/1.36 倍，CBOR 2.39/1.83 倍 |
| 09 静态对象布局 | P1 | 03、04，明确布局契约 | 待实施 |
| 10 函数值与闭包 | P2 | 03；与 09 共用类型描述 | 待实施 |
| 11 堆 | P2 | 03、04 | 待实施 |
| 12 迭代器与向量 | P2 | 03、04，01 的等价基线 | 待实施 |
| 13 算术与展开 | P2 | 03；涉及布局时依赖 09 | 待实施 |
| 14 深拷贝与对象图 | P2 | 02、09、10 的相关表示 | 待实施 |
| 15 剩余库热点 | P2 | 02～05 的复测；热点仍存在 | 待实施 |
| 16 内联与构建评估 | P3 | 对应前序修改稳定；热点仍存在 | 待评估 |

后续每完成一个修改项，在本表更新状态，并附独立验收记录，至少包含：

1. 修改项及小点编号、实际修改文件、未覆盖内容。
2. 基线与候选的源码/产物标识，是否含工作树改动。
3. 生成 IR/机器码中减少了什么，仍保留什么以及原因。
4. 相关语义检查、执行结果和边界；失败项未解决时不能标完成。
5. 同轮原始性能样本、校验值、中位数、TX 前后比例、等价 C++ 倍率及测量限制。
6. ABI、包兼容、编译成本或内存有无变化，下一项的前置条件是否满足。

本计划保持现有语法、公开 API 和正确性保证，实施范围以已定位的热点及其必要依赖为准。实施时按最新工作树重新核对证据，避免将本次候选状态当成永久不变的事实。
