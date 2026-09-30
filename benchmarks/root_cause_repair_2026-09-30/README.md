# 根因修复定向复测

使用原始 features/diverse/concurrency 源码重新编译，和上一轮静态执行归档目录下
留存的旧二进制交替执行。每组预热 1 轮，记录 3 轮，逐轮核对全部负载校验值。
两边均为 ThinLTO；旧程序使用其内嵌的旧运行时，新程序链接本次重建的运行时。

这是一轮定向复测，不替代全量性能报告。亚毫秒结果存在调度扰动；serde 的改善
也只代表本次配对，不能据此把历史跨轮变化全部归因于特化选择。

| 项目 | 修复前中位数 ms | 修复后中位数 ms | 前/后倍率 |
|---|---:|---:|---:|
| heap_push_pop | 29.804 | 7.934 | 3.76 |
| iterator_snapshot | 1.468 | 0.161 | 9.12 |
| mutex_uncontended | 19.826 | 1.393 | 14.23 |
| serde_short_text | 6.901 | 6.148 | 1.12 |
| format_literal | 6.238 | 0.586 | 10.65 |
| encoding_literal | 11.220 | 1.045 | 10.74 |
| encoding_dynamic | 10.931 | 1.035 | 10.56 |

`encoding_dynamic` 是原基准名称；当前源码中的 codec 可被证明为常量，因此使用
已知编码路径。不能用此项声称任意运行时动态编码名称都得到相同收益。

原始样本、校验值、源码/新旧程序哈希及采样编译器哈希见 [results.json](results.json)。
复现入口为 `scripts/measure_root_cause_repair.py`，依赖本机留存旧二进制。
修复内容和保守边界见 `docs/root_cause_repair.md`；原根因报告保留不改。

首批样本采集之后补齐了 sum 通用表示的保守分配/GC 效果标记和一处编译警告，
随后完成下面的第二批优化。各组采样哈希对应各自采样时版本，不能混作一次全量报告。

## 对象、标准库与外部题

第二批使用 1 轮预热、3 轮交替采样并核对校验值，详见
[remaining_results.json](remaining_results.json)。基线程序路径与哈希逐组记录；
其中 language 组使用留存的语言特性程序，其余组使用脚本指定的历史程序。

| 项目 | 修复前中位数 ms | 修复后中位数 ms |
|---|---:|---:|
| deinit | 14.786 | 10.350 |
| deep_copy | 25.441 | 19.089 |
| copy_cycle | 9.148 | 8.311 |
| cycle_gc | 1.442 | 1.372 |
| 同契约 graph_copy | 7.260 | 4.887 |
| 同契约 graph_gc | 10.851 | 7.501 |
| string_conversion | 16.414 | 10.911 |
| dictionary_int_hit | 1.274 | 0.884 |
| dictionary_text_hit | 2.340 | 2.278 |
| dictionary_mostly_miss | 0.867 | 0.531 |
| test_parameterized | 0.205 | 0.190 |
| test_property | 0.340 | 0.304 |
| sqlite_pool | 43.235 | 1.664 |
| sqlite_async | 52.002 | 3.409 |
| random_long | 36.000 | 24.000 |

外部题使用原始计时解和正式答案，每题选择五组输入，全部答案一致。
mini-filesystem 的五组中位数分别为 11.704→11.130、12.572→12.438、
31.966→30.791、5.587→5.807、10.776→10.064 ms；
not-yet-on-stage 为 0.997→0.997、2.636→2.631、1.001→1.013、
4.273→4.261、3.183→3.184 ms。部分持平或略慢，不能声称外部题普遍显著提速。
源码、输入、答案和程序哈希见 [external_results.json](external_results.json)。

## TLS 配置选择与最终包

第二批 TLS 初测为 92.109→100.039 ms，未显示改善，因此继续做配置对照。
[tls_selection.json](tls_selection.json) 记录旧版、Everest 候选、关闭 Everest 的新版
交替运行的 5 轮样本（另有 1 轮预热），中位数依次为 136.189、102.664、119.813 ms。
该文件中的 candidate 指关闭 Everest 的对照程序，不是最终交付配置。

最终恢复 `MBEDTLS_ECDH_VARIANT_EVEREST_ENABLED`，并重新构建原生与 ThinLTO 库。
`scripts/measure_tls_repair.py --final` 单独记录旧版与最终包的 5 轮配对，
中位数为 110.485→105.933 ms，约降低 4.1%。原始样本、配置和编译器哈希见
[tls_final.json](tls_final.json)。网络样本波动较大；这些结果不证明固定提速比例，
也不代表 Mbed TLS 与 OpenSSL 的后端差距已消除。

最终构建后重新通过 `check_root_cause_lifecycle.py`、`check_tls_stream.py`、
`check_root_cause_repair.py`：覆盖图共享与复活、局部类清理、SQLite 旧租约失效、
TLS 身份独占与关闭、正常/拒绝握手及随机数线程退出、首批优化的语义和 IR。
没有重跑全量性能套件。当前交付覆盖根因报告中明确的缺陷和已实施的优化，
不将参考程序的语义差异或剩余性能差距表述成已消除。
