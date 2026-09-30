# 根因修复工作区全量性能测试结果

基准 HEAD：`5aa6d0244092df1e52b0aa49f3e22083834b2ce6`。测试包含未提交根因修复的当前工作区；源码指纹见 build_manifest.json。正式重建后串行采样。
采样时间（北京时间）：2026-10-01 00:14:22 至 2026-10-01 00:25:39，共 11.28 分钟。
52/52 个公开模块有代表性能负载。新增标准库 29 项均为四语言各 5 轮；原有套件维持原轮数，包含语言、库、网络、外部题、综合、启动/编译/内存、优化专项及契约对照。
两道外部题全部 89 组正式答案核对通过；两套完成记录均确认采样期间源码与工具链指纹不变。

综合程序墙钟中位数：TX **70.682 ms**，C++ **70.647 ms**，倍率 **1.00×**。这不是全库平均倍率。

全部参考语言共有 43 条 ≥3× 记录，按名称去重 34 项；其中 C++ 对照 31 条、按名称去重 **27 项**。

## 对 C++ 达到 3× 的全部项目

同名有多组采样时，下表展示最高倍率及其对应套件，避免重复计数；主测与 GCC/clang 配对轮的完整记录见全部清单。临界值不能视为稳定超过阈值。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 可比边界 |
| --- | --- | --- | --- | --- | --- | --- |
| 语言特性 | cycle_gc | C++ | 1.449000 | 0.073900 | 19.61× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 语言特性 | deinit | C++ | 11.408000 | 0.733200 | 15.56× |  |
| 语言特性 | deep_copy | C++ | 20.827000 | 2.343400 | 8.89× | 参考只复制已知形状；通用图复制另见新增契约 |
| 语言特性 | variadic_unpack | C++ | 11.646000 | 1.639000 | 7.11× | 参考不执行 TX 动态命名实参绑定 |
| 外部题/mini-filesystem | fanout-2000 | C++ | 12.817000 | 1.826000 | 7.02× |  |
| 综合 | serde_short_text | C++ | 6.436000 | 0.940000 | 6.85× |  |
| 新增/diagnostics | test_property | C++ | 0.304000 | 0.046100 | 6.59× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 外部题/mini-filesystem | random-2000-1 | C++ | 5.999000 | 0.971000 | 6.18× |  |
| 契约/format_contract | format_alternating | C++ | 17.156000 | 3.237000 | 5.30× |  |
| 契约/format_contract | format_parameter | C++ | 17.288000 | 3.264000 | 5.30× |  |
| 外部题/mini-filesystem | moves-2000 | C++ | 5.232000 | 1.006000 | 5.20× |  |
| audit/compute | bytes_hex | C++ | 8.792000 | 1.832000 | 4.80× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.583000 | 0.348000 | 4.55× |  |
| 语言特性 | copy_cycle | C++ | 8.561000 | 1.919800 | 4.46× |  |
| 校准/diverse_clang | vector_scan_100k | clang C++ | 3.554000 | 0.809000 | 4.39× |  |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.256000 | 0.064000 | 4.00× |  |
| 语言特性 | class_methods | C++ | 0.652000 | 0.163100 | 4.00× |  |
| audit/features | vector_push | C++ | 1.942000 | 0.500000 | 3.88× |  |
| library/组合 | string | C++ | 10.000000 | 2.586900 | 3.87× | 整数毫秒短项，倍率精度有限 |
| 新增/concurrency | mutex_uncontended | C++ | 1.383000 | 0.374500 | 3.69× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.006000 | 0.556000 | 3.61× |  |
| audit/features | queue_push_pop | C++ | 1.165000 | 0.324000 | 3.60× |  |
| 语言特性 | string_conversion | C++ | 11.303000 | 3.180800 | 3.55× |  |
| audit/features | iterator_snapshot | C++ | 0.164000 | 0.047000 | 3.49× |  |
| 新增/diagnostics | test_parameterized | C++ | 0.190000 | 0.056900 | 3.34× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| audit/compute | random_int | C++ | 0.998000 | 0.327000 | 3.05× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.353000 | 1.442000 | 3.02× |  |

## 完整数据

- [原有套件逐项报告](README.md)
- [新增标准库四语言逐项报告](stdlib/README.md)
- [全部 ≥3× 记录及语义边界](all_gaps.md)
- [52 模块覆盖映射](coverage.md)
- [与上一轮 TX 耗时比较](comparison.md)

通用图复制、确定性析构、动态实参绑定及 guard 生命周期的成本不能由简化 C++ 参考完全分离。
HTTP 为客户端端到端对照，数据库跨语言驱动及缓存策略不同；短项和网络测量受调度扰动，本轮不据此直接断言根因。
本轮只新增测试入口与归档，保留原有未提交实现；未修改编译器或标准库实现。

## 构建与解释边界

首次 CMake 配置自动混用了 clang C 与 GCC C++，在编译前失败；记录见 build.log。随后原生构建完成，但发行目录缺少配套 LLVM 工具，封包停止，见 build_retry.log。显式指定配套 GCC 与完整 LLVM 目录后继续正式构建，成功日志见 build_completed.json 的 build_log 字段。失败配置保留在 tx_build/root_cause_configure_failure；成功的 build/ 已清理。
当前工作区未提交；build_manifest.json 保存基准 HEAD、初始 Git 状态和全部实现/构建输入哈希。构建前后与两套采样期间均检查输入稳定。默认 TX 使用原有 -O3、ThinLTO 和 ld.lld。
两套实际执行时间合计 9.14 分钟，中间续跑间隔 2.15 分钟。原有套件完成后，标准库在开始采样前触发历史目录防覆盖检查；随后只调整归档入口并续跑 29 项，没有重跑或覆盖已完成样本。见 stdlib_resume.json、run.log 和 stdlib_run.log。
run_core_snapshot.py 保留原有套件执行时的入口代码，其哈希与原 manifest 相符；两套采样的共同实现、配置和工具链哈希一致，最终源码仍匹配正式构建。统一完成核验见 completed_all.json。
格式化和 UTF-8 部分原始负载只消费结果长度，当前编译器可依法消除完整结果物化；名称包含 dynamic 的不可变编码名/局部模板仍可静态特化。不能据此推断任意动态模板、编码名或完整输出内容的成本。
SQLite 池与异步只读查询现在可复用安全连接；C++ 同后端同步获得此行为，而 Java/Python 参考仍关闭连接。跨语言端到端比例保留该生命周期差异。
全量指仓库现有全部性能套件及 52 个公开模块的代表负载，不表示每个 API、失败路径或生产压力全覆盖。内存为 2 ms 进程树工作集采样峰值。
复现：新归档目录先执行 run.py --build-only，再执行 run.py，最后 summarize.py；本目录拒绝覆盖已有正式记录。依赖仓库既有归档源码、本机固定参考和 E:/Project/problems 正式数据。

## 本轮全部 TX 耗时变化

基线为 performance_retest_2026-09-30_static_execution 及其配套标准库全量采样。
单位 ms；负百分比为变快，跨轮机器状态不同，短项、文件和网络负载会受调度扰动。

