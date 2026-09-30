# 本轮 TX 耗时达到参考语言三倍的项目

倍率为同轮 TX 中位耗时除以同轮参考中位耗时，按未四舍五入值筛选 ≥3。这里列出每个套件的原始倍率，参考工作量不同的项目保留说明，不能视为通用语言性能结论。

成功断言可被优化消除、旧简单求和参考及零耗时均不进入此表。HTTP 对 Java 的两个比例注明为端到端观察。没有参考实现的专项不计算跨语言倍率。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 说明 |
| --- | --- | --- | --- | --- | --- | --- |
| 语言特性 | cycle_gc | C++ | 1.449000 | 0.073900 | 19.61× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 语言特性 | deinit | C++ | 11.408000 | 0.733200 | 15.56× |  |
| 语言特性 | variadic_unpack | Java | 11.646000 | 0.928100 | 12.55× | 参考不执行 TX 动态命名实参绑定 |
| 语言特性 | deep_copy | C++ | 20.827000 | 2.343400 | 8.89× | 参考只复制已知形状；通用图复制另见新增契约 |
| 语言特性 | variadic_unpack | C++ | 11.646000 | 1.639000 | 7.11× | 参考不执行 TX 动态命名实参绑定 |
| 外部题/mini-filesystem | fanout-2000 | C++ | 12.817000 | 1.826000 | 7.02× |  |
| 综合 | serde_short_text | C++ | 6.436000 | 0.940000 | 6.85× |  |
| audit/compute | parse_int | Java | 11.754000 | 1.759000 | 6.68× |  |
| 校准/diverse_clang | serde_short_text | clang C++ | 6.246000 | 0.973000 | 6.42× |  |
| 校准/diverse | serde_short_text | GCC C++ | 6.050000 | 0.946000 | 6.40× |  |
| 外部题/mini-filesystem | random-2000-1 | C++ | 5.999000 | 0.971000 | 6.18× |  |
| 语言特性 | deinit | Java | 11.408000 | 1.857800 | 6.14× | Java 显式 close 回调，不是 JVM 确定性析构 |
| audit/compute | env_get | Java | 10.014000 | 1.836000 | 5.45× |  |
| 契约/format_contract | format_alternating | C++ | 17.156000 | 3.237000 | 5.30× |  |
| 契约/format_contract | format_parameter | C++ | 17.288000 | 3.264000 | 5.30× |  |
| 外部题/mini-filesystem | moves-2000 | C++ | 5.232000 | 1.006000 | 5.20× |  |
| audit/compute | bytes_hex | C++ | 8.792000 | 1.832000 | 4.80× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.583000 | 0.348000 | 4.55× |  |
| 语言特性 | copy_cycle | C++ | 8.561000 | 1.919800 | 4.46× |  |
| 校准/diverse_clang | vector_scan_100k | clang C++ | 3.554000 | 0.809000 | 4.39× |  |
| audit/compute | decimal_add | Java | 11.036000 | 2.544000 | 4.34× |  |
| 网络观察 | requests_get / http_get | Java | 977.079000 | 235.224000 | 4.15× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 网络观察 | httpx_get / http_get | Java | 962.718000 | 235.224000 | 4.09× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.256000 | 0.064000 | 4.00× |  |
| 语言特性 | class_methods | C++ | 0.652000 | 0.163100 | 4.00× |  |
| audit/features | vector_push | C++ | 1.942000 | 0.500000 | 3.88× |  |
| library/组合 | string | C++ | 10.000000 | 2.586900 | 3.87× | 整数毫秒短项，倍率精度有限 |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.006000 | 0.556000 | 3.61× |  |
| audit/features | queue_push_pop | C++ | 1.165000 | 0.324000 | 3.60× |  |
| 语言特性 | string_conversion | C++ | 11.303000 | 3.180800 | 3.55× |  |
| audit/features | iterator_snapshot | C++ | 0.164000 | 0.047000 | 3.49× |  |
| 语言特性 | variadic_unpack | Python | 11.646000 | 3.589900 | 3.24× |  |
| audit/compute | random_int | C++ | 0.998000 | 0.327000 | 3.05× |  |
| 外部题/mini-filesystem | random-2000-1 | JavaScript | 5.999000 | 1.977000 | 3.03× |  |
| 校准/diverse | vector_scan_100k | GCC C++ | 3.524000 | 1.162000 | 3.03× |  |
| 语言特性 | deep_copy | Java | 20.827000 | 6.883600 | 3.03× | 参考只复制已知形状；通用图复制另见新增契约 |
| 综合 | vector_scan_100k | C++ | 3.584000 | 1.187000 | 3.02× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.353000 | 1.442000 | 3.02× |  |
