# 性能计划 01：基线与等价对照验收记录

日期：2026-09-29。基于 `master` 的 `bb97baabb6f6626eef007a971007a47226106c78` 和含未提交改动的工作树。这里保存的是计划 01 的证据与校准结果，没有修改编译器、运行时或标准库的性能实现。

## 归档范围与复现

- `manifest.json` 记录快照时间、HEAD、工作树路径及已跟踪差异的 SHA-256、CPU/系统、工具链版本、编译器/标准库/clang 及基准源码哈希。快照时编译器 SHA-256 为 `58ff0459fcee33b82a2a887f73109c3a71143cd10135b6ac23edb32161c771d3`，标准库为 `ec5f0804addcca0997178d951ab0e6e5a108fc12481a38bf4994eca6bb3519c1`。
- 工作树和源码哈希是在归档时取得的；原复测时间点未保存完整的未提交补丁，不能据此证明当时每一个未提交源码字节都相同。编译器与静态库二进制哈希和原报告一致，因此新旧性能数据以产物哈希为主要身份标识。
- `samples/` 保存 9 月 27 日与 29 日现存原始 JSON、两组新旧程序同轮样本、TX/GCC 与 TX/clang 校准样本。各文件的源路径、大小与复制前后 SHA-256 见三个清单。用 `python -X utf8 -B scripts/archive_performance_baseline.py --verify` 校验哈希；本机可核对全部 48 个文件，克隆后会核对 37 个版本化文件并跳过未随仓库分发的 11 个本地二进制文件。
- `audit/` 是原 `tx_build/perf_audit_20260927/` 中经筛选的 `.tx`、C++、Python、Java 审计源码和本地服务脚本，以及原采样脚本。没有纳入 `.exe`、`.dll`、`.class`、`.ll` 或整个临时目录。原文件和副本逐个做过 SHA-256 比对。
- 原 2026-09-27 程序及归档时 `tx_build/` 中的 DLL 配套保存在忽略目录 `tx_build/performance_baseline_2026-09-29/legacy_bundle/`；其哈希见 `manifest.json`。`objdump` 显示两份旧程序的第三方依赖仅 `libgcc_s_seh-1.dll` 和 `libstdc++-6.dll`，两者在 `tx/`、`tx_build/` 和保留的 9 月 27 日审计目录中哈希相同。这个本地二进制包不会随 Git 克隆取得；缺少旧轮初次运行的加载路径记录，不能完全证明当时加载的就是这两份 DLL。

可从仓库根目录重建本项实际使用的 TX/C++ 负载并重新交替采样：

```powershell
python -X utf8 -B scripts/run_performance_equivalence.py
```

脚本只编译综合负载、归档中的 `features.tx` 和 `compute.tx`、两份 C++ 参考；分别预热一次，按 TX/C++、C++/TX 交替测 5 轮，逐轮核对固定校验值，并在测量前后核对工具链及源码哈希。输出是忽略目录 `tx_build/performance_equivalence/results_complete.json`。C++ 同时使用 GCC 13.1.0 `-std=c++23 -O3 -DNDEBUG` 和随 TX 分发的 clang 23.1.2 `-O3`；clang 对照明确使用本机 GCC 13.1 MinGW 头文件与链接组件。TX 由当前 `txc.exe` 以 `-O3` 生成。内置参数 `--rounds` 可指定 5、6 或 7 轮。若工具链已经变化，应先重建，再把结果作为新候选，不能冒充本轮快照。

通用新旧程序交替脚本为 `scripts/compare_performance_programs.py`，接收 `--old`、`--new`、`--source`、`--output` 和 5～7 轮设置。它记录程序与源码哈希、轮次顺序、每轮内部耗时和校验值；调用方须先确认两个程序由同一基准源码、输入与构建约定得到。`call_borrowing.tx` 和 `static_runtime.tx` 从 9 月 27 日提交 `9f6803a` 到本轮未变。本轮新旧对照用的命令及精确二进制哈希保存在 `samples/*same_session.json`。

完整四语言综合脚本 `scripts/run_diverse_performance.py` 仍可运行，更新后的输出路径为 `tx_build/diverse_performance_results_equivalent.json`，以免覆盖旧样本。本次没有重跑该全套；Python/Java 的 serde 等项目仍是各自原有简化参考，不应随 C++ 新对照一起解读为等价实现。

## 对照语义