| 套件 | 项目 | 上轮 TX ms | 本轮 TX ms | 变化 |
| --- | --- | --- | --- | --- |
| 语言特性 | scalar_control | 0.168000 | 0.257000 | +53.0% |
| 语言特性 | updates | 1.417000 | 1.418000 | +0.1% |
| 语言特性 | while_logic | 0.177000 | 0.177000 | +0.0% |
| 语言特性 | float_arithmetic | 0.355000 | 0.355000 | +0.0% |
| 语言特性 | overloads | 0.188000 | 0.191000 | +1.6% |
| 语言特性 | named_arguments | 0.404000 | 0.401000 | -0.7% |
| 语言特性 | recursion | 0.035000 | 0.035000 | +0.0% |
| 语言特性 | variadic_unpack | 11.229000 | 11.646000 | +3.7% |
| 语言特性 | array_destructure | 0.010000 | 0.010000 | +0.0% |
| 语言特性 | array_padded | 0.024000 | 0.024000 | +0.0% |
| 语言特性 | dict_iteration | 7.313000 | 7.179000 | -1.8% |
| 语言特性 | struct_operators | 0.025000 | 0.025000 | +0.0% |
| 语言特性 | class_methods | 0.662000 | 0.652000 | -1.5% |
| 语言特性 | virtual_interface | 0.904000 | 0.969000 | +7.2% |
| 语言特性 | class_operator | 0.598000 | 0.612000 | +2.3% |
| 语言特性 | runtime_cast | 0.236000 | 0.242000 | +2.5% |
| 语言特性 | module_call | 0.024000 | 0.025000 | +4.2% |
| 语言特性 | string_conversion | 16.576000 | 11.303000 | -31.8% |
| 语言特性 | deep_copy | 25.082000 | 20.827000 | -17.0% |
| 语言特性 | copy_cycle | 9.302000 | 8.561000 | -8.0% |
| 语言特性 | deinit | 15.378000 | 11.408000 | -25.8% |
| 语言特性 | cycle_gc | 1.542000 | 1.449000 | -6.0% |
| library/组合 | math | 4.000000 | 4.000000 | +0.0% |
| library/组合 | random | 1.000000 | 1.000000 | +0.0% |
| library/组合 | string | 9.000000 | 10.000000 | +11.1% |
| library/组合 | array | 18.000000 | 18.000000 | +0.0% |
| library/组合 | dict | 3.000000 | 1.000000 | -66.7% |
| library/组合 | path | 21.000000 | 22.000000 | +4.8% |
| library/组合 | fs | 43.000000 | 44.000000 | +2.3% |
| library/组合 | file | 1317.000000 | 1298.000000 | -1.4% |
| library/组合 | io | 9.000000 | 8.000000 | -11.1% |
| library/组合 | time | 4.000000 | 3.000000 | -25.0% |
| audit/compute | algorithm_sort | 5.469000 | 5.892000 | +7.7% |
| audit/compute | bytes_hex | 8.617000 | 8.792000 | +2.0% |
| audit/compute | cancel_status | 0.573000 | 0.553000 | -3.5% |
| audit/compute | cbor_roundtrip | 22.235000 | 22.862000 | +2.8% |
| audit/compute | crypto_sha256 | 8.852000 | 8.804000 | -0.5% |
| audit/compute | csv_parse | 24.532000 | 24.769000 | +1.0% |
| audit/compute | decimal_add | 10.978000 | 11.036000 | +0.5% |
| audit/compute | dictionary_contains | 0.362000 | 0.287000 | -20.7% |
| audit/compute | encoding_utf8 | 4.330000 | 0.407000 | -90.6% |
| audit/compute | env_get | 9.844000 | 10.014000 | +1.7% |
| audit/compute | format_text | 1.231000 | 0.115000 | -90.7% |
| audit/compute | json_parse | 23.194000 | 22.615000 | -2.5% |
| audit/compute | math_sqrt | 4.153000 | 4.109000 | -1.1% |
| audit/compute | parse_int | 11.408000 | 11.754000 | +3.0% |
| audit/compute | random_int | 1.390000 | 0.998000 | -28.2% |
| audit/compute | regex_search | 12.651000 | 12.787000 | +1.1% |
| audit/compute | serde_json | 13.898000 | 12.420000 | -10.6% |
| audit/compute | statistics_mean | 4.457000 | 4.540000 | +1.9% |
| audit/compute | test_assert | 7.654000 | 8.307000 | +8.5% |
| audit/compute | unicode_nfc | 5.347000 | 5.500000 | +2.9% |
| audit/compute | xml_parse | 32.989000 | 34.180000 | +3.6% |
| audit/features | vector_push | 1.938000 | 1.942000 | +0.2% |
| audit/features | vector_index | 0.146000 | 0.175000 | +19.9% |
| audit/features | map_lookup | 0.325000 | 0.327000 | +0.6% |
| audit/features | set_contains | 0.259000 | 0.258000 | -0.4% |
| audit/features | heap_push_pop | 29.891000 | 7.475000 | -75.0% |
| audit/features | queue_push_pop | 1.119000 | 1.165000 | +4.1% |
| audit/features | iterator_snapshot | 1.468000 | 0.164000 | -88.8% |
| audit/features | option_value | 0.025000 | 0.025000 | +0.0% |
| audit/features | function_value | 0.286000 | 0.282000 | -1.4% |
| audit/features | closure_bind | 0.190000 | 0.171000 | -10.0% |
| audit/system | debug_location | 5.391000 | 5.110000 | -5.2% |
| audit/system | error_stack | 1.953000 | 1.877000 | -3.9% |
| audit/system | file_stream_rw | 843.336000 | 850.519000 | +0.9% |
| audit/system | log_event | 7.738000 | 7.945000 | +2.7% |
| audit/system | process_spawn | 907.729000 | 913.913000 | +0.7% |
| audit/system | system_os | 2.363000 | 2.478000 | +4.9% |
| network/http | httpx_get | 975.974000 | 962.718000 | -1.4% |
| network/http | requests_get | 981.354000 | 977.079000 | -0.4% |
| network/websocket | websocket_echo | 894.903000 | 1003.761000 | +12.2% |
| 综合 | vector_scan_1k | 0.262000 | 0.261000 | -0.4% |
| 综合 | vector_scan_100k | 2.479000 | 3.584000 | +44.6% |
| 综合 | vector_index_sequential | 0.301000 | 0.297000 | -1.3% |
| 综合 | vector_index_strided | 0.298000 | 0.295000 | -1.0% |
| 综合 | map_hit_128 | 1.606000 | 1.581000 | -1.6% |
| 综合 | map_hit_8192 | 2.215000 | 2.262000 | +2.1% |
| 综合 | map_hit_10_percent | 2.522000 | 1.967000 | -22.0% |
| 综合 | dictionary_int_hit | 2.364000 | 0.879000 | -62.8% |
| 综合 | dictionary_text_hit | 3.157000 | 2.324000 | -26.4% |
| 综合 | dictionary_mostly_miss | 0.845000 | 0.544000 | -35.6% |
| 综合 | format_literal | 6.574000 | 0.591000 | -91.0% |
| 综合 | format_dynamic | 6.923000 | 0.564000 | -91.9% |
| 综合 | encoding_literal | 12.491000 | 1.086000 | -91.3% |
| 综合 | encoding_dynamic | 11.221000 | 1.020000 | -90.9% |
| 综合 | serde_short_text | 9.825000 | 6.436000 | -34.5% |
| 综合 | serde_long_text | 8.712000 | 8.192000 | -6.0% |
| 综合 | parse_valid | 0.830000 | 0.758000 | -8.7% |
| 综合 | parse_invalid | 0.745000 | 0.755000 | +1.3% |
| 校准/diverse | vector_scan_1k | 0.260000 | 0.255000 | -1.9% |
| 校准/diverse_clang | vector_scan_1k | 0.261000 | 0.256000 | -1.9% |
| 校准/diverse | vector_scan_100k | 2.416000 | 3.524000 | +45.9% |
| 校准/diverse_clang | vector_scan_100k | 2.434000 | 3.554000 | +46.0% |
| 校准/diverse | vector_index_sequential | 0.294000 | 0.292000 | -0.7% |
| 校准/diverse_clang | vector_index_sequential | 0.295000 | 0.293000 | -0.7% |
| 校准/diverse | vector_index_strided | 0.293000 | 0.292000 | -0.3% |
| 校准/diverse_clang | vector_index_strided | 0.299000 | 0.293000 | -2.0% |
| 校准/diverse | map_hit_128 | 1.518000 | 1.516000 | -0.1% |
| 校准/diverse_clang | map_hit_128 | 1.552000 | 1.583000 | +2.0% |
| 校准/diverse | map_hit_8192 | 2.008000 | 2.020000 | +0.6% |
| 校准/diverse_clang | map_hit_8192 | 2.015000 | 2.006000 | -0.4% |
| 校准/diverse | map_hit_10_percent | 1.898000 | 1.932000 | +1.8% |
| 校准/diverse_clang | map_hit_10_percent | 2.039000 | 1.959000 | -3.9% |
| 校准/diverse | dictionary_int_hit | 1.328000 | 0.874000 | -34.2% |
| 校准/diverse_clang | dictionary_int_hit | 1.289000 | 0.918000 | -28.8% |
| 校准/diverse | dictionary_text_hit | 2.239000 | 2.249000 | +0.4% |
| 校准/diverse_clang | dictionary_text_hit | 2.281000 | 2.317000 | +1.6% |
| 校准/diverse | dictionary_mostly_miss | 0.838000 | 0.538000 | -35.8% |
| 校准/diverse_clang | dictionary_mostly_miss | 0.833000 | 0.539000 | -35.3% |
| 校准/diverse | format_literal | 6.306000 | 0.567000 | -91.0% |
| 校准/diverse_clang | format_literal | 6.346000 | 0.571000 | -91.0% |
| 校准/diverse | format_dynamic | 6.219000 | 0.568000 | -90.9% |
| 校准/diverse_clang | format_dynamic | 6.229000 | 0.561000 | -91.0% |
| 校准/diverse | encoding_literal | 11.582000 | 1.076000 | -90.7% |
| 校准/diverse_clang | encoding_literal | 11.720000 | 1.075000 | -90.8% |
| 校准/diverse | encoding_dynamic | 11.300000 | 1.010000 | -91.1% |
| 校准/diverse_clang | encoding_dynamic | 11.079000 | 1.007000 | -90.9% |
| 校准/diverse | serde_short_text | 6.943000 | 6.050000 | -12.9% |
| 校准/diverse_clang | serde_short_text | 7.818000 | 6.246000 | -20.1% |
| 校准/diverse | serde_long_text | 8.432000 | 7.882000 | -6.5% |
| 校准/diverse_clang | serde_long_text | 8.588000 | 7.939000 | -7.6% |
| 校准/diverse | parse_valid | 0.764000 | 0.744000 | -2.6% |
| 校准/diverse_clang | parse_valid | 0.781000 | 0.730000 | -6.5% |
| 校准/diverse | parse_invalid | 0.727000 | 0.741000 | +1.9% |
| 校准/diverse_clang | parse_invalid | 0.747000 | 0.739000 | -1.1% |
| 校准/heap | heap_push_pop | 29.625000 | 7.519000 | -74.6% |
| 校准/heap_clang | heap_push_pop | 34.858000 | 7.447000 | -78.6% |
| 校准/statistics | statistics_mean | 4.520000 | 4.439000 | -1.8% |
| 校准/statistics_clang | statistics_mean | 4.485000 | 4.398000 | -1.9% |
| 外部题/mini-filesystem | fanout-2000 | 11.182000 | 12.817000 | +14.6% |
| 外部题/mini-filesystem | deep-pwd-2000 | 12.568000 | 11.564000 | -8.0% |
| 外部题/mini-filesystem | moves-2000 | 5.442000 | 5.232000 | -3.9% |
| 外部题/mini-filesystem | random-2000-1 | 6.399000 | 5.999000 | -6.3% |
| 外部题/mini-filesystem | linklong-2000 | 32.177000 | 31.150000 | -3.2% |
| 外部题/not-yet-on-stage | all-free-max | 0.993000 | 1.025000 | +3.2% |
| 外部题/not-yet-on-stage | forced-increasing-max | 4.299000 | 4.353000 | +1.3% |
| 外部题/not-yet-on-stage | forced-decreasing-max | 1.010000 | 0.996000 | -1.4% |
| 外部题/not-yet-on-stage | shuffled-tight-max-1 | 3.280000 | 3.178000 | -3.1% |
| 外部题/not-yet-on-stage | alternating-tight-max | 2.701000 | 2.677000 | -0.9% |
| 专项/borrowing | dictionary | 3.298000 | 2.113000 | -35.9% |
| 专项/borrowing | map | 2.878000 | 3.542000 | +23.1% |
| 专项/borrowing | set | 2.550000 | 2.487000 | -2.5% |
| 专项/borrowing | queue | 1.258000 | 1.266000 | +0.6% |
| 专项/borrowing | cancel | 5.504000 | 5.617000 | +2.1% |
| 专项/static_runtime | vector_foreach | 1.263000 | 1.300000 | +2.9% |
| 专项/static_runtime | serde_json | 13.982000 | 12.896000 | -7.8% |
| 专项/static_runtime | format_text | 13.734000 | 1.416000 | -89.7% |
| 专项/static_runtime | statistics_mean | 10.430000 | 10.029000 | -3.8% |
| 专项/static_runtime | encoding_utf8 | 22.987000 | 2.331000 | -89.9% |
| 专项/parse_paths | local_valid | 0.766000 | 0.776000 | +1.3% |
| 专项/parse_paths | local_invalid | 0.750000 | 0.754000 | +0.5% |
| 专项/parse_paths | full_valid | 0.878000 | 0.857000 | -2.4% |
| 专项/parse_paths | full_invalid | 3.241000 | 3.286000 | +1.4% |
| 专项/format_paths | format_literal | 7.315000 | 0.553000 | -92.4% |
| 专项/format_paths | format_local | 7.316000 | 0.554000 | -92.4% |
| 专项/format_paths | format_dynamic | 19.273000 | 18.514000 | -3.9% |
| 专项/serde_paths | serde_json_short | 7.210000 | 6.456000 | -10.5% |
| 专项/serde_paths | serde_cbor_short | 4.732000 | 4.222000 | -10.8% |
| 专项/serde_paths | serde_json_long | 8.425000 | 7.745000 | -8.1% |
| 专项/serde_paths | serde_cbor_long | 8.341000 | 7.998000 | -4.1% |
| 专项/paths_09_11 | struct_fields | 0.662000 | 0.670000 | +1.2% |
| 专项/paths_09_11 | class_methods | 1.630000 | 1.641000 | +0.7% |
| 专项/paths_09_11 | module_call | 0.024000 | 0.024000 | +0.0% |
| 专项/paths_09_11 | function_value | 2.810000 | 2.842000 | +1.1% |
| 专项/paths_09_11 | closure_bind | 1.645000 | 1.661000 | +1.0% |
| 专项/paths_09_11 | heap_push_pop | 29.949000 | 7.713000 | -74.2% |
| 专项/paths_12_14 | snapshot | 14.787000 | 1.851000 | -87.5% |
| 专项/paths_12_14 | vector_1k | 0.240000 | 0.240000 | +0.0% |
| 专项/paths_12_14 | vector_100k | 0.297000 | 0.278000 | -6.4% |
| 专项/paths_12_14 | division | 0.398000 | 0.391000 | -1.8% |
| 专项/paths_12_14 | recursion | 0.001000 | 0.000000 | -100.0% |
| 专项/paths_12_14 | static_spread | 19.563000 | 20.174000 | +3.1% |
| 专项/paths_12_14 | dynamic_spread | 19.422000 | 20.867000 | +7.4% |
| 专项/paths_12_14 | deep_copy | 78.753000 | 56.580000 | -28.2% |
| 专项/paths_15_16 | hex_encode_short | 18.549000 | 16.678000 | -10.1% |
| 专项/paths_15_16 | hex_decode_short | 31.879000 | 28.584000 | -10.3% |
| 专项/paths_15_16 | hex_encode_large | 4.566000 | 3.775000 | -17.3% |
| 专项/paths_15_16 | hex_decode_large | 6.752000 | 5.919000 | -12.3% |
| 专项/paths_15_16 | utf8_encode_short | 24.570000 | 1.986000 | -91.9% |
| 专项/paths_15_16 | utf8_decode_short | 14.067000 | 3.608000 | -74.4% |
| 专项/paths_15_16 | utf8_encode_large | 16.209000 | 14.501000 | -10.5% |
| 专项/paths_15_16 | utf8_decode_large | 32.357000 | 29.255000 | -9.6% |
| 专项/paths_15_16 | mean_short | 5.125000 | 4.509000 | -12.0% |
| 专项/paths_15_16 | mean_large | 25.333000 | 22.028000 | -13.0% |
| 专项/file_io | file_stream_rw | 888.247000 | 864.641000 | -2.7% |
| 专项 | random_long | 41.000000 | 25.000000 | -39.0% |
| 契约/format_contract | format_alternating | 17.629000 | 17.156000 | -2.7% |
| 契约/format_contract | format_parameter | 17.913000 | 17.288000 | -3.5% |
| 契约/graph_contract | graph_copy | 7.334000 | 4.798000 | -34.6% |
| 契约/graph_contract | graph_gc | 10.423000 | 7.531000 | -27.7% |
| 进程运行 | TX | 122.547100 | 70.681900 | -42.3% |
| 空程序启动 | TX | 46.547500 | 28.621600 | -38.5% |
| 编译 | TX check | 379.139000 | 355.172900 | -6.3% |
| 编译 | TX full | 1691.320200 | 1538.550200 | -9.0% |
| 新增/concurrency | thread_spawn_join | 13.854000 | 13.873000 | +0.1% |
| 新增/concurrency | mutex_uncontended | 20.517000 | 1.383000 | -93.3% |
| 新增/concurrency | atomic_add | 0.384000 | 0.397000 | +3.4% |
| 新增/concurrency | channel_send_recv | 1.978000 | 1.935000 | -2.2% |
| 新增/concurrency | task_spawn_wait | 7.506000 | 7.271000 | -3.1% |
| 新增/sqlite | sqlite_insert | 11.422000 | 11.867000 | +3.9% |
| 新增/sqlite | sqlite_read | 26.133000 | 24.775000 | -5.2% |
| 新增/sqlite | sqlite_savepoint | 2.622000 | 2.827000 | +7.8% |
| 新增/sqlite | sqlite_pool | 137.792000 | 1.691000 | -98.8% |
| 新增/sqlite | sqlite_async | 54.773000 | 3.340000 | -93.9% |
| 新增/sqlite | migration_recheck | 2.117000 | 2.056000 | -2.9% |
| 新增/postgres | postgres_insert | 189.458000 | 185.869000 | -1.9% |
| 新增/postgres | postgres_read | 41.141000 | 38.771000 | -5.8% |
| 新增/postgres | postgres_savepoint | 115.403000 | 105.332000 | -8.7% |
| 新增/security | secret_equal | 14.163000 | 14.277000 | +0.8% |
| 新增/security | argon2_hash_verify | 163.263000 | 150.038000 | -8.1% |
| 新增/security | ed25519_sign | 17.126000 | 17.763000 | +3.7% |
| 新增/security | ed25519_verify | 24.329000 | 25.124000 | +3.3% |
| 新增/security | x509_parse_der | 2.965000 | 2.891000 | -2.5% |
| 新增/network | dns_localhost | 0.266000 | 0.284000 | +6.8% |
| 新增/network | udp_echo | 34.354000 | 34.740000 | +1.1% |
| 新增/network | ipc_echo | 12.771000 | 13.990000 | +9.5% |
| 新增/network | tls_handshake | 144.254000 | 65.634000 | -54.5% |
| 新增/async_file | async_file_rw | 38.012000 | 40.389000 | +6.3% |
| 新增/diagnostics | test_parameterized | 0.223000 | 0.190000 | -14.8% |
| 新增/diagnostics | test_property | 0.334000 | 0.304000 | -9.0% |
| 新增/diagnostics | log_filtered | 0.901000 | 0.838000 | -7.0% |
| 新增/diagnostics | log_file | 31.396000 | 31.382000 | -0.0% |
| 新增/profile | profile_spans | 0.471000 | 0.455000 | -3.4% |


