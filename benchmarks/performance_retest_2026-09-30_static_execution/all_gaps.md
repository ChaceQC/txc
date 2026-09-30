# 本轮全部 ≥3× 差距

倍率 = 同组 TX 中位耗时 / 参考中位耗时，按未四舍五入的比值筛选 ≥3。
同名项目在不同套件/语言中的记录不累计为独立热点；保留工作量和语义边界。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 可比边界 |
| --- | --- | --- | --- | --- | --- | --- |
| 新增/concurrency | mutex_uncontended | C++ | 20.517000 | 0.481100 | 42.65× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| audit/features | iterator_snapshot | C++ | 1.468000 | 0.046000 | 31.91× |  |
| 语言特性 | deinit | C++ | 15.378000 | 0.707100 | 21.75× |  |
| 语言特性 | cycle_gc | C++ | 1.542000 | 0.073100 | 21.09× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 语言特性 | variadic_unpack | Java | 11.229000 | 0.965900 | 11.63× | 参考不执行 TX 动态命名实参绑定 |
| 语言特性 | deep_copy | C++ | 25.082000 | 2.342900 | 10.71× | 参考只复制已知形状；通用图复制另见新增契约 |
| 综合 | serde_short_text | C++ | 9.825000 | 0.976000 | 10.07× |  |
| audit/features | heap_push_pop | C++ | 29.891000 | 3.029000 | 9.87× |  |
| 校准/diverse_clang | format_literal | clang C++ | 6.346000 | 0.647000 | 9.81× | C++ 直接拼接固定内容，不是通用格式器 |
| 综合 | format_literal | C++ | 6.574000 | 0.782000 | 8.41× | C++ 直接拼接固定内容，不是通用格式器 |
| 校准/heap | heap_push_pop | GCC C++ | 29.625000 | 3.656000 | 8.10× |  |
| 校准/diverse | format_literal | GCC C++ | 6.306000 | 0.782000 | 8.06× | C++ 直接拼接固定内容，不是通用格式器 |
| 语言特性 | deinit | Java | 15.378000 | 1.970700 | 7.80× | Java 显式 close 回调，不是 JVM 确定性析构 |
| 校准/diverse_clang | serde_short_text | clang C++ | 7.818000 | 1.025000 | 7.63× |  |
| 校准/heap_clang | heap_push_pop | clang C++ | 34.858000 | 4.759000 | 7.32× |  |
| 新增/diagnostics | test_property | C++ | 0.334000 | 0.046500 | 7.18× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 校准/diverse | serde_short_text | GCC C++ | 6.943000 | 0.975000 | 7.12× |  |
| 语言特性 | variadic_unpack | C++ | 11.229000 | 1.608500 | 6.98× | 参考不执行 TX 动态命名实参绑定 |
| 校准/diverse_clang | format_dynamic | clang C++ | 6.229000 | 0.919000 | 6.78× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| 外部题/mini-filesystem | fanout-2000 | C++ | 11.182000 | 1.671000 | 6.69× |  |
| 综合 | format_dynamic | C++ | 6.923000 | 1.041000 | 6.65× | 局部不可变模板，不能代表真正动态格式化；C++ 仅实现本负载的两占位符格式 |
| 外部题/mini-filesystem | random-2000-1 | C++ | 6.399000 | 0.997000 | 6.42× |  |
| 新增/sqlite | sqlite_insert | Python | 11.422000 | 1.914800 | 5.97× | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| 校准/diverse | format_dynamic | GCC C++ | 6.219000 | 1.044000 | 5.96× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| audit/compute | parse_int | Java | 11.408000 | 1.923000 | 5.93× |  |
| 新增/network | tls_handshake | C++ | 144.254000 | 25.348400 | 5.69× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 契约/format_contract | format_parameter | C++ | 17.913000 | 3.239000 | 5.53× |  |
| 契约/format_contract | format_alternating | C++ | 17.629000 | 3.262000 | 5.40× |  |
| 外部题/mini-filesystem | moves-2000 | C++ | 5.442000 | 1.017000 | 5.35× |  |
| 语言特性 | copy_cycle | C++ | 9.302000 | 1.793300 | 5.19× |  |
| 语言特性 | string_conversion | C++ | 16.576000 | 3.197800 | 5.18× |  |
| audit/compute | bytes_hex | C++ | 8.617000 | 1.837000 | 4.69× |  |
| audit/compute | env_get | Java | 9.844000 | 2.155000 | 4.57× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.552000 | 0.356000 | 4.36× |  |
| 综合 | encoding_literal | C++ | 12.491000 | 2.866000 | 4.36× |  |
| audit/compute | random_int | C++ | 1.390000 | 0.322000 | 4.32× |  |
| 校准/diverse | encoding_dynamic | GCC C++ | 11.300000 | 2.664000 | 4.24× |  |
| 校准/diverse | encoding_literal | GCC C++ | 11.582000 | 2.806000 | 4.13× |  |
| 语言特性 | class_methods | C++ | 0.662000 | 0.161100 | 4.11× |  |
| 长随机数 | random_long | C++ | 41.000000 | 10.104800 | 4.06× |  |
| 综合 | encoding_dynamic | C++ | 11.221000 | 2.785000 | 4.03× |  |
| 新增/sqlite | sqlite_savepoint | Python | 2.622000 | 0.653100 | 4.01× | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| audit/features | vector_push | C++ | 1.938000 | 0.484000 | 4.00× |  |
| 新增/diagnostics | test_parameterized | C++ | 0.223000 | 0.056200 | 3.97× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.261000 | 0.066000 | 3.95× |  |
| 语言特性 | deinit | Python | 15.378000 | 3.934000 | 3.91× |  |
| 校准/diverse_clang | encoding_dynamic | clang C++ | 11.079000 | 2.845000 | 3.89× |  |
| 网络观察 | requests_get / http_get | Java | 981.354000 | 253.905000 | 3.87× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 网络观察 | httpx_get / http_get | Java | 975.974000 | 253.905000 | 3.84× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| audit/features | queue_push_pop | C++ | 1.119000 | 0.292000 | 3.83× |  |
| 语言特性 | deep_copy | Java | 25.082000 | 6.677200 | 3.76× | 参考只复制已知形状；通用图复制另见新增契约 |
| 校准/diverse_clang | encoding_literal | clang C++ | 11.720000 | 3.227000 | 3.63× |  |
| audit/compute | decimal_add | Java | 10.978000 | 3.133000 | 3.50× |  |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.015000 | 0.576000 | 3.50× |  |
| audit/features | heap_push_pop | Java | 29.891000 | 8.579000 | 3.48× |  |
| library/组合 | string | C++ | 9.000000 | 2.606800 | 3.45× | 整数毫秒短项，倍率精度有限 |
| 新增/concurrency | mutex_uncontended | Java | 20.517000 | 6.021400 | 3.41× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 契约/graph_contract | graph_copy | C++ | 7.334000 | 2.315000 | 3.17× |  |
| 综合 | dictionary_int_hit | C++ | 2.364000 | 0.754000 | 3.14× |  |
| 新增/sqlite | sqlite_pool | C++ | 137.792000 | 44.204900 | 3.12× | TX 与 C++ 测真实池；其 SQLite 归还策略为关闭重开。Java/Python 参考每次连接关闭，保持生命周期，未复制完整池等待/别名检查。 |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.299000 | 1.418000 | 3.03× |  |
