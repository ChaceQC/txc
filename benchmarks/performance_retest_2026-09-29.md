# 2026-09-29 全量性能复测

本轮在当前 `master` 工作树上重建 TX 工具链，并执行仓库现存的全部性能基准。工作树含未提交改动；本报告只描述这份本地候选产物。所有基准程序均正常退出，固定校验值或正式答案检查通过；性能方面发现多处明显变慢。

## 环境与覆盖

- Windows 11 专业版 build 26200；AMD Ryzen 7 6800H；内存 15.2 GiB。
- TX 使用随包 clang 23.1.2；C++ 使用 GCC 13.1.0 Release；Python 3.12.10；Java 23.0.2；外部题另用 Node.js 24.13.0。
- 先执行 `scripts/build.ps1`，成功重建 `tx/txc.exe`、`tx/libtxstdlib.a` 和标准库桥接对象。编译器 SHA-256 为 `58ff0459fcee33b82a2a887f73109c3a71143cd10135b6ac23edb32161c771d3`，静态库为 `ec5f0804addcca0997178d951ab0e6e5a108fc12481a38bf4994eca6bb3519c1`。标准库组合、模块审计、网络、外部题及综合负载脚本在测量前后核对了这两个文件的哈希。

| 基准 | 本轮范围 | 采样方式 |
| --- | --- | --- |
| 语言特性 | 22 项，TX/C++/Python/Java | 各预热一次，交替 7 轮 |
| 标准库组合 | 10 项，TX/C++ | 各预热一次，交替 7 轮 |
| 标准库模块及新增特性 | 37 个旧审计模块的代表负载、10 项新增特性及系统负载 | 各预热一次，交替 3 轮 |
| 本机网络 | HTTP、Requests、WebSocket 共 3 项 | 各预热一次，交替 3 轮 |
| 外部题 | 2 题各 5 组输入，TX/C++/Python/Java/JavaScript | 每组每语言预热 2 次、正式 17 次；每次核对答案 |
| 综合负载 | 18 项，TX/C++/Python/Java；另测启动、编译和采样内存 | 运行预热 1 次、正式 5 次；启动 7 次；编译 3 次 |
| 专项 | 调用借用 5 项、静态运行时 5 项、长随机数 1 项 | 运行预热 1 次，正式分别 7/3/7 次 |

当前 `tx/stdlib/` 有 50 个公开 `.txh` 模块，既有模块性能审计只覆盖其中 37 个。尚无独立代表负载的 13 个模块为 `async_file`、`channel`、`dns`、`ipc`、`password`、`public_key`、`secret`、`socket`、`sync`、`task`、`thread`、`tls`、`x509`。因此这里的“全量”指仓库现存性能基准全量执行，不等于 50 个模块均已建立性能基线。

## 22 项语言特性

执行 `benchmarks/language_features/run_compare.ps1 -Rounds 7`。单位为程序内部耗时的 7 次中位数，毫秒。每轮四语言的 22 项校验值均符合脚本预期。

| 项目 | TX | C++ | Python | Java |
| --- | ---: | ---: | ---: | ---: |
| scalar_control | 0.17 | 0.2331 | 23.145 | 2.553 |
| updates | 2.06 | 1.2830 | 60.841 | 5.050 |
| while_logic | 0.16 | 0.1748 | 30.033 | 2.291 |
| float_arithmetic | 1.43 | 0.3493 | 24.494 | 1.712 |
| overloads | 0.30 | 0.1854 | 18.464 | 3.865 |
| named_arguments | 0.62 | 0.3706 | 25.546 | 2.935 |
| recursion | 1.01 | 0.0665 | 11.867 | 1.038 |
| variadic_unpack | 9.04 | 1.6695 | 3.385 | 0.920 |
| array_destructure | 3.13 | 1.0838 | 4.064 | 4.793 |
| array_padded | 0.03 | 2.1333 | 7.569 | 8.936 |
| dict_iteration | 43.27 | 4.1852 | 19.660 | 47.130 |
| struct_operators | 124.82 | 12.5045 | 78.288 | 11.358 |
| class_methods | 1.75 | 0.1602 | 46.064 | 5.013 |
| virtual_interface | 2.21 | 0.4260 | 35.272 | 3.638 |
| class_operator | 1.11 | 0.2359 | 16.151 | 2.338 |
| runtime_cast | 15.77 | 1.8364 | 12.972 | 2.307 |
| module_call | 30.44 | 3.2873 | 25.571 | 5.979 |
| string_conversion | 17.36 | 2.8834 | 9.905 | 14.087 |
| deep_copy | 44.96 | 2.3007 | 63.923 | 6.014 |
| copy_cycle | 15.42 | 2.0439 | 11.290 | 3.765 |
| deinit | 14.29 | 0.7069 | 4.061 | 1.821 |
| cycle_gc | 1.52 | 0.0706 | 1.310 | 1.751 |

## 主要性能差异

下表的旧值取自 [2026-09-27 提交后复测](performance_retest_2026-09-27_committed.md)，新值取自本轮相同负载。单位为程序内部耗时中位数，毫秒；正百分比表示本轮 TX 较慢。不同日期的比较用于发现候选热点，不能单独证明代码导致差异。

