# 本轮 TX 耗时达到参考语言三倍的项目

倍率为同轮 TX 中位耗时除以同轮参考中位耗时，按未四舍五入值筛选 ≥3。这里列出每个套件的原始倍率，参考工作量不同的项目保留说明，不能视为通用语言性能结论。

成功断言可被优化消除、旧简单求和参考及零耗时均不进入此表。HTTP 对 Java 的两个比例注明为端到端观察。没有参考实现的专项不计算跨语言倍率。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 说明 |
| --- | --- | --- | --- | --- | --- | --- |
| 语言特性 | deinit | C++ | 16.537000 | 0.697900 | 23.70× |  |
| 语言特性 | cycle_gc | C++ | 1.522000 | 0.069800 | 21.81× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 语言特性 | variadic_unpack | Java | 12.543000 | 0.987400 | 12.70× | 参考不执行 TX 动态命名实参绑定 |
| 语言特性 | deep_copy | C++ | 27.608000 | 2.379000 | 11.60× | 参考只复制已知形状；通用图复制另见新增契约 |
| 校准/diverse_clang | format_literal | clang C++ | 6.875000 | 0.638000 | 10.78× | C++ 直接拼接固定内容，不是通用格式器 |
| 校准/diverse | format_literal | GCC C++ | 7.529000 | 0.787000 | 9.57× | C++ 直接拼接固定内容，不是通用格式器 |
| 校准/diverse | serde_short_text | GCC C++ | 8.240000 | 0.902000 | 9.14× |  |
| 校准/diverse_clang | serde_short_text | clang C++ | 8.445000 | 0.947000 | 8.92× |  |
| 综合 | serde_short_text | C++ | 8.467000 | 0.956000 | 8.86× |  |
| 综合 | format_literal | C++ | 6.735000 | 0.801000 | 8.41× | C++ 直接拼接固定内容，不是通用格式器 |
| 语言特性 | deinit | Java | 16.537000 | 2.114100 | 7.82× | Java 显式 close 回调，不是 JVM 确定性析构 |
| 外部题/mini-filesystem | fanout-2000 | C++ | 13.140000 | 1.710000 | 7.68× |  |
| 语言特性 | variadic_unpack | C++ | 12.543000 | 1.692700 | 7.41× | 参考不执行 TX 动态命名实参绑定 |
| 外部题/mini-filesystem | random-2000-1 | C++ | 7.241000 | 0.980000 | 7.39× |  |
| 校准/diverse_clang | format_dynamic | clang C++ | 6.733000 | 0.934000 | 7.21× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| audit/compute | regex_search | Java | 54.007000 | 7.625000 | 7.08× |  |
| 综合 | format_dynamic | C++ | 7.248000 | 1.034000 | 7.01× | 局部不可变模板，不能代表真正动态格式化；C++ 仅实现本负载的两占位符格式 |
| audit/compute | decimal_add | Java | 19.500000 | 2.848000 | 6.85× |  |
| 契约/format_contract | format_parameter | C++ | 22.387000 | 3.349000 | 6.68× |  |
| 契约/format_contract | format_alternating | C++ | 22.498000 | 3.372000 | 6.67× |  |
| 校准/diverse | format_dynamic | GCC C++ | 6.880000 | 1.049000 | 6.56× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| 外部题/mini-filesystem | moves-2000 | C++ | 6.463000 | 0.994000 | 6.50× |  |
| 语言特性 | copy_cycle | C++ | 12.211000 | 1.903400 | 6.42× |  |
| library/组合 | random | C++ | 2.000000 | 0.333200 | 6.00× | 整数毫秒短项，倍率精度有限 |
| 校准/diverse | serde_long_text | GCC C++ | 26.069000 | 4.376000 | 5.96× |  |
| 语言特性 | string_conversion | C++ | 17.107000 | 2.961300 | 5.78× |  |
| 综合 | serde_long_text | C++ | 26.261000 | 4.557000 | 5.76× |  |
| 校准/diverse_clang | serde_long_text | clang C++ | 26.353000 | 4.953000 | 5.32× |  |
| audit/compute | bytes_hex | C++ | 9.441000 | 1.816000 | 5.20× |  |
| audit/compute | regex_search | C++ | 54.007000 | 10.604000 | 5.09× |  |
| audit/compute | env_get | Java | 14.391000 | 2.938000 | 4.90× |  |
| audit/compute | decimal_add | Python | 19.500000 | 4.086000 | 4.77× |  |
| audit/compute | parse_int | Java | 11.998000 | 2.529000 | 4.74× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.691000 | 0.357000 | 4.74× |  |
| audit/features | queue_push_pop | C++ | 1.389000 | 0.295000 | 4.71× |  |
| 校准/diverse | encoding_dynamic | GCC C++ | 12.218000 | 2.721000 | 4.49× |  |
| 校准/diverse | encoding_literal | GCC C++ | 12.933000 | 2.939000 | 4.40× |  |
| 契约/graph_contract | graph_copy | C++ | 10.086000 | 2.295000 | 4.39× |  |
| 综合 | encoding_literal | C++ | 12.564000 | 2.878000 | 4.37× |  |
| 校准/diverse_clang | encoding_dynamic | clang C++ | 12.516000 | 2.902000 | 4.31× |  |
| library/组合 | string | C++ | 10.000000 | 2.322000 | 4.31× | 整数毫秒短项，倍率精度有限 |
| 校准/diverse_clang | encoding_literal | clang C++ | 12.903000 | 2.999000 | 4.30× |  |
| 综合 | encoding_dynamic | C++ | 12.601000 | 2.945000 | 4.28× |  |
| 语言特性 | class_methods | C++ | 0.680000 | 0.165300 | 4.11× |  |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.261000 | 0.065000 | 4.02× |  |
| 长随机数 | random_long | C++ | 34.000000 | 8.508900 | 4.00× |  |
| 网络观察 | requests_get / http_get | Java | 1086.811000 | 275.237000 | 3.95× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| audit/compute | regex_search | Python | 54.007000 | 13.845000 | 3.90× |  |
| 语言特性 | deinit | Python | 16.537000 | 4.270500 | 3.87× |  |
| 语言特性 | deep_copy | Java | 27.608000 | 7.169000 | 3.85× | 参考只复制已知形状；通用图复制另见新增契约 |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.175000 | 0.571000 | 3.81× |  |
| audit/features | vector_push | C++ | 1.883000 | 0.502000 | 3.75× |  |
| 语言特性 | array_destructure | C++ | 4.066000 | 1.104900 | 3.68× |  |
| 校准/statistics | statistics_mean | GCC C++ | 10.653000 | 2.901000 | 3.67× |  |
| 网络观察 | httpx_get / http_get | Java | 1010.161000 | 275.237000 | 3.67× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| audit/compute | random_int | C++ | 1.357000 | 0.378000 | 3.59× |  |
| 语言特性 | variadic_unpack | Python | 12.543000 | 3.518000 | 3.57× |  |
| 外部题/mini-filesystem | moves-2000 | Python | 6.463000 | 1.891000 | 3.42× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.814000 | 1.424000 | 3.38× |  |
| 语言特性 | class_operator | C++ | 0.808000 | 0.240900 | 3.35× |  |
| 外部题/mini-filesystem | random-2000-1 | JavaScript | 7.241000 | 2.182000 | 3.32× |  |
| 外部题/mini-filesystem | moves-2000 | JavaScript | 6.463000 | 1.954000 | 3.31× |  |
| audit/features | iterator_snapshot | C++ | 0.163000 | 0.053000 | 3.08× |  |
| 外部题/mini-filesystem | fanout-2000 | Python | 13.140000 | 4.361000 | 3.01× |  |

