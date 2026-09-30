# 本轮 TX 耗时达到参考语言三倍的项目

倍率为同轮 TX 中位耗时除以同轮参考中位耗时，按未四舍五入值筛选 ≥3。这里列出每个套件的原始倍率，参考工作量不同的项目保留说明，不能视为通用语言性能结论。

成功断言可被优化消除、旧简单求和参考及零耗时均不进入此表。HTTP 对 Java 的两个比例注明为端到端观察。没有参考实现的专项不计算跨语言倍率。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 说明 |
| --- | --- | --- | --- | --- | --- | --- |
| 语言特性 | deinit | C++ | 16.452000 | 0.720800 | 22.82× |  |
| 语言特性 | cycle_gc | C++ | 1.544000 | 0.073200 | 21.09× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 语言特性 | variadic_unpack | Java | 12.209000 | 0.992100 | 12.31× | 参考不执行 TX 动态命名实参绑定 |
| 校准/diverse_clang | format_literal | clang C++ | 7.018000 | 0.617000 | 11.37× | C++ 直接拼接固定内容，不是通用格式器 |
| 语言特性 | deep_copy | C++ | 26.444000 | 2.327900 | 11.36× | 参考只复制已知形状；通用图复制另见新增契约 |
| 综合 | format_literal | C++ | 7.371000 | 0.757000 | 9.74× | C++ 直接拼接固定内容，不是通用格式器 |
| 校准/diverse | format_literal | GCC C++ | 7.327000 | 0.767000 | 9.55× | C++ 直接拼接固定内容，不是通用格式器 |
| 语言特性 | deinit | Java | 16.452000 | 1.772100 | 9.28× | Java 显式 close 回调，不是 JVM 确定性析构 |
| 校准/diverse_clang | format_dynamic | clang C++ | 7.458000 | 0.903000 | 8.26× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| 校准/diverse | serde_short_text | GCC C++ | 7.057000 | 0.871000 | 8.10× |  |
| 综合 | serde_short_text | C++ | 6.964000 | 0.904000 | 7.70× |  |
| 校准/diverse_clang | serde_short_text | clang C++ | 6.862000 | 0.929000 | 7.39× |  |
| 校准/diverse | format_dynamic | GCC C++ | 7.514000 | 1.022000 | 7.35× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| 外部题/mini-filesystem | random-2000-1 | C++ | 7.198000 | 0.989000 | 7.28× |  |
| 外部题/mini-filesystem | fanout-2000 | C++ | 12.302000 | 1.695000 | 7.26× |  |
| 综合 | format_dynamic | C++ | 7.537000 | 1.043000 | 7.23× | 局部不可变模板，不能代表真正动态格式化；C++ 仅实现本负载的两占位符格式 |
| 语言特性 | variadic_unpack | C++ | 12.209000 | 1.720400 | 7.10× | 参考不执行 TX 动态命名实参绑定 |
| 外部题/mini-filesystem | moves-2000 | C++ | 6.191000 | 1.028000 | 6.02× |  |
| audit/compute | bytes_hex | C++ | 11.710000 | 1.948000 | 6.01× |  |
| 语言特性 | string_conversion | C++ | 17.674000 | 2.980600 | 5.93× |  |
| 语言特性 | copy_cycle | C++ | 10.497000 | 1.803600 | 5.82× |  |
| audit/compute | regex_search | Java | 63.799000 | 11.457000 | 5.57× |  |
| audit/compute | regex_search | C++ | 63.799000 | 11.790000 | 5.41× |  |
| audit/compute | regex_search | Python | 63.799000 | 12.226000 | 5.22× |  |
| 契约/format_contract | format_parameter | C++ | 16.380000 | 3.262000 | 5.02× |  |
| 契约/format_contract | format_alternating | C++ | 16.046000 | 3.240000 | 4.95× |  |
| audit/compute | env_get | Java | 17.033000 | 3.502000 | 4.86× |  |
| audit/compute | parse_int | Java | 14.054000 | 2.995000 | 4.69× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.640000 | 0.356000 | 4.61× |  |
| audit/features | queue_push_pop | C++ | 1.323000 | 0.288000 | 4.59× |  |
| 综合 | encoding_literal | C++ | 12.816000 | 2.849000 | 4.50× |  |
| library/组合 | string | C++ | 10.000000 | 2.285900 | 4.37× | 整数毫秒短项，倍率精度有限 |
| 校准/diverse | encoding_dynamic | GCC C++ | 11.627000 | 2.718000 | 4.28× |  |
| 校准/diverse | encoding_literal | GCC C++ | 12.220000 | 2.896000 | 4.22× |  |
| 语言特性 | class_methods | C++ | 0.682000 | 0.163000 | 4.18× |  |
| audit/compute | random_int | C++ | 1.514000 | 0.362000 | 4.18× |  |
| 综合 | encoding_dynamic | C++ | 11.586000 | 2.804000 | 4.13× |  |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.258000 | 0.064000 | 4.03× |  |
| 校准/diverse_clang | encoding_dynamic | clang C++ | 11.880000 | 2.951000 | 4.03× |  |
| 语言特性 | deep_copy | Java | 26.444000 | 6.766300 | 3.91× | 参考只复制已知形状；通用图复制另见新增契约 |
| audit/compute | decimal_add | Java | 14.592000 | 3.765000 | 3.88× |  |
| 校准/diverse_clang | encoding_literal | clang C++ | 11.625000 | 3.003000 | 3.87× |  |
| 长随机数 | random_long | C++ | 34.000000 | 8.805900 | 3.86× |  |
| 语言特性 | deinit | Python | 16.452000 | 4.341200 | 3.79× |  |
| audit/features | vector_push | C++ | 2.045000 | 0.549000 | 3.72× |  |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.011000 | 0.559000 | 3.60× |  |
| 网络观察 | requests_get / http_get | Java | 1081.947000 | 306.341000 | 3.53× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 语言特性 | class_operator | C++ | 0.838000 | 0.240900 | 3.48× |  |
| audit/features | iterator_snapshot | C++ | 0.165000 | 0.049000 | 3.37× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.836000 | 1.439000 | 3.36× |  |
| 语言特性 | variadic_unpack | Python | 12.209000 | 3.666100 | 3.33× |  |
| 网络观察 | httpx_get / http_get | Java | 997.729000 | 306.341000 | 3.26× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| audit/compute | format_text | C++ | 2.687000 | 0.830000 | 3.24× |  |
| audit/compute | bytes_hex | Java | 11.710000 | 3.637000 | 3.22× |  |
| audit/compute | decimal_add | Python | 14.592000 | 4.648000 | 3.14× |  |
| 外部题/mini-filesystem | random-2000-1 | JavaScript | 7.198000 | 2.297000 | 3.13× |  |
| 外部题/mini-filesystem | fanout-2000 | Python | 12.302000 | 4.087000 | 3.01× |  |