| 负载 | 旧 TX | 本轮 TX | 变化 |
| --- | ---: | ---: | ---: |
| set_contains | 1.620 | 3.926 | +142.3% |
| cancel_status | 1.771 | 4.031 | +127.6% |
| queue_push_pop | 5.505 | 12.192 | +121.5% |
| dictionary_contains | 1.727 | 3.740 | +116.6% |
| map_hit_10_percent | 10.134 | 21.829 | +115.4% |
| dictionary_int_hit | 4.160 | 8.765 | +110.7% |
| bytes_hex | 10.224 | 16.976 | +66.0% |
| mini-filesystem / random-2000-1 | 5.322 | 8.545 | +60.6% |
| mini-filesystem / linklong-2000 | 33.571 | 52.298 | +55.8% |
| mini-filesystem / fanout-2000 | 9.497 | 14.703 | +54.8% |
| format_literal | 11.000 | 15.241 | +38.6% |

`mini-filesystem` 其余两组 TX 也变慢：`moves-2000` 为 4.424 → 6.694 ms，`deep-pwd-2000` 为 19.267 → 23.976 ms。`not-yet-on-stage` 五组 TX 较旧记录高 2.4%～9.3%，两份 TX 程序分别通过 45/45 正式数据；两题每次计时输出均与正式答案一致。本机 HTTP 与 WebSocket 三项约在旧值的 ±3.5% 内。标准库组合的 10 项使用整数毫秒计时，短项变化不宜据此下结论。

为了排除跨日机器状态的主要干扰，另将 2026-09-27 保留的旧可执行程序与本轮同源码新程序在同一台机器上交替测量。调用借用项目各 7 轮；下表为程序内部计时中位数：

| 负载 | 旧程序 (ms) | 本轮程序 (ms) | 本轮 / 旧程序 |
| --- | ---: | ---: | ---: |
| dictionary.contains(int) | 12.985 | 38.238 | 2.94 |
| map<int,int> 索引读取 | 4.073 | 3.603 | 0.88 |
| set<int>.contains | 13.645 | 38.161 | 2.80 |
| queue<int> push/front/pop | 4.669 | 12.504 | 2.68 |
| cancel.status | 15.484 | 40.931 | 2.64 |

静态运行时旧/新程序同轮交替 3 次：`format_text` 为 19.055 → 33.296 ms，`encoding_utf8` 为 31.232 → 35.785 ms，`vector_foreach` 为 1.244 → 1.366 ms，`serde_json` 为 42.686 → 39.516 ms，`statistics_mean` 为 19.048 → 19.300 ms。长随机数负载 7 次中位数为 36 ms，校验和 `2500067985`，与旧轮 37 ms 接近。专项程序的固定校验值全部一致。

这些同轮对照支持字典、set、queue、取消状态和字面量格式化负载存在实际性能回退。尚未对代码改动逐项隔离或分析热点，不能把回退归因到某个具体模块。

## 进程、编译与内存

18 项综合程序的 TX 进程墙钟中位数为 481.81 ms，采样工作集峰值约 8.58 MiB；C++ 分别为 44.70 ms、5.11 MiB。空程序启动墙钟 TX 为 26.93 ms，C++ 为 23.69 ms。TX `check` 为 176.16 ms、约 9.75 MiB；TX 完整编译为 975.71 ms、约 71.05 MiB。内存按 2 ms 轮询进程树工作集，只是近似峰值。

本轮 C++、Python、Java 的进程墙钟、空程序启动和编译耗时也普遍高于 9 月 27 日，因此这些跨日墙钟变化不能直接归因于 TX。综合负载中 TX 的 18 项固定校验值与其他三种语言全部一致。

## 结果文件与边界

- [模块、网络、标准库组合及外部题原始样本](performance_baseline_2026-09-29/samples/performance_retest_20260929.json)；[旧轮样本](performance_baseline_2026-09-29/samples/performance_retest_20260927_committed.json)也已归档。
- [综合负载、启动、编译与内存原始样本](performance_baseline_2026-09-29/samples/diverse_performance_results_20260929.json)；[旧轮样本](performance_baseline_2026-09-29/samples/diverse_performance_results.json)也已归档。
- 本轮沿用 `tx_build/performance_retest_20260927.py` 及 `tx_build/perf_audit_20260927/` 的历史审计负载，先用当前工具链重新编译 TX 程序，再运行 `library`、`audit`、`network`、`external` 四组；综合负载沿用 `scripts/run_diverse_performance.py`。输出路径在进程内覆盖为本轮文件名，未覆盖旧轮 JSON。
- 审计中 `random_int` 的 TX/C++ 校验和一致，Python 和 Java 使用不同随机数实现，校验和不相同。各语言自身逐轮稳定；该项不可作为跨语言同随机序列的倍率。HTTP 负载的程序输出名称不同，仅逐语言检查稳定性。
- 复测时 `tx_build/` 被 Git 忽略。计划 01 后已将现存原始 JSON 与审计源码逐文件核对并归档到 `benchmarks/performance_baseline_2026-09-29/`；二进制旧程序及其候选 DLL 配套仍只留在本机忽略目录。历史对照表没有因后续校准而改写。

## 计划 01 后续基线校准

[验收记录](performance_baseline_2026-09-29/README.md)列出原始证据、重建命令、同轮新旧样本、64 位 C++ 容器、解析核心/完整结果、`payload` schema、补偿统计和稳定堆对照，以及仍不可独立恢复的历史证据。校准结果保存在[新参考原始样本](performance_baseline_2026-09-29/samples/equivalence_results.json)中；它们是 01 的新测量，不替换本报告中的 9 月 29 日历史倍率。