| 负载 | 新 C++ 参考执行的契约 | 历史参考边界 |
| --- | --- | --- |
| `vector_*`、`map_*`、`dictionary_*` | 数据和键统一为 64 位整数；混合字典仍区分整数、文本与布尔键 | 历史综合 C++ 的容器数据及整数键为 32 位；表示成本仍不等同 TX 的动态字典 |
| `parse_valid`、`parse_invalid` | 去空白、可选正负号、2～36 进制、完整消费、64 位边界、错误类别及含字符串字段的完整结果；边界自检含 `INT64_MIN` | 历史综合 C++ 只用 `from_chars` 和成功标志。另列 `parse_core_*` 纯解析核心，只作分层参考，不与 TX 公开结果直接相除 |
| `serde_short_text`、`serde_long_text` | 对 `payload` 的 `$schema=1`、`id:int64`、`name:str` 做完整 JSON 往返；检查必填、字段类型、重复字段、未知字段和版本 | 历史综合 C++ 只拼接并查找固定 JSON 片段。新参考只覆盖本负载的 `payload` schema，不代表任意嵌套 schema 的完整通用成本 |
| `statistics_mean` | 与 TX 相同的有限值、计数/结果范围检查和 Neumaier 扩展精度补偿累计 | 历史 C++ 使用普通 `accumulate` |
| `heap_push_pop` | 64 位元素；同优先级按入堆序号稳定弹出，自检三项相同优先级的顺序 | 历史 C++ `priority_queue<int>` 没有稳定性规则 |

上述契约自检在采样构建后执行一次；耗时不计入基准。C++ 参考是针对这些固定负载的独立实现，仍有通用性与库表示差异。其余综合负载保留历史算法参考的语义标签，不因校验值相同就自动标记等价。

## 01 实测结果

下表取 `samples/equivalence_results.json` 的 5 轮内部计时中位数，单位毫秒；每轮耗时、校验值、程序及源码哈希均在 JSON 中。GCC 与 clang C++ 分别和同轮 TX 交替测量，不能把两列 C++ 结果当成同一次进程运行。

| 项目 | TX/GCC C++ | TX/clang C++ | 说明 |
| --- | ---: | ---: | --- |
| `map_hit_128` | 2.32 | 5.40 | 64 位键和值 |
| `map_hit_8192` | 2.80 | 4.79 | 64 位键和值 |
| `map_hit_10_percent` | 13.55 | 16.53 | 仅 10% 命中 |
| `dictionary_int_hit` | 10.99 | 9.79 | 混合键容器 |
| `dictionary_text_hit` | 7.12 | 7.29 | 混合键容器 |
| `parse_valid` | 45.18 | 82.73 | 公开结果；C++ 纯核心另为 0.394/0.486 ms |
| `parse_invalid` | 13.86 | 15.18 | 公开错误结果；C++ 纯核心另为 0.447/0.505 ms |
| `serde_short_text` | 20.82 | 20.63 | `payload` 完整 schema |
| `serde_long_text` | 8.87 | 8.97 | `payload` 完整 schema |
| `heap_push_pop` | 10.52 | 9.06 | 稳定、64 位 |
| `statistics_mean` | 3.49 | 2.29 | 补偿累计 |

同轮旧/新程序样本见 `samples/call_borrowing_same_session.json` 与 `samples/static_runtime_same_session.json`。5 轮中位数：`dictionary` 13.42 → 38.14 ms、`set` 14.54 → 38.81 ms、`queue` 4.89 → 11.99 ms、`cancel` 15.66 → 41.05 ms；`map` 4.26 → 3.38 ms。另一组 `format_text` 19.11 → 29.56 ms，而 `serde_json` 37.80 → 37.53 ms。两组每轮校验值相同，但旧 DLL 的上述来源限制仍适用。它们是回退定位证据，不是某一优化措施的收益。

## 测量口径与未覆盖证据

- 综合、审计与同轮对照 JSON 中的负载时间为程序内部计时，不含启动、编译及输出。归档 `diverse_performance_results_20260929.json` 另有进程墙钟、编译耗时和 2 ms 轮询的近似工作集峰值；本次定向校准没有新增内存采样。
- 9 月 29 日原始跨语言结果和旧轮 JSON 已归档；`language_features/run_compare.ps1` 当时只输出中位数，没有保存 7 轮原始逐项样本。因此 22 项语言特性的历史逐轮分布无法从现存材料恢复，报告表格仍只代表历史中位数。
- 外部题的原采样脚本已保存，但它依赖本仓库以外的 `E:\Project\problems` 源码、输入和答案；这些外部文件没有纳入本仓库，外部题不属于本项可独立重建的负载。
- 同轮比较保持固定源码、输入、校验值与轮次顺序。短项若分布使结论不稳，后续才在 TX 和 C++ 两边等量放大循环，并同时保留原负载；不自动重跑全套。CPU 频率、负载或样本异常要记入报告。`heap_push_pop` 的 clang 样本有单个偏高值，中位数可读，但不据此推断亚毫秒级收益。

阶段目标：简单查询与静态调用在**等价** C++ 对照下低于 5 倍，并同时报告绝对耗时及分布。恢复 9 月 27 日旧值只证明回退缩小。解析、serde、GC 等复杂契约先以完整对照和语义保持为准；未完成等价校准的历史简化参考不套用 5 倍目标。目前 `dictionary_*`、`map_hit_10_percent` 与解析等仍超目标，本记录不表示这些热点已经优化。
