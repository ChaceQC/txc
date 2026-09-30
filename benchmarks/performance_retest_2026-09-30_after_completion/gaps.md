# 本轮 TX 耗时达到参考语言三倍的项目

倍率为同轮 TX 中位耗时除以同轮参考中位耗时，按未四舍五入值筛选 ≥3。这里列出每个套件的原始倍率，参考工作量不同的项目保留说明，不能视为通用语言性能结论。

成功断言可被优化消除、旧简单求和参考及零耗时均不进入此表。HTTP 对 Java 的两个比例注明为端到端观察。没有参考实现的专项不计算跨语言倍率。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 说明 |
| --- | --- | --- | --- | --- | --- | --- |
| 语言特性 | deinit | C++ | 12.106000 | 0.685600 | 17.66× |  |
| 语言特性 | cycle_gc | C++ | 1.151000 | 0.065400 | 17.60× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 语言特性 | variadic_unpack | Java | 8.583000 | 0.895400 | 9.59× | 参考不执行 TX 动态命名实参绑定 |
| 语言特性 | deep_copy | C++ | 20.569000 | 2.198600 | 9.36× | 参考只复制已知形状；通用图复制另见新增契约 |
| 校准/diverse_clang | format_literal | clang C++ | 5.530000 | 0.609000 | 9.08× | C++ 直接拼接固定内容，不是通用格式器 |
| 校准/diverse_clang | serde_short_text | clang C++ | 7.894000 | 0.943000 | 8.37× |  |
| 综合 | serde_short_text | C++ | 7.237000 | 0.871000 | 8.31× |  |
| 校准/diverse | serde_short_text | GCC C++ | 7.532000 | 0.911000 | 8.27× |  |
| 语言特性 | deinit | Java | 12.106000 | 1.529200 | 7.92× | Java 显式 close 回调，不是 JVM 确定性析构 |
| 综合 | format_literal | C++ | 5.469000 | 0.771000 | 7.09× | C++ 直接拼接固定内容，不是通用格式器 |
| 校准/diverse | format_literal | GCC C++ | 5.729000 | 0.830000 | 6.90× | C++ 直接拼接固定内容，不是通用格式器 |
| 外部题/mini-filesystem | fanout-2000 | C++ | 10.047000 | 1.663000 | 6.04× |  |
| 校准/diverse_clang | format_dynamic | clang C++ | 5.397000 | 0.917000 | 5.89× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| 校准/diverse | serde_long_text | GCC C++ | 24.358000 | 4.342000 | 5.61× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 2.052000 | 0.367000 | 5.59× |  |
| 契约/format_contract | format_parameter | C++ | 17.295000 | 3.205000 | 5.40× |  |
| 外部题/mini-filesystem | random-2000-1 | C++ | 5.192000 | 0.964000 | 5.39× |  |
| 语言特性 | variadic_unpack | C++ | 8.583000 | 1.635700 | 5.25× | 参考不执行 TX 动态命名实参绑定 |
| 校准/diverse_clang | serde_long_text | clang C++ | 25.374000 | 4.889000 | 5.19× |  |
| 语言特性 | copy_cycle | C++ | 9.566000 | 1.846400 | 5.18× |  |
| 契约/format_contract | format_alternating | C++ | 16.818000 | 3.258000 | 5.16× |  |
| 综合 | format_dynamic | C++ | 5.212000 | 1.027000 | 5.07× | 局部不可变模板，不能代表真正动态格式化；C++ 仅实现本负载的两占位符格式 |
| 外部题/mini-filesystem | moves-2000 | C++ | 4.896000 | 0.977000 | 5.01× |  |
| audit/compute | regex_search | Java | 45.314000 | 9.107000 | 4.98× |  |
| 契约/graph_contract | graph_copy | C++ | 8.897000 | 1.792000 | 4.96× |  |
| 校准/diverse | format_dynamic | GCC C++ | 5.237000 | 1.067000 | 4.91× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| audit/features | queue_push_pop | C++ | 1.259000 | 0.266000 | 4.73× |  |
| 语言特性 | string_conversion | C++ | 13.364000 | 2.887300 | 4.63× |  |
| 综合 | serde_long_text | C++ | 24.103000 | 5.653000 | 4.26× |  |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.352000 | 0.561000 | 4.19× |  |
| audit/compute | decimal_add | Java | 11.954000 | 2.874000 | 4.16× |  |
| audit/compute | regex_search | C++ | 45.314000 | 11.082000 | 4.09× |  |
| 长随机数 | random_long | C++ | 34.000000 | 8.407300 | 4.04× |  |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.268000 | 0.067000 | 4.00× |  |
| audit/compute | env_get | Java | 9.019000 | 2.276000 | 3.96× |  |
| 语言特性 | class_methods | C++ | 0.609000 | 0.161100 | 3.78× |  |
| audit/compute | random_int | C++ | 1.335000 | 0.362000 | 3.69× |  |
| audit/features | vector_push | C++ | 1.821000 | 0.495000 | 3.68× |  |
| audit/compute | regex_search | Python | 45.314000 | 12.427000 | 3.65× |  |
| 语言特性 | deep_copy | Java | 20.569000 | 5.800300 | 3.55× | 参考只复制已知形状；通用图复制另见新增契约 |
| audit/compute | bytes_hex | C++ | 6.675000 | 1.894000 | 3.52× |  |
| 网络观察 | requests_get / http_get | Java | 965.789000 | 274.698000 | 3.52× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 网络观察 | httpx_get / http_get | Java | 965.279000 | 274.698000 | 3.51× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.866000 | 1.443000 | 3.37× |  |
| 语言特性 | class_operator | C++ | 0.784000 | 0.233000 | 3.36× |  |
| audit/features | iterator_snapshot | C++ | 0.161000 | 0.048000 | 3.35× |  |
| 校准/diverse | encoding_dynamic | GCC C++ | 9.239000 | 2.783000 | 3.32× |  |
| 综合 | encoding_dynamic | C++ | 8.514000 | 2.618000 | 3.25× |  |
| 校准/statistics | statistics_mean | GCC C++ | 9.235000 | 2.889000 | 3.20× |  |
| 综合 | encoding_literal | C++ | 8.619000 | 2.773000 | 3.11× |  |
| library/组合 | string | C++ | 7.000000 | 2.280800 | 3.07× | 整数毫秒短项，倍率精度有限 |
| 语言特性 | deinit | Python | 12.106000 | 3.992500 | 3.03× |  |

