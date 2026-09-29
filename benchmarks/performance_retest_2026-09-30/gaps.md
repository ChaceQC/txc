# 本轮 TX 耗时达到参考语言 3 倍的项目

语义限制：cycle_gc 的 C++/Java 参考仅处理本例单节点自环；deep_copy 的 C++/Java 参考只复制已知嵌套形状；variadic_unpack 的 C++ 不执行 TX 的动态实参绑定；Java deinit 以显式 close 模拟析构回调。这些倍率是现有负载的实测差异，不等于通用机制的等价性能差距。下表保留带说明的受限参考，便于完整审计，不能只按倍率排序决定优化优先级。

倍率 = TX 中位耗时 / 同轮参考中位耗时，使用未四舍五入值筛选 ≥3。单位 ms。不同套件的同名项目保留各自结果，不累计为独立热点。

无参考实现的 TX 专项无法判定跨语言倍率。零耗时不计算倍率；test_assert 的近零参考可以被优化消除，不能据此排名。标准库 dict 与 dict_hash/dict_dynamic 名称及实现不同，不强行配对。HTTP 客户端名称和实现不同，不强行配对。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 说明 |
| --- | --- | --- | ---: | ---: | ---: | --- |
| audit/compute | test_assert | Java | 6.401000 | 0.246000 | 26.02× | 参考循环可被优化消除，不作为有效热点排名 |
| 语言特性 | deinit | CPP | 14.486000 | 0.708600 | 20.44× |  |
| 语言特性 | cycle_gc | CPP | 1.465000 | 0.072400 | 20.23× |  |
| audit/compute | statistics_mean | C++ | 9.203000 | 0.535000 | 17.20× | 旧参考为普通求和；优先看校准补偿统计 |
| 综合 | serde_short_text | C++ | 11.316000 | 0.959000 | 11.80× |  |
| 语言特性 | variadic_unpack | Java | 8.807000 | 0.887900 | 9.92× |  |
| 校准/diverse_clang | serde_short_text | clang C++ | 10.060000 | 1.024000 | 9.82× |  |
| 语言特性 | deep_copy | CPP | 22.996000 | 2.369700 | 9.70× |  |
| 校准/diverse | format_literal | GCC C++ | 7.200000 | 0.787000 | 9.15× |  |
| 语言特性 | recursion | CPP | 0.607000 | 0.067300 | 9.02× |  |
| 语言特性 | deinit | Java | 14.486000 | 1.614600 | 8.97× |  |
| 综合 | format_literal | C++ | 6.302000 | 0.729000 | 8.64× |  |
| 语言特性 | runtime_cast | CPP | 15.810000 | 1.866500 | 8.47× |  |
| 校准/diverse | serde_short_text | GCC C++ | 10.439000 | 1.404000 | 7.44× |  |
| 校准/diverse_clang | format_literal | clang C++ | 6.421000 | 0.884000 | 7.26× |  |
| 校准/diverse | format_dynamic | GCC C++ | 6.851000 | 1.032000 | 6.64× |  |
| 语言特性 | runtime_cast | Java | 15.810000 | 2.395200 | 6.60× |  |
| 校准/diverse_clang | encoding_dynamic | clang C++ | 19.579000 | 3.071000 | 6.38× |  |
| 语言特性 | struct_operators | Java | 75.968000 | 12.162800 | 6.25× |  |
| 综合 | format_dynamic | C++ | 6.370000 | 1.045000 | 6.10× |  |
| 外部题/mini-filesystem | fanout-2000 | C++ | 9.848000 | 1.657000 | 5.94× |  |
| 语言特性 | variadic_unpack | CPP | 8.807000 | 1.532500 | 5.75× |  |
| 语言特性 | struct_operators | CPP | 75.968000 | 13.264700 | 5.73× |  |
| 综合 | serde_long_text | C++ | 28.168000 | 4.995000 | 5.64× |  |
| 校准/diverse_clang | serde_long_text | clang C++ | 31.405000 | 5.593000 | 5.62× |  |
| 综合 | encoding_literal | C++ | 15.706000 | 2.816000 | 5.58× |  |
| 外部题/mini-filesystem | random-2000-1 | C++ | 5.252000 | 0.943000 | 5.57× |  |
| 校准/diverse | serde_long_text | GCC C++ | 28.494000 | 5.136000 | 5.55× |  |
| 综合 | dictionary_text_hit | Java | 19.257000 | 3.485000 | 5.53× |  |
| 校准/diverse | encoding_dynamic | GCC C++ | 15.731000 | 2.896000 | 5.43× |  |
| 校准/diverse_clang | encoding_literal | clang C++ | 18.399000 | 3.396000 | 5.42× |  |
| 综合 | encoding_dynamic | C++ | 15.690000 | 2.897000 | 5.42× |  |
| 校准/diverse | encoding_literal | GCC C++ | 16.011000 | 2.973000 | 5.39× |  |
| 语言特性 | module_call | CPP | 17.639000 | 3.284600 | 5.37× |  |
| 语言特性 | copy_cycle | CPP | 10.677000 | 2.049200 | 5.21× |  |
| 外部题/mini-filesystem | moves-2000 | C++ | 4.738000 | 0.964000 | 4.91× |  |
| audit/features | queue_push_pop | C++ | 1.352000 | 0.294000 | 4.60× |  |
| 校准/diverse_clang | format_dynamic | clang C++ | 7.545000 | 1.675000 | 4.50× |  |
| 校准/diverse_clang | dictionary_text_hit | clang C++ | 19.571000 | 4.428000 | 4.42× |  |
| audit/compute | regex_search | Java | 38.285000 | 8.747000 | 4.38× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.710000 | 0.391000 | 4.37× |  |
| 综合 | dictionary_text_hit | C++ | 19.257000 | 4.428000 | 4.35× |  |
| 语言特性 | class_methods | CPP | 0.669000 | 0.161200 | 4.15× |  |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.265000 | 0.066000 | 4.02× |  |
| 校准/diverse | dictionary_text_hit | GCC C++ | 18.250000 | 4.569000 | 3.99× |  |
| audit/compute | random_int | C++ | 1.314000 | 0.330000 | 3.98× |  |
| 语言特性 | string_conversion | CPP | 12.683000 | 3.186300 | 3.98× |  |
| 长随机数 | random_long | C++ | 35.000000 | 8.838800 | 3.96× |  |
| 语言特性 | deep_copy | Java | 22.996000 | 6.223200 | 3.70× |  |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.274000 | 0.617000 | 3.69× |  |
| 语言特性 | deinit | Python | 14.486000 | 4.029200 | 3.60× |  |
| 语言特性 | class_operator | CPP | 0.856000 | 0.238800 | 3.58× |  |
| audit/compute | decimal_add | Java | 12.081000 | 3.420000 | 3.53× |  |
| audit/features | iterator_snapshot | C++ | 0.155000 | 0.044000 | 3.52× |  |
| audit/compute | regex_search | C++ | 38.285000 | 10.983000 | 3.49× |  |
| audit/compute | bytes_hex | C++ | 6.394000 | 1.846000 | 3.46× |  |
| audit/features | vector_push | C++ | 1.941000 | 0.569000 | 3.41× |  |
| 校准/statistics | statistics_mean | GCC C++ | 9.707000 | 2.925000 | 3.32× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.863000 | 1.472000 | 3.30× |  |
| audit/compute | regex_search | Python | 38.285000 | 12.011000 | 3.19× |  |
| 综合 | encoding_dynamic | Java | 15.690000 | 4.958000 | 3.16× |  |
| audit/compute | env_get | Java | 9.052000 | 2.906000 | 3.11× |  |
| 语言特性 | copy_cycle | Java | 10.677000 | 3.440600 | 3.10× |  |
| library/组合 | random | C++ | 1.000000 | 0.331500 | 3.02× | 整数毫秒短项，倍率精度有限 |
