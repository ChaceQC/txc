# 本轮全部 ≥3× 差距

倍率 = 同组 TX 中位耗时 / 参考中位耗时，按未四舍五入的比值筛选 ≥3。
同名项目在不同套件/语言中的记录不累计为独立热点；保留工作量和语义边界。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 可比边界 |
| --- | --- | --- | --- | --- | --- | --- |
| 语言特性 | cycle_gc | C++ | 1.449000 | 0.073900 | 19.61× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 语言特性 | deinit | C++ | 11.408000 | 0.733200 | 15.56× |  |
| 语言特性 | variadic_unpack | Java | 11.646000 | 0.928100 | 12.55× | 参考不执行 TX 动态命名实参绑定 |
| 语言特性 | deep_copy | C++ | 20.827000 | 2.343400 | 8.89× | 参考只复制已知形状；通用图复制另见新增契约 |
| 语言特性 | variadic_unpack | C++ | 11.646000 | 1.639000 | 7.11× | 参考不执行 TX 动态命名实参绑定 |
| 外部题/mini-filesystem | fanout-2000 | C++ | 12.817000 | 1.826000 | 7.02× |  |
| 综合 | serde_short_text | C++ | 6.436000 | 0.940000 | 6.85× |  |
| audit/compute | parse_int | Java | 11.754000 | 1.759000 | 6.68× |  |
| 新增/diagnostics | test_property | C++ | 0.304000 | 0.046100 | 6.59× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 校准/diverse_clang | serde_short_text | clang C++ | 6.246000 | 0.973000 | 6.42× |  |
| 校准/diverse | serde_short_text | GCC C++ | 6.050000 | 0.946000 | 6.40× |  |
| 外部题/mini-filesystem | random-2000-1 | C++ | 5.999000 | 0.971000 | 6.18× |  |
| 语言特性 | deinit | Java | 11.408000 | 1.857800 | 6.14× | Java 显式 close 回调，不是 JVM 确定性析构 |
| 新增/sqlite | sqlite_insert | Python | 11.867000 | 1.946400 | 6.10× | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| audit/compute | env_get | Java | 10.014000 | 1.836000 | 5.45× |  |
| 契约/format_contract | format_alternating | C++ | 17.156000 | 3.237000 | 5.30× |  |
| 契约/format_contract | format_parameter | C++ | 17.288000 | 3.264000 | 5.30× |  |
| 外部题/mini-filesystem | moves-2000 | C++ | 5.232000 | 1.006000 | 5.20× |  |
| audit/compute | bytes_hex | C++ | 8.792000 | 1.832000 | 4.80× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.583000 | 0.348000 | 4.55× |  |
| 语言特性 | copy_cycle | C++ | 8.561000 | 1.919800 | 4.46× |  |
| 校准/diverse_clang | vector_scan_100k | clang C++ | 3.554000 | 0.809000 | 4.39× |  |
| audit/compute | decimal_add | Java | 11.036000 | 2.544000 | 4.34× |  |
| 新增/sqlite | sqlite_savepoint | Python | 2.827000 | 0.655100 | 4.32× | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| 网络观察 | requests_get / http_get | Java | 977.079000 | 235.224000 | 4.15× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 网络观察 | httpx_get / http_get | Java | 962.718000 | 235.224000 | 4.09× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
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
| 语言特性 | variadic_unpack | Python | 11.646000 | 3.589900 | 3.24× |  |
| audit/compute | random_int | C++ | 0.998000 | 0.327000 | 3.05× |  |
| 外部题/mini-filesystem | random-2000-1 | JavaScript | 5.999000 | 1.977000 | 3.03× |  |
| 校准/diverse | vector_scan_100k | GCC C++ | 3.524000 | 1.162000 | 3.03× |  |
| 语言特性 | deep_copy | Java | 20.827000 | 6.883600 | 3.03× | 参考只复制已知形状；通用图复制另见新增契约 |
| 综合 | vector_scan_100k | C++ | 3.584000 | 1.187000 | 3.02× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.353000 | 1.442000 | 3.02× |  |
