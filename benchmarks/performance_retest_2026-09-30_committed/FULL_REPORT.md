# 提交后全量性能测试结果

测试提交：`da798cde02fdd8aba65393ea1d8df77850915dc3`。先提交，再正式重建工具链，随后串行执行所有既有性能套件。
采样时间（北京时间）：2026-09-30 13:09:55 至 13:18:27。
52/52 个公开模块有代表性能负载。新增标准库 29 项均为四语言各 5 轮；原有套件维持原轮数，包含语言、库、网络、外部题、综合、启动/编译/内存、优化专项及契约对照。
两道外部题全部 89 组正式答案核对通过；两套完成记录均确认采样期间源码与工具链指纹不变。

综合程序墙钟中位数：TX **112.139 ms**，C++ **58.476 ms**，倍率 **1.92×**。这不是全库平均倍率。

全部参考语言共有 65 条 ≥3× 记录，按名称去重 43 项；其中 C++ 对照 46 条、按名称去重 **36 项**。

## 对 C++ 达到 3× 的全部项目

同名有多组采样时，下表展示最高倍率及其对应套件，避免重复计数；主测与 GCC/clang 配对轮的完整记录见全部清单。临界值不能视为稳定超过阈值。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 可比边界 |
| --- | --- | --- | --- | --- | --- | --- |
| 新增/concurrency | mutex_uncontended | C++ | 20.630000 | 0.374300 | 55.12× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 语言特性 | deinit | C++ | 16.452000 | 0.720800 | 22.82× |  |
| 语言特性 | cycle_gc | C++ | 1.544000 | 0.073200 | 21.09× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 新增/diagnostics | test_property | C++ | 0.664000 | 0.047200 | 14.07× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 校准/diverse_clang | format_literal | clang C++ | 7.018000 | 0.617000 | 11.37× | C++ 直接拼接固定内容，不是通用格式器 |
| 语言特性 | deep_copy | C++ | 26.444000 | 2.327900 | 11.36× | 参考只复制已知形状；通用图复制另见新增契约 |
| 校准/diverse_clang | format_dynamic | clang C++ | 7.458000 | 0.903000 | 8.26× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| 校准/diverse | serde_short_text | GCC C++ | 7.057000 | 0.871000 | 8.10× |  |
| 外部题/mini-filesystem | random-2000-1 | C++ | 7.198000 | 0.989000 | 7.28× |  |
| 外部题/mini-filesystem | fanout-2000 | C++ | 12.302000 | 1.695000 | 7.26× |  |
| 语言特性 | variadic_unpack | C++ | 12.209000 | 1.720400 | 7.10× | 参考不执行 TX 动态命名实参绑定 |
| 新增/diagnostics | test_parameterized | C++ | 0.377000 | 0.062000 | 6.08× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 外部题/mini-filesystem | moves-2000 | C++ | 6.191000 | 1.028000 | 6.02× |  |
| audit/compute | bytes_hex | C++ | 11.710000 | 1.948000 | 6.01× |  |
| 语言特性 | string_conversion | C++ | 17.674000 | 2.980600 | 5.93× |  |
| 语言特性 | copy_cycle | C++ | 10.497000 | 1.803600 | 5.82× |  |
| audit/compute | regex_search | C++ | 63.799000 | 11.790000 | 5.41× |  |
| 契约/format_contract | format_parameter | C++ | 16.380000 | 3.262000 | 5.02× |  |
| 契约/format_contract | format_alternating | C++ | 16.046000 | 3.240000 | 4.95× |  |
| 新增/network | tls_handshake | C++ | 114.903000 | 24.220200 | 4.74× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.640000 | 0.356000 | 4.61× |  |
| audit/features | queue_push_pop | C++ | 1.323000 | 0.288000 | 4.59× |  |
| 综合 | encoding_literal | C++ | 12.816000 | 2.849000 | 4.50× |  |
| library/组合 | string | C++ | 10.000000 | 2.285900 | 4.37× | 整数毫秒短项，倍率精度有限 |
| 校准/diverse | encoding_dynamic | GCC C++ | 11.627000 | 2.718000 | 4.28× |  |
| 语言特性 | class_methods | C++ | 0.682000 | 0.163000 | 4.18× |  |
| audit/compute | random_int | C++ | 1.514000 | 0.362000 | 4.18× |  |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.258000 | 0.064000 | 4.03× |  |
| 长随机数 | random_long | C++ | 34.000000 | 8.805900 | 3.86× |  |
| audit/features | vector_push | C++ | 2.045000 | 0.549000 | 3.72× |  |
| 新增/sqlite | sqlite_pool | C++ | 139.236000 | 38.424900 | 3.62× | TX 与 C++ 测真实池；其 SQLite 归还策略为关闭重开。Java/Python 参考每次连接关闭，保持生命周期，未复制完整池等待/别名检查。 |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.011000 | 0.559000 | 3.60× |  |
| 语言特性 | class_operator | C++ | 0.838000 | 0.240900 | 3.48× |  |
| audit/features | iterator_snapshot | C++ | 0.165000 | 0.049000 | 3.37× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.836000 | 1.439000 | 3.36× |  |
| audit/compute | format_text | C++ | 2.687000 | 0.830000 | 3.24× |  |

## 完整数据

- [原有套件逐项报告](README.md)
- [新增标准库四语言逐项报告](../stdlib_retest_2026-09-30_committed/README.md)
- [全部 ≥3× 记录及语义边界](all_gaps.md)
- [52 模块覆盖映射](coverage.md)
- [与上一轮 TX 耗时比较](comparison.md)

通用图复制、确定性析构、动态实参绑定及 guard 生命周期的成本不能由简化 C++ 参考完全分离。
HTTP 为客户端端到端对照，数据库跨语言驱动及缓存策略不同；短项和网络测量受调度扰动，本轮不据此直接断言根因。
本轮未修改编译器或标准库实现，仅修正汇总脚本的历史假设并新增复测归档。