## 全部有效跨语言对照

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 可比边界 |
| --- | --- | --- | --- | --- | --- | --- |
| 语言特性 | scalar_control | C++ | 0.257000 | 0.235800 | 1.09× |  |
| 语言特性 | scalar_control | Python | 0.257000 | 24.360800 | 0.01× |  |
| 语言特性 | scalar_control | Java | 0.257000 | 3.000100 | 0.09× |  |
| 语言特性 | updates | C++ | 1.418000 | 1.304300 | 1.09× |  |
| 语言特性 | updates | Python | 1.418000 | 64.536000 | 0.02× |  |
| 语言特性 | updates | Java | 1.418000 | 5.607900 | 0.25× |  |
| 语言特性 | while_logic | C++ | 0.177000 | 0.175900 | 1.01× |  |
| 语言特性 | while_logic | Python | 0.177000 | 31.584000 | 0.01× |  |
| 语言特性 | while_logic | Java | 0.177000 | 2.398300 | 0.07× |  |
| 语言特性 | float_arithmetic | C++ | 0.355000 | 0.352000 | 1.01× |  |
| 语言特性 | float_arithmetic | Python | 0.355000 | 26.230400 | 0.01× |  |
| 语言特性 | float_arithmetic | Java | 0.355000 | 1.738500 | 0.20× |  |
| 语言特性 | overloads | C++ | 0.191000 | 0.188600 | 1.01× |  |
| 语言特性 | overloads | Python | 0.191000 | 19.875800 | 0.01× |  |
| 语言特性 | overloads | Java | 0.191000 | 3.914600 | 0.05× |  |
| 语言特性 | named_arguments | C++ | 0.401000 | 0.377200 | 1.06× |  |
| 语言特性 | named_arguments | Python | 0.401000 | 27.187200 | 0.01× |  |
| 语言特性 | named_arguments | Java | 0.401000 | 3.229300 | 0.12× |  |
| 语言特性 | recursion | C++ | 0.035000 | 0.067700 | 0.52× |  |
| 语言特性 | recursion | Python | 0.035000 | 12.430200 | 0.00× |  |
| 语言特性 | recursion | Java | 0.035000 | 1.100200 | 0.03× |  |
| 语言特性 | variadic_unpack | C++ | 11.646000 | 1.639000 | 7.11× | 参考不执行 TX 动态命名实参绑定 |
| 语言特性 | variadic_unpack | Python | 11.646000 | 3.589900 | 3.24× |  |
| 语言特性 | variadic_unpack | Java | 11.646000 | 0.928100 | 12.55× | 参考不执行 TX 动态命名实参绑定 |
| 语言特性 | array_destructure | C++ | 0.010000 | 1.109200 | 0.01× |  |
| 语言特性 | array_destructure | Python | 0.010000 | 4.187400 | 0.00× |  |
| 语言特性 | array_destructure | Java | 0.010000 | 5.009200 | 0.00× |  |
| 语言特性 | array_padded | C++ | 0.024000 | 2.196100 | 0.01× |  |
| 语言特性 | array_padded | Python | 0.024000 | 7.602400 | 0.00× |  |
| 语言特性 | array_padded | Java | 0.024000 | 9.348400 | 0.00× |  |
| 语言特性 | dict_iteration | C++ | 7.179000 | 4.458600 | 1.61× |  |
| 语言特性 | dict_iteration | Python | 7.179000 | 21.223500 | 0.34× |  |
| 语言特性 | dict_iteration | Java | 7.179000 | 53.874900 | 0.13× |  |
| 语言特性 | struct_operators | C++ | 0.025000 | 13.402800 | 0.00× |  |
| 语言特性 | struct_operators | Python | 0.025000 | 88.356900 | 0.00× |  |
| 语言特性 | struct_operators | Java | 0.025000 | 13.939800 | 0.00× |  |
| 语言特性 | class_methods | C++ | 0.652000 | 0.163100 | 4.00× |  |
| 语言特性 | class_methods | Python | 0.652000 | 47.264200 | 0.01× |  |
| 语言特性 | class_methods | Java | 0.652000 | 5.239800 | 0.12× |  |
| 语言特性 | virtual_interface | C++ | 0.969000 | 0.442800 | 2.19× |  |
| 语言特性 | virtual_interface | Python | 0.969000 | 41.728800 | 0.02× |  |
| 语言特性 | virtual_interface | Java | 0.969000 | 4.284000 | 0.23× |  |
| 语言特性 | class_operator | C++ | 0.612000 | 0.245000 | 2.50× |  |
| 语言特性 | class_operator | Python | 0.612000 | 17.409600 | 0.04× |  |
| 语言特性 | class_operator | Java | 0.612000 | 2.468600 | 0.25× |  |
| 语言特性 | runtime_cast | C++ | 0.242000 | 1.904900 | 0.13× |  |
| 语言特性 | runtime_cast | Python | 0.242000 | 13.180700 | 0.02× |  |
| 语言特性 | runtime_cast | Java | 0.242000 | 2.608200 | 0.09× |  |
| 语言特性 | module_call | C++ | 0.025000 | 3.475800 | 0.01× |  |
| 语言特性 | module_call | Python | 0.025000 | 27.900900 | 0.00× |  |
| 语言特性 | module_call | Java | 0.025000 | 6.457100 | 0.00× |  |
| 语言特性 | string_conversion | C++ | 11.303000 | 3.180800 | 3.55× |  |
| 语言特性 | string_conversion | Python | 11.303000 | 10.570500 | 1.07× |  |
| 语言特性 | string_conversion | Java | 11.303000 | 15.025100 | 0.75× |  |
| 语言特性 | deep_copy | C++ | 20.827000 | 2.343400 | 8.89× | 参考只复制已知形状；通用图复制另见新增契约 |
| 语言特性 | deep_copy | Python | 20.827000 | 66.980500 | 0.31× |  |
| 语言特性 | deep_copy | Java | 20.827000 | 6.883600 | 3.03× | 参考只复制已知形状；通用图复制另见新增契约 |
| 语言特性 | copy_cycle | C++ | 8.561000 | 1.919800 | 4.46× |  |
| 语言特性 | copy_cycle | Python | 8.561000 | 11.832100 | 0.72× |  |
| 语言特性 | copy_cycle | Java | 8.561000 | 4.335100 | 1.97× |  |
| 语言特性 | deinit | C++ | 11.408000 | 0.733200 | 15.56× |  |
| 语言特性 | deinit | Python | 11.408000 | 4.145400 | 2.75× |  |
| 语言特性 | deinit | Java | 11.408000 | 1.857800 | 6.14× | Java 显式 close 回调，不是 JVM 确定性析构 |
| 语言特性 | cycle_gc | C++ | 1.449000 | 0.073900 | 19.61× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 语言特性 | cycle_gc | Python | 1.449000 | 1.434000 | 1.01× |  |
| 语言特性 | cycle_gc | Java | 1.449000 | 2.504800 | 0.58× | 参考只识别单节点自环；通用图 GC 见契约组 |
| library/组合 | math | C++ | 4.000000 | 2.118600 | 1.89× | 整数毫秒短项，倍率精度有限 |
| library/组合 | random | C++ | 1.000000 | 0.338800 | 2.95× | 整数毫秒短项，倍率精度有限 |
| library/组合 | string | C++ | 10.000000 | 2.586900 | 3.87× | 整数毫秒短项，倍率精度有限 |
| library/组合 | array | C++ | 18.000000 | 12.514800 | 1.44× | 整数毫秒短项，倍率精度有限 |
| library/组合 | path | C++ | 22.000000 | 9.040200 | 2.43× | 整数毫秒短项，倍率精度有限 |
| library/组合 | fs | C++ | 44.000000 | 37.955400 | 1.16× | 整数毫秒短项，倍率精度有限 |
| library/组合 | file | C++ | 1298.000000 | 1256.253400 | 1.03× | 整数毫秒短项，倍率精度有限 |
| library/组合 | io | C++ | 8.000000 | 7.838100 | 1.02× | 整数毫秒短项，倍率精度有限 |
| library/组合 | time | C++ | 3.000000 | 3.347200 | 0.90× | 整数毫秒短项，倍率精度有限 |
| audit/compute | algorithm_sort | C++ | 5.892000 | 3.175000 | 1.86× |  |
| audit/compute | algorithm_sort | Python | 5.892000 | 6.017000 | 0.98× |  |
| audit/compute | algorithm_sort | Java | 5.892000 | 5.961000 | 0.99× |  |
| audit/compute | bytes_hex | C++ | 8.792000 | 1.832000 | 4.80× |  |
| audit/compute | bytes_hex | Python | 8.792000 | 3.172000 | 2.77× |  |
| audit/compute | bytes_hex | Java | 8.792000 | 3.865000 | 2.27× |  |
| audit/compute | cancel_status | C++ | 0.553000 | 0.383000 | 1.44× |  |
| audit/compute | cancel_status | Python | 0.553000 | 19.034000 | 0.03× |  |
| audit/compute | cancel_status | Java | 0.553000 | 0.616000 | 0.90× |  |
| audit/compute | crypto_sha256 | C++ | 8.804000 | 4.577000 | 1.92× |  |
| audit/compute | crypto_sha256 | Python | 8.804000 | 7.010000 | 1.26× |  |
| audit/compute | crypto_sha256 | Java | 8.804000 | 11.307000 | 0.78× |  |
| audit/compute | csv_parse | Python | 24.769000 | 15.270000 | 1.62× |  |
| audit/compute | decimal_add | Python | 11.036000 | 4.136000 | 2.67× |  |
| audit/compute | decimal_add | Java | 11.036000 | 2.544000 | 4.34× |  |
| audit/compute | dictionary_contains | C++ | 0.287000 | 0.188000 | 1.53× |  |
| audit/compute | dictionary_contains | Python | 0.287000 | 7.515000 | 0.04× |  |
| audit/compute | dictionary_contains | Java | 0.287000 | 1.181000 | 0.24× |  |
| audit/compute | encoding_utf8 | Python | 0.407000 | 1.827000 | 0.22× |  |
| audit/compute | encoding_utf8 | Java | 0.407000 | 4.493000 | 0.09× |  |
| audit/compute | env_get | C++ | 10.014000 | 52.092000 | 0.19× |  |
| audit/compute | env_get | Python | 10.014000 | 5.435000 | 1.84× |  |
| audit/compute | env_get | Java | 10.014000 | 1.836000 | 5.45× |  |
| audit/compute | format_text | C++ | 0.115000 | 0.790000 | 0.15× |  |
| audit/compute | format_text | Python | 0.115000 | 1.961000 | 0.06× |  |
| audit/compute | format_text | Java | 0.115000 | 8.455000 | 0.01× |  |
| audit/compute | json_parse | Python | 22.615000 | 14.088000 | 1.61× |  |
| audit/compute | json_parse | Java | 22.615000 | 24.372000 | 0.93× |  |
| audit/compute | math_sqrt | C++ | 4.109000 | 2.108000 | 1.95× |  |
| audit/compute | math_sqrt | Python | 4.109000 | 62.194000 | 0.07× |  |
| audit/compute | math_sqrt | Java | 4.109000 | 2.595000 | 1.58× |  |
| audit/compute | parse_int | C++ | 11.754000 | 4.782000 | 2.46× |  |
| audit/compute | parse_int | Python | 11.754000 | 10.038000 | 1.17× |  |
| audit/compute | parse_int | Java | 11.754000 | 1.759000 | 6.68× |  |
| audit/compute | random_int | C++ | 0.998000 | 0.327000 | 3.05× |  |
| audit/compute | random_int | Python | 0.998000 | 69.969000 | 0.01× | 随机序列不同，仅作原始负载观察 |
| audit/compute | random_int | Java | 0.998000 | 2.316000 | 0.43× | 随机序列不同，仅作原始负载观察 |
| audit/compute | regex_search | C++ | 12.787000 | 11.312000 | 1.13× |  |
| audit/compute | regex_search | Python | 12.787000 | 11.909000 | 1.07× |  |
| audit/compute | regex_search | Java | 12.787000 | 7.160000 | 1.79× |  |
| audit/compute | serde_json | Python | 12.420000 | 34.365000 | 0.36× |  |
| audit/compute | serde_json | Java | 12.420000 | 34.550000 | 0.36× |  |
| audit/compute | statistics_mean | Python | 4.540000 | 287.690000 | 0.02× |  |
| audit/compute | unicode_nfc | Python | 5.500000 | 2.130000 | 2.58× |  |
| audit/compute | unicode_nfc | Java | 5.500000 | 10.798000 | 0.51× |  |
| audit/compute | xml_parse | Python | 34.180000 | 75.140000 | 0.45× |  |
| audit/compute | xml_parse | Java | 34.180000 | 129.064000 | 0.26× |  |
| audit/features | vector_push | C++ | 1.942000 | 0.500000 | 3.88× |  |
| audit/features | vector_push | Python | 1.942000 | 3.906000 | 0.50× |  |
| audit/features | vector_push | Java | 1.942000 | 2.030000 | 0.96× |  |
| audit/features | vector_index | C++ | 0.175000 | 0.209000 | 0.84× |  |
| audit/features | vector_index | Python | 0.175000 | 23.701000 | 0.01× |  |
| audit/features | vector_index | Java | 0.175000 | 2.127000 | 0.08× |  |
| audit/features | map_lookup | C++ | 0.327000 | 0.184000 | 1.78× |  |
| audit/features | map_lookup | Python | 0.327000 | 5.263000 | 0.06× |  |
| audit/features | map_lookup | Java | 0.327000 | 1.623000 | 0.20× |  |
| audit/features | set_contains | C++ | 0.258000 | 0.170000 | 1.52× |  |
| audit/features | set_contains | Python | 0.258000 | 8.015000 | 0.03× |  |
| audit/features | set_contains | Java | 0.258000 | 1.247000 | 0.21× |  |
| audit/features | heap_push_pop | C++ | 7.475000 | 3.001000 | 2.49× |  |
| audit/features | heap_push_pop | Python | 7.475000 | 15.472000 | 0.48× |  |
| audit/features | heap_push_pop | Java | 7.475000 | 8.960000 | 0.83× |  |
| audit/features | queue_push_pop | C++ | 1.165000 | 0.324000 | 3.60× |  |
| audit/features | queue_push_pop | Python | 1.165000 | 7.938000 | 0.15× |  |
| audit/features | queue_push_pop | Java | 1.165000 | 3.437000 | 0.34× |  |
| audit/features | iterator_snapshot | C++ | 0.164000 | 0.047000 | 3.49× |  |
| audit/features | iterator_snapshot | Python | 0.164000 | 3.854000 | 0.04× |  |
| audit/features | iterator_snapshot | Java | 0.164000 | 0.824000 | 0.20× |  |
| audit/features | option_value | C++ | 0.025000 | 0.013000 | 1.92× |  |
| audit/features | option_value | Python | 0.025000 | 18.095000 | 0.00× |  |
| audit/features | option_value | Java | 0.025000 | 0.999000 | 0.03× |  |
| audit/features | function_value | C++ | 0.282000 | 0.163000 | 1.73× |  |
| audit/features | function_value | Python | 0.282000 | 6.395000 | 0.04× |  |
| audit/features | function_value | Java | 0.282000 | 0.376000 | 0.75× |  |
| audit/features | closure_bind | C++ | 0.171000 | 0.163000 | 1.05× |  |
| audit/features | closure_bind | Python | 0.171000 | 8.210000 | 0.02× |  |
| audit/features | closure_bind | Java | 0.171000 | 0.452000 | 0.38× |  |
| audit/system | file_stream_rw | C++ | 850.519000 | 846.952000 | 1.00× |  |
| audit/system | file_stream_rw | Python | 850.519000 | 914.453000 | 0.93× |  |
| audit/system | file_stream_rw | Java | 850.519000 | 943.169000 | 0.90× |  |
| audit/system | process_spawn | C++ | 913.913000 | 2103.749000 | 0.43× |  |
| audit/system | process_spawn | Python | 913.913000 | 896.187000 | 1.02× |  |
| audit/system | process_spawn | Java | 913.913000 | 7464.389000 | 0.12× |  |
| network/http | requests_get | Python | 977.079000 | 1130.713000 | 0.86× |  |
| network/websocket | websocket_echo | C++ | 1003.761000 | 931.344000 | 1.08× |  |
| network/websocket | websocket_echo | Python | 1003.761000 | 696.395000 | 1.44× |  |
| network/websocket | websocket_echo | Java | 1003.761000 | 428.918000 | 2.34× |  |
| 网络观察 | httpx_get / http_get | Java | 962.718000 | 235.224000 | 4.09× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 网络观察 | requests_get / http_get | Java | 977.079000 | 235.224000 | 4.15× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 综合 | vector_scan_1k | C++ | 0.261000 | 0.117000 | 2.23× |  |
| 综合 | vector_scan_1k | Python | 0.261000 | 23.458000 | 0.01× |  |
| 综合 | vector_scan_1k | Java | 0.261000 | 2.111000 | 0.12× |  |
| 综合 | vector_scan_100k | C++ | 3.584000 | 1.187000 | 3.02× |  |
| 综合 | vector_scan_100k | Python | 3.584000 | 235.644000 | 0.02× |  |
| 综合 | vector_scan_100k | Java | 3.584000 | 7.696000 | 0.47× |  |
| 综合 | vector_index_sequential | C++ | 0.297000 | 0.317000 | 0.94× |  |
| 综合 | vector_index_sequential | Python | 0.297000 | 69.585000 | 0.00× |  |
| 综合 | vector_index_sequential | Java | 0.297000 | 2.764000 | 0.11× |  |
| 综合 | vector_index_strided | C++ | 0.295000 | 0.303000 | 0.97× |  |
| 综合 | vector_index_strided | Python | 0.295000 | 70.261000 | 0.00× |  |
| 综合 | vector_index_strided | Java | 0.295000 | 3.098000 | 0.10× |  |
| 综合 | map_hit_128 | C++ | 1.581000 | 0.856000 | 1.85× |  |
| 综合 | map_hit_128 | Python | 1.581000 | 33.363000 | 0.05× |  |
| 综合 | map_hit_128 | Java | 1.581000 | 10.948000 | 0.14× |  |
| 综合 | map_hit_8192 | C++ | 2.262000 | 0.843000 | 2.68× |  |
| 综合 | map_hit_8192 | Python | 2.262000 | 40.735000 | 0.06× |  |
| 综合 | map_hit_8192 | Java | 2.262000 | 11.115000 | 0.20× |  |
| 综合 | map_hit_10_percent | C++ | 1.967000 | 1.533000 | 1.28× |  |
| 综合 | map_hit_10_percent | Python | 1.967000 | 46.398000 | 0.04× |  |
| 综合 | map_hit_10_percent | Java | 1.967000 | 9.488000 | 0.21× |  |
| 综合 | dictionary_int_hit | C++ | 0.879000 | 0.760000 | 1.16× |  |
| 综合 | dictionary_int_hit | Python | 0.879000 | 14.664000 | 0.06× |  |
| 综合 | dictionary_int_hit | Java | 0.879000 | 4.209000 | 0.21× |  |
| 综合 | dictionary_text_hit | C++ | 2.324000 | 4.372000 | 0.53× |  |
| 综合 | dictionary_text_hit | Python | 2.324000 | 14.456000 | 0.16× |  |
| 综合 | dictionary_text_hit | Java | 2.324000 | 3.425000 | 0.68× |  |
| 综合 | dictionary_mostly_miss | C++ | 0.544000 | 0.747000 | 0.73× |  |
| 综合 | dictionary_mostly_miss | Python | 0.544000 | 15.293000 | 0.04× |  |
| 综合 | dictionary_mostly_miss | Java | 0.544000 | 5.117000 | 0.11× |  |
| 综合 | format_literal | C++ | 0.591000 | 0.768000 | 0.77× |  |
| 综合 | format_literal | Python | 0.591000 | 5.123000 | 0.12× |  |
| 综合 | format_literal | Java | 0.591000 | 15.081000 | 0.04× |  |
| 综合 | format_dynamic | C++ | 0.564000 | 1.059000 | 0.53× | 局部不可变模板，不能代表真正动态格式化 |
| 综合 | format_dynamic | Python | 0.564000 | 10.031000 | 0.06× | 局部不可变模板，不能代表真正动态格式化 |
| 综合 | format_dynamic | Java | 0.564000 | 29.213000 | 0.02× | 局部不可变模板，不能代表真正动态格式化 |
| 综合 | encoding_literal | C++ | 1.086000 | 2.833000 | 0.38× |  |
| 综合 | encoding_literal | Python | 1.086000 | 4.915000 | 0.22× |  |
| 综合 | encoding_literal | Java | 1.086000 | 5.747000 | 0.19× |  |
| 综合 | encoding_dynamic | C++ | 1.020000 | 2.684000 | 0.38× |  |
| 综合 | encoding_dynamic | Python | 1.020000 | 4.899000 | 0.21× |  |
| 综合 | encoding_dynamic | Java | 1.020000 | 4.708000 | 0.22× |  |
| 综合 | serde_short_text | C++ | 6.436000 | 0.940000 | 6.85× |  |
| 综合 | serde_short_text | Python | 6.436000 | 16.270000 | 0.40× |  |
| 综合 | serde_short_text | Java | 6.436000 | 14.253000 | 0.45× |  |
| 综合 | serde_long_text | C++ | 8.192000 | 4.387000 | 1.87× |  |
| 综合 | serde_long_text | Python | 8.192000 | 18.435000 | 0.44× |  |
| 综合 | serde_long_text | Java | 8.192000 | 17.988000 | 0.46× |  |
| 综合 | parse_valid | C++ | 0.758000 | 1.709000 | 0.44× |  |
| 综合 | parse_valid | Python | 0.758000 | 13.085000 | 0.06× |  |
| 综合 | parse_valid | Java | 0.758000 | 9.843000 | 0.08× |  |
| 综合 | parse_invalid | C++ | 0.755000 | 6.493000 | 0.12× |  |
| 综合 | parse_invalid | Python | 0.755000 | 75.031000 | 0.01× |  |
| 综合 | parse_invalid | Java | 0.755000 | 149.392000 | 0.01× |  |
| 校准/diverse | vector_scan_1k | GCC C++ | 0.255000 | 0.115000 | 2.22× |  |
| 校准/diverse | vector_scan_100k | GCC C++ | 3.524000 | 1.162000 | 3.03× |  |
| 校准/diverse | vector_index_sequential | GCC C++ | 0.292000 | 0.307000 | 0.95× |  |
| 校准/diverse | vector_index_strided | GCC C++ | 0.292000 | 0.292000 | 1.00× |  |
| 校准/diverse | map_hit_128 | GCC C++ | 1.516000 | 0.816000 | 1.86× |  |
| 校准/diverse | map_hit_8192 | GCC C++ | 2.020000 | 0.822000 | 2.46× |  |
| 校准/diverse | map_hit_10_percent | GCC C++ | 1.932000 | 1.484000 | 1.30× |  |
| 校准/diverse | dictionary_int_hit | GCC C++ | 0.874000 | 0.745000 | 1.17× |  |
| 校准/diverse | dictionary_text_hit | GCC C++ | 2.249000 | 4.247000 | 0.53× |  |
| 校准/diverse | dictionary_mostly_miss | GCC C++ | 0.538000 | 0.725000 | 0.74× |  |
| 校准/diverse | format_literal | GCC C++ | 0.567000 | 0.750000 | 0.76× |  |
| 校准/diverse | format_dynamic | GCC C++ | 0.568000 | 1.012000 | 0.56× |  |
| 校准/diverse | encoding_literal | GCC C++ | 1.076000 | 2.792000 | 0.39× |  |
| 校准/diverse | encoding_dynamic | GCC C++ | 1.010000 | 2.690000 | 0.38× |  |
| 校准/diverse | serde_short_text | GCC C++ | 6.050000 | 0.946000 | 6.40× |  |
| 校准/diverse | serde_long_text | GCC C++ | 7.882000 | 4.296000 | 1.83× |  |
| 校准/diverse | parse_valid | GCC C++ | 0.744000 | 1.701000 | 0.44× |  |
| 校准/diverse | parse_invalid | GCC C++ | 0.741000 | 6.047000 | 0.12× |  |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.256000 | 0.064000 | 4.00× |  |
| 校准/diverse_clang | vector_scan_100k | clang C++ | 3.554000 | 0.809000 | 4.39× |  |
| 校准/diverse_clang | vector_index_sequential | clang C++ | 0.293000 | 0.230000 | 1.27× |  |
| 校准/diverse_clang | vector_index_strided | clang C++ | 0.293000 | 0.231000 | 1.27× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.583000 | 0.348000 | 4.55× |  |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.006000 | 0.556000 | 3.61× |  |
| 校准/diverse_clang | map_hit_10_percent | clang C++ | 1.959000 | 1.233000 | 1.59× |  |
| 校准/diverse_clang | dictionary_int_hit | clang C++ | 0.918000 | 0.830000 | 1.11× |  |
| 校准/diverse_clang | dictionary_text_hit | clang C++ | 2.317000 | 4.160000 | 0.56× |  |
| 校准/diverse_clang | dictionary_mostly_miss | clang C++ | 0.539000 | 0.854000 | 0.63× |  |
| 校准/diverse_clang | format_literal | clang C++ | 0.571000 | 0.624000 | 0.92× |  |
| 校准/diverse_clang | format_dynamic | clang C++ | 0.561000 | 0.888000 | 0.63× |  |
| 校准/diverse_clang | encoding_literal | clang C++ | 1.075000 | 2.957000 | 0.36× |  |
| 校准/diverse_clang | encoding_dynamic | clang C++ | 1.007000 | 2.873000 | 0.35× |  |
| 校准/diverse_clang | serde_short_text | clang C++ | 6.246000 | 0.973000 | 6.42× |  |
| 校准/diverse_clang | serde_long_text | clang C++ | 7.939000 | 4.548000 | 1.75× |  |
| 校准/diverse_clang | parse_valid | clang C++ | 0.730000 | 0.952000 | 0.77× |  |
| 校准/diverse_clang | parse_invalid | clang C++ | 0.739000 | 5.339000 | 0.14× |  |
| 校准/heap | heap_push_pop | GCC C++ | 7.519000 | 3.463000 | 2.17× |  |
| 校准/heap_clang | heap_push_pop | clang C++ | 7.447000 | 4.383000 | 1.70× |  |
| 校准/statistics | statistics_mean | GCC C++ | 4.439000 | 2.785000 | 1.59× |  |
| 校准/statistics_clang | statistics_mean | clang C++ | 4.398000 | 4.098000 | 1.07× |  |
| 外部题/mini-filesystem | fanout-2000 | C++ | 12.817000 | 1.826000 | 7.02× |  |
| 外部题/mini-filesystem | fanout-2000 | JavaScript | 12.817000 | 5.597000 | 2.29× |  |
| 外部题/mini-filesystem | fanout-2000 | Java | 12.817000 | 9.036000 | 1.42× |  |
| 外部题/mini-filesystem | fanout-2000 | Python | 12.817000 | 4.399000 | 2.91× |  |
| 外部题/mini-filesystem | deep-pwd-2000 | C++ | 11.564000 | 14.984000 | 0.77× |  |
| 外部题/mini-filesystem | deep-pwd-2000 | JavaScript | 11.564000 | 9.708000 | 1.19× |  |
| 外部题/mini-filesystem | deep-pwd-2000 | Java | 11.564000 | 32.458000 | 0.36× |  |
| 外部题/mini-filesystem | deep-pwd-2000 | Python | 11.564000 | 11.644000 | 0.99× |  |
| 外部题/mini-filesystem | moves-2000 | C++ | 5.232000 | 1.006000 | 5.20× |  |
| 外部题/mini-filesystem | moves-2000 | JavaScript | 5.232000 | 1.974000 | 2.65× |  |
| 外部题/mini-filesystem | moves-2000 | Java | 5.232000 | 5.812000 | 0.90× |  |
| 外部题/mini-filesystem | moves-2000 | Python | 5.232000 | 1.935000 | 2.70× |  |
| 外部题/mini-filesystem | random-2000-1 | C++ | 5.999000 | 0.971000 | 6.18× |  |
| 外部题/mini-filesystem | random-2000-1 | JavaScript | 5.999000 | 1.977000 | 3.03× |  |
| 外部题/mini-filesystem | random-2000-1 | Java | 5.999000 | 5.856000 | 1.02× |  |
| 外部题/mini-filesystem | random-2000-1 | Python | 5.999000 | 2.568000 | 2.34× |  |
| 外部题/mini-filesystem | linklong-2000 | C++ | 31.150000 | 116.025000 | 0.27× |  |
| 外部题/mini-filesystem | linklong-2000 | JavaScript | 31.150000 | 120.678000 | 0.26× |  |
| 外部题/mini-filesystem | linklong-2000 | Java | 31.150000 | 278.662000 | 0.11× |  |
| 外部题/mini-filesystem | linklong-2000 | Python | 31.150000 | 458.638000 | 0.07× |  |
| 外部题/not-yet-on-stage | all-free-max | C++ | 1.025000 | 1.438000 | 0.71× |  |
| 外部题/not-yet-on-stage | all-free-max | JavaScript | 1.025000 | 3.310000 | 0.31× |  |
| 外部题/not-yet-on-stage | all-free-max | Java | 1.025000 | 4.496000 | 0.23× |  |
| 外部题/not-yet-on-stage | all-free-max | Python | 1.025000 | 45.986000 | 0.02× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.353000 | 1.442000 | 3.02× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | JavaScript | 4.353000 | 4.684000 | 0.93× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | Java | 4.353000 | 5.479000 | 0.79× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | Python | 4.353000 | 78.369000 | 0.06× |  |
| 外部题/not-yet-on-stage | forced-decreasing-max | C++ | 0.996000 | 1.426000 | 0.70× |  |
| 外部题/not-yet-on-stage | forced-decreasing-max | JavaScript | 0.996000 | 3.263000 | 0.31× |  |
| 外部题/not-yet-on-stage | forced-decreasing-max | Java | 0.996000 | 4.880000 | 0.20× |  |
| 外部题/not-yet-on-stage | forced-decreasing-max | Python | 0.996000 | 45.125000 | 0.02× |  |
| 外部题/not-yet-on-stage | shuffled-tight-max-1 | C++ | 3.178000 | 1.890000 | 1.68× |  |
| 外部题/not-yet-on-stage | shuffled-tight-max-1 | JavaScript | 3.178000 | 4.396000 | 0.72× |  |
| 外部题/not-yet-on-stage | shuffled-tight-max-1 | Java | 3.178000 | 5.445000 | 0.58× |  |
| 外部题/not-yet-on-stage | shuffled-tight-max-1 | Python | 3.178000 | 64.999000 | 0.05× |  |
| 外部题/not-yet-on-stage | alternating-tight-max | C++ | 2.677000 | 1.447000 | 1.85× |  |
| 外部题/not-yet-on-stage | alternating-tight-max | JavaScript | 2.677000 | 4.016000 | 0.67× |  |
| 外部题/not-yet-on-stage | alternating-tight-max | Java | 2.677000 | 5.323000 | 0.50× |  |
| 外部题/not-yet-on-stage | alternating-tight-max | Python | 2.677000 | 61.040000 | 0.04× |  |
| 长随机数 | random_long | C++ | 25.000000 | 8.514700 | 2.94× |  |
| 契约/format_contract | format_alternating | C++ | 17.156000 | 3.237000 | 5.30× |  |
| 契约/format_contract | format_parameter | C++ | 17.288000 | 3.264000 | 5.30× |  |
| 契约/graph_contract | graph_copy | C++ | 4.798000 | 2.209000 | 2.17× | 直接链接 TX 运行时；不代表前端生成代码的全部语义 |
| 契约/graph_contract | graph_gc | C++ | 7.531000 | 8.037000 | 0.94× | 直接链接 TX 运行时；不代表前端生成代码的全部语义 |
| 新增/concurrency | thread_spawn_join | C++ | 13.873000 | 14.589400 | 0.95× | C++ std::thread；Java 平台线程；Python threading。含线程创建/退出。 |
| 新增/concurrency | thread_spawn_join | Java | 13.873000 | 25.816800 | 0.54× | C++ std::thread；Java 平台线程；Python threading。含线程创建/退出。 |
| 新增/concurrency | thread_spawn_join | Python | 13.873000 | 17.449500 | 0.80× | C++ std::thread；Java 平台线程；Python threading。含线程创建/退出。 |
| 新增/concurrency | mutex_uncontended | C++ | 1.383000 | 0.374500 | 3.69× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 新增/concurrency | mutex_uncontended | Java | 1.383000 | 5.821100 | 0.24× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 新增/concurrency | mutex_uncontended | Python | 1.383000 | 15.495700 | 0.09× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 新增/concurrency | atomic_add | C++ | 0.397000 | 0.173900 | 2.28× | C++ std::atomic、Java AtomicLong；Python 没有对应公开原子整数，使用 Lock 实现线程安全加法，不能当成原子指令性能。 |
| 新增/concurrency | atomic_add | Java | 0.397000 | 0.803000 | 0.49× | C++ std::atomic、Java AtomicLong；Python 没有对应公开原子整数，使用 Lock 实现线程安全加法，不能当成原子指令性能。 |
| 新增/concurrency | atomic_add | Python | 0.397000 | 15.454500 | 0.03× | C++ std::atomic、Java AtomicLong；Python 没有对应公开原子整数，使用 Lock 实现线程安全加法，不能当成原子指令性能。 |
| 新增/concurrency | channel_send_recv | C++ | 1.935000 | 0.789100 | 2.45× | C++ mutex/condition_variable 队列；Java ArrayBlockingQueue；Python Queue。都是立即成功路径；TX 另有取消及句柄规则，不代表多生产者吞吐。 |
| 新增/concurrency | channel_send_recv | Java | 1.935000 | 3.681900 | 0.53× | C++ mutex/condition_variable 队列；Java ArrayBlockingQueue；Python Queue。都是立即成功路径；TX 另有取消及句柄规则，不代表多生产者吞吐。 |
| 新增/concurrency | channel_send_recv | Python | 1.935000 | 31.627000 | 0.06× | C++ mutex/condition_variable 队列；Java ArrayBlockingQueue；Python Queue。都是立即成功路径；TX 另有取消及句柄规则，不代表多生产者吞吐。 |
| 新增/concurrency | task_spawn_wait | C++ | 7.271000 | 7.028700 | 1.03× | 参考使用单工作线程池；TX 使用任务运行时和 scope。包含组内初次线程池启动，不代表饱和并发吞吐。 |
| 新增/concurrency | task_spawn_wait | Java | 7.271000 | 22.781600 | 0.32× | 参考使用单工作线程池；TX 使用任务运行时和 scope。包含组内初次线程池启动，不代表饱和并发吞吐。 |
| 新增/concurrency | task_spawn_wait | Python | 7.271000 | 17.372800 | 0.42× | 参考使用单工作线程池；TX 使用任务运行时和 scope。包含组内初次线程池启动，不代表饱和并发吞吐。 |
| 新增/sqlite | sqlite_insert | C++ | 11.867000 | 8.739200 | 1.36× | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| 新增/sqlite | sqlite_insert | Java | 11.867000 | 27.602300 | 0.43× | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| 新增/sqlite | sqlite_insert | Python | 11.867000 | 1.946400 | 6.10× | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| 新增/sqlite | sqlite_read | C++ | 24.775000 | 9.213700 | 2.69× | C++ 使用同一后端及行快照；Java JDBC、Python sqlite3。TX/C++ 差值包含 ABI、option、行读取接口与生成代码开销。 |
| 新增/sqlite | sqlite_read | Java | 24.775000 | 26.594300 | 0.93× | C++ 使用同一后端及行快照；Java JDBC、Python sqlite3。TX/C++ 差值包含 ABI、option、行读取接口与生成代码开销。 |
| 新增/sqlite | sqlite_read | Python | 24.775000 | 9.433900 | 2.63× | C++ 使用同一后端及行快照；Java JDBC、Python sqlite3。TX/C++ 差值包含 ABI、option、行读取接口与生成代码开销。 |
| 新增/sqlite | sqlite_savepoint | C++ | 2.827000 | 2.075100 | 1.36× | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| 新增/sqlite | sqlite_savepoint | Java | 2.827000 | 6.798600 | 0.42× | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| 新增/sqlite | sqlite_savepoint | Python | 2.827000 | 0.655100 | 4.32× | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| 新增/sqlite | sqlite_pool | C++ | 1.691000 | 1.743400 | 0.97× | TX/C++ 使用当前同一数据库池后端，已证明无会话副作用的只读连接可复用；Java/Python 仍每次关闭连接。跨语言生命周期不同，仅作端到端观察。 |
| 新增/sqlite | sqlite_pool | Java | 1.691000 | 81.727100 | 0.02× | TX/C++ 使用当前同一数据库池后端，已证明无会话副作用的只读连接可复用；Java/Python 仍每次关闭连接。跨语言生命周期不同，仅作端到端观察。 |
| 新增/sqlite | sqlite_pool | Python | 1.691000 | 69.777400 | 0.02× | TX/C++ 使用当前同一数据库池后端，已证明无会话副作用的只读连接可复用；Java/Python 仍每次关闭连接。跨语言生命周期不同，仅作端到端观察。 |
| 新增/sqlite | sqlite_async | C++ | 3.340000 | 2.847800 | 1.17× | TX 任务运行时；C++ 同异步数据库后端加单线程执行器；Java/Python 单线程池。TX/C++ 当前可复用安全的只读池连接，Java/Python 仍关闭连接；跨语言不代表相同连接生命周期。 |
| 新增/sqlite | sqlite_async | Java | 3.340000 | 70.601200 | 0.05× | TX 任务运行时；C++ 同异步数据库后端加单线程执行器；Java/Python 单线程池。TX/C++ 当前可复用安全的只读池连接，Java/Python 仍关闭连接；跨语言不代表相同连接生命周期。 |
| 新增/sqlite | sqlite_async | Python | 3.340000 | 50.849100 | 0.07× | TX 任务运行时；C++ 同异步数据库后端加单线程执行器；Java/Python 单线程池。TX/C++ 当前可复用安全的只读池连接，Java/Python 仍关闭连接；跨语言不代表相同连接生命周期。 |
| 新增/sqlite | migration_recheck | C++ | 2.056000 | 2.172800 | 0.95× | C++ 同迁移后端；Java/Python 实现相同长度分帧 SHA-256、两次 IMMEDIATE 事务及账本读取；只覆盖幂等成功路径。 |
| 新增/sqlite | migration_recheck | Java | 2.056000 | 10.212400 | 0.20× | C++ 同迁移后端；Java/Python 实现相同长度分帧 SHA-256、两次 IMMEDIATE 事务及账本读取；只覆盖幂等成功路径。 |
| 新增/sqlite | migration_recheck | Python | 2.056000 | 0.968800 | 2.12× | C++ 同迁移后端；Java/Python 实现相同长度分帧 SHA-256、两次 IMMEDIATE 事务及账本读取；只覆盖幂等成功路径。 |
| 新增/postgres | postgres_insert | C++ | 185.869000 | 173.297000 | 1.07× | C++ 同 libpq 后端；Java PostgreSQL JDBC；Python psycopg。驱动 prepare 缓存策略不同；共用临时 PostgreSQL 18.4，不连接已有数据库。 |
| 新增/postgres | postgres_insert | Java | 185.869000 | 268.594900 | 0.69× | C++ 同 libpq 后端；Java PostgreSQL JDBC；Python psycopg。驱动 prepare 缓存策略不同；共用临时 PostgreSQL 18.4，不连接已有数据库。 |
| 新增/postgres | postgres_insert | Python | 185.869000 | 262.084100 | 0.71× | C++ 同 libpq 后端；Java PostgreSQL JDBC；Python psycopg。驱动 prepare 缓存策略不同；共用临时 PostgreSQL 18.4，不连接已有数据库。 |
| 新增/postgres | postgres_read | C++ | 38.771000 | 23.443800 | 1.65× | TX/C++ libpq 单行模式；Java/Python 默认结果获取策略不同。C++ 同后端可用于定位包装层开销，跨驱动比例是端到端负载观察。 |
| 新增/postgres | postgres_read | Java | 38.771000 | 28.622100 | 1.35× | TX/C++ libpq 单行模式；Java/Python 默认结果获取策略不同。C++ 同后端可用于定位包装层开销，跨驱动比例是端到端负载观察。 |
| 新增/postgres | postgres_read | Python | 38.771000 | 19.083200 | 2.03× | TX/C++ libpq 单行模式；Java/Python 默认结果获取策略不同。C++ 同后端可用于定位包装层开销，跨驱动比例是端到端负载观察。 |
| 新增/postgres | postgres_savepoint | C++ | 105.332000 | 129.209000 | 0.82× | C++ 同后端；Java/Python 各自驱动。所有操作在真实服务器执行，受本机网络调度影响。 |
| 新增/postgres | postgres_savepoint | Java | 105.332000 | 109.728600 | 0.96× | C++ 同后端；Java/Python 各自驱动。所有操作在真实服务器执行，受本机网络调度影响。 |
| 新增/postgres | postgres_savepoint | Python | 105.332000 | 164.485000 | 0.64× | C++ 同后端；Java/Python 各自驱动。所有操作在真实服务器执行，受本机网络调度影响。 |
| 新增/security | secret_equal | C++ | 14.277000 | 8.904100 | 1.60× | C++ 同 secret 后端；Java MessageDigest.isEqual；Python hmac.compare_digest。Java/Python 普通字节数组没有 TX 秘密句柄的保护与清零生命周期。 |
| 新增/security | secret_equal | Java | 14.277000 | 27.067900 | 0.53× | C++ 同 secret 后端；Java MessageDigest.isEqual；Python hmac.compare_digest。Java/Python 普通字节数组没有 TX 秘密句柄的保护与清零生命周期。 |
| 新增/security | secret_equal | Python | 14.277000 | 11.825700 | 1.21× | C++ 同 secret 后端；Java MessageDigest.isEqual；Python hmac.compare_digest。Java/Python 普通字节数组没有 TX 秘密句柄的保护与清零生命周期。 |
| 新增/security | argon2_hash_verify | C++ | 150.038000 | 153.365000 | 0.98× | m=19456 KiB、t=2、p=1、v=19、16 字节随机 salt、32 字节输出；C++ 同 Argon2 后端，Java BouncyCastle，Python argon2-cffi；含 PHC 编解码。 |
| 新增/security | argon2_hash_verify | Java | 150.038000 | 531.273700 | 0.28× | m=19456 KiB、t=2、p=1、v=19、16 字节随机 salt、32 字节输出；C++ 同 Argon2 后端，Java BouncyCastle，Python argon2-cffi；含 PHC 编解码。 |
| 新增/security | argon2_hash_verify | Python | 150.038000 | 158.615700 | 0.95× | m=19456 KiB、t=2、p=1、v=19、16 字节随机 salt、32 字节输出；C++ 同 Argon2 后端，Java BouncyCastle，Python argon2-cffi；含 PHC 编解码。 |
| 新增/security | ed25519_sign | C++ | 17.763000 | 16.675300 | 1.07× | 共用临时随机 seed；C++ 同 libsodium 后端；Java JCA Ed25519；Python cryptography。密钥导入不计时，签名随后验签。 |
| 新增/security | ed25519_sign | Java | 17.763000 | 408.045700 | 0.04× | 共用临时随机 seed；C++ 同 libsodium 后端；Java JCA Ed25519；Python cryptography。密钥导入不计时，签名随后验签。 |
| 新增/security | ed25519_sign | Python | 17.763000 | 17.119800 | 1.04× | 共用临时随机 seed；C++ 同 libsodium 后端；Java JCA Ed25519；Python cryptography。密钥导入不计时，签名随后验签。 |
| 新增/security | ed25519_verify | C++ | 25.124000 | 24.568800 | 1.02× | 三类后端验证各自生成的签名；同一 seed 和消息，成功次数逐轮核对。 |
| 新增/security | ed25519_verify | Java | 25.124000 | 362.542700 | 0.07× | 三类后端验证各自生成的签名；同一 seed 和消息，成功次数逐轮核对。 |
| 新增/security | ed25519_verify | Python | 25.124000 | 45.986200 | 0.55× | 三类后端验证各自生成的签名；同一 seed 和消息，成功次数逐轮核对。 |
| 新增/security | x509_parse_der | C++ | 2.891000 | 2.813600 | 1.03× | C++ 同 Windows 证书后端；Java CertificateFactory 可能缓存；Python cryptography。测重复证书热路径，不外推到大量不同证书。 |
| 新增/security | x509_parse_der | Java | 2.891000 | 6.908200 | 0.42× | C++ 同 Windows 证书后端；Java CertificateFactory 可能缓存；Python cryptography。测重复证书热路径，不外推到大量不同证书。 |
| 新增/security | x509_parse_der | Python | 2.891000 | 3.115700 | 0.93× | C++ 同 Windows 证书后端；Java CertificateFactory 可能缓存；Python cryptography。测重复证书热路径，不外推到大量不同证书。 |
| 新增/network | dns_localhost | C++ | 0.284000 | 31.721600 | 0.01× | TX 对 localhost 有专用路径；C++ getaddrinfo、Java InetAddress、Python getaddrinfo 各有不同缓存策略。不能用于比较远端 DNS。 |
| 新增/network | dns_localhost | Java | 0.284000 | 54.517300 | 0.01× | TX 对 localhost 有专用路径；C++ getaddrinfo、Java InetAddress、Python getaddrinfo 各有不同缓存策略。不能用于比较远端 DNS。 |
| 新增/network | dns_localhost | Python | 0.284000 | 39.356600 | 0.01× | TX 对 localhost 有专用路径；C++ getaddrinfo、Java InetAddress、Python getaddrinfo 各有不同缓存策略。不能用于比较远端 DNS。 |
| 新增/network | udp_echo | C++ | 34.740000 | 24.124800 | 1.44× | 四语言使用同一 Python 回声服务，逐次完整比较 payload。服务端调度也计入端到端时间。 |
| 新增/network | udp_echo | Java | 34.740000 | 44.072500 | 0.79× | 四语言使用同一 Python 回声服务，逐次完整比较 payload。服务端调度也计入端到端时间。 |
| 新增/network | udp_echo | Python | 34.740000 | 30.605600 | 1.14× | 四语言使用同一 Python 回声服务，逐次完整比较 payload。服务端调度也计入端到端时间。 |
| 新增/network | ipc_echo | C++ | 13.990000 | 8.581100 | 1.63× | 使用同一服务器、22 字节帧和 CBOR 整数 42。C++/Java/Python 固定整数编解码；TX 为通用 CBOR 与资源句柄，不代表任意对象 IPC 差距。 |
| 新增/network | ipc_echo | Java | 13.990000 | 12.067100 | 1.16× | 使用同一服务器、22 字节帧和 CBOR 整数 42。C++/Java/Python 固定整数编解码；TX 为通用 CBOR 与资源句柄，不代表任意对象 IPC 差距。 |
| 新增/network | ipc_echo | Python | 13.990000 | 9.734200 | 1.44× | 使用同一服务器、22 字节帧和 CBOR 整数 42。C++/Java/Python 固定整数编解码；TX 为通用 CBOR 与资源句柄，不代表任意对象 IPC 差距。 |
| 新增/network | tls_handshake | C++ | 65.634000 | 25.868600 | 2.54× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 新增/network | tls_handshake | Java | 65.634000 | 226.490700 | 0.29× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 新增/network | tls_handshake | Python | 65.634000 | 63.272600 | 1.04× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 新增/async_file | async_file_rw | C++ | 40.389000 | 33.295100 | 1.21× | TX IOCP、Java AsynchronousFileChannel；C++/Python 工作线程执行定位 I/O。全部逐次等待并核对字节，不代表大量在途 I/O 吞吐。 |
| 新增/async_file | async_file_rw | Java | 40.389000 | 69.751500 | 0.58× | TX IOCP、Java AsynchronousFileChannel；C++/Python 工作线程执行定位 I/O。全部逐次等待并核对字节，不代表大量在途 I/O 吞吐。 |
| 新增/async_file | async_file_rw | Python | 40.389000 | 147.383200 | 0.27× | TX IOCP、Java AsynchronousFileChannel；C++/Python 工作线程执行定位 I/O。全部逐次等待并核对字节，不代表大量在途 I/O 吞吐。 |
| 新增/diagnostics | test_parameterized | C++ | 0.190000 | 0.056900 | 3.34× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 新增/diagnostics | test_parameterized | Java | 0.190000 | 0.817600 | 0.23× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 新增/diagnostics | test_parameterized | Python | 0.190000 | 1.514000 | 0.13× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 新增/diagnostics | test_property | C++ | 0.304000 | 0.046100 | 6.59× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 新增/diagnostics | test_property | Java | 0.304000 | 1.099800 | 0.28× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 新增/diagnostics | test_property | Python | 0.304000 | 2.348800 | 0.13× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 新增/diagnostics | log_filtered | C++ | 0.838000 | 0.414700 | 2.02× | C++ 调用真实 TX 日志后端级别检查，Java Logger，Python logging；各参考仅在级别允许时构造字段，TX 另有闭包/错误边界。废弃初版 C++ volatile 检查倍率。 |
| 新增/diagnostics | log_filtered | Java | 0.838000 | 3.835700 | 0.22× | C++ 调用真实 TX 日志后端级别检查，Java Logger，Python logging；各参考仅在级别允许时构造字段，TX 另有闭包/错误边界。废弃初版 C++ volatile 检查倍率。 |
| 新增/diagnostics | log_filtered | Python | 0.838000 | 6.563800 | 0.13× | C++ 调用真实 TX 日志后端级别检查，Java Logger，Python logging；各参考仅在级别允许时构造字段，TX 另有闭包/错误边界。废弃初版 C++ volatile 检查倍率。 |
| 新增/diagnostics | log_file | C++ | 31.382000 | 28.558600 | 1.10× | C++ 同日志后端；Java/Python 固定 schema 构造。全部每条 flush、进程内加锁；计时外核对行数、索引、请求标识、遮蔽值。未覆盖轮转压力。 |
| 新增/diagnostics | log_file | Java | 31.382000 | 44.461700 | 0.71× | C++ 同日志后端；Java/Python 固定 schema 构造。全部每条 flush、进程内加锁；计时外核对行数、索引、请求标识、遮蔽值。未覆盖轮转压力。 |
| 新增/diagnostics | log_file | Python | 31.382000 | 26.121900 | 1.20× | C++ 同日志后端；Java/Python 固定 schema 构造。全部每条 flush、进程内加锁；计时外核对行数、索引、请求标识、遮蔽值。未覆盖轮转压力。 |
| 新增/profile | profile_spans | C++ | 0.455000 | 0.211700 | 2.15× | TX/C++ 启用真实分析器；Java/Python 仅保存时间段记录，没有同时启用 CPU/分配采样。仅为 API 负载参考，不能宣称完整 profiler 等价。 |
| 新增/profile | profile_spans | Java | 0.455000 | 0.995500 | 0.46× | TX/C++ 启用真实分析器；Java/Python 仅保存时间段记录，没有同时启用 CPU/分配采样。仅为 API 负载参考，不能宣称完整 profiler 等价。 |
| 新增/profile | profile_spans | Python | 0.455000 | 0.310700 | 1.46× | TX/C++ 启用真实分析器；Java/Python 仅保存时间段记录，没有同时启用 CPU/分配采样。仅为 API 负载参考，不能宣称完整 profiler 等价。 |
