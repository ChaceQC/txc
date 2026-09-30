# 静态执行体系提交后的全量性能对比报告

测试提交：`f1cc9a5368d01d8a9b413facf2495af8f63e5c57`。先提交，再正式重建工具链，随后串行执行所有既有性能套件。
采样时间（北京时间）：2026-09-30 19:36:01 至 19:45:12。
52/52 个公开模块有代表性能负载。新增标准库 29 项均为四语言各 5 轮；原有套件维持原轮数，包含语言、库、网络、外部题、综合、启动/编译/内存、优化专项及契约对照。
两道外部题全部 89 组正式答案核对通过；两套完成记录均确认采样期间源码与工具链指纹不变。

综合程序墙钟中位数：TX **122.547 ms**，C++ **70.008 ms**，倍率 **1.75×**。这不是全库平均倍率。

全部参考语言共有 61 条 ≥3× 记录，按名称去重 43 项；其中 C++ 对照 48 条、按名称去重 **36 项**。

## 对 C++ 达到 3× 的全部项目

同名有多组采样时，下表展示最高倍率及其对应套件，避免重复计数；主测与 GCC/clang 配对轮的完整记录见全部清单。临界值不能视为稳定超过阈值。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 可比边界 |
| --- | --- | --- | --- | --- | --- | --- |
| 新增/concurrency | mutex_uncontended | C++ | 20.517000 | 0.481100 | 42.65× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| audit/features | iterator_snapshot | C++ | 1.468000 | 0.046000 | 31.91× |  |
| 语言特性 | deinit | C++ | 15.378000 | 0.707100 | 21.75× |  |
| 语言特性 | cycle_gc | C++ | 1.542000 | 0.073100 | 21.09× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 语言特性 | deep_copy | C++ | 25.082000 | 2.342900 | 10.71× | 参考只复制已知形状；通用图复制另见新增契约 |
| 综合 | serde_short_text | C++ | 9.825000 | 0.976000 | 10.07× |  |
| audit/features | heap_push_pop | C++ | 29.891000 | 3.029000 | 9.87× |  |
| 校准/diverse_clang | format_literal | clang C++ | 6.346000 | 0.647000 | 9.81× | C++ 直接拼接固定内容，不是通用格式器 |
| 新增/diagnostics | test_property | C++ | 0.334000 | 0.046500 | 7.18× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 语言特性 | variadic_unpack | C++ | 11.229000 | 1.608500 | 6.98× | 参考不执行 TX 动态命名实参绑定 |
| 校准/diverse_clang | format_dynamic | clang C++ | 6.229000 | 0.919000 | 6.78× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| 外部题/mini-filesystem | fanout-2000 | C++ | 11.182000 | 1.671000 | 6.69× |  |
| 外部题/mini-filesystem | random-2000-1 | C++ | 6.399000 | 0.997000 | 6.42× |  |
| 新增/network | tls_handshake | C++ | 144.254000 | 25.348400 | 5.69× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 契约/format_contract | format_parameter | C++ | 17.913000 | 3.239000 | 5.53× |  |
| 契约/format_contract | format_alternating | C++ | 17.629000 | 3.262000 | 5.40× |  |
| 外部题/mini-filesystem | moves-2000 | C++ | 5.442000 | 1.017000 | 5.35× |  |
| 语言特性 | copy_cycle | C++ | 9.302000 | 1.793300 | 5.19× |  |
| 语言特性 | string_conversion | C++ | 16.576000 | 3.197800 | 5.18× |  |
| audit/compute | bytes_hex | C++ | 8.617000 | 1.837000 | 4.69× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.552000 | 0.356000 | 4.36× |  |
| 综合 | encoding_literal | C++ | 12.491000 | 2.866000 | 4.36× |  |
| audit/compute | random_int | C++ | 1.390000 | 0.322000 | 4.32× |  |
| 校准/diverse | encoding_dynamic | GCC C++ | 11.300000 | 2.664000 | 4.24× |  |
| 语言特性 | class_methods | C++ | 0.662000 | 0.161100 | 4.11× |  |
| 长随机数 | random_long | C++ | 41.000000 | 10.104800 | 4.06× |  |
| audit/features | vector_push | C++ | 1.938000 | 0.484000 | 4.00× |  |
| 新增/diagnostics | test_parameterized | C++ | 0.223000 | 0.056200 | 3.97× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.261000 | 0.066000 | 3.95× |  |
| audit/features | queue_push_pop | C++ | 1.119000 | 0.292000 | 3.83× |  |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.015000 | 0.576000 | 3.50× |  |
| library/组合 | string | C++ | 9.000000 | 2.606800 | 3.45× | 整数毫秒短项，倍率精度有限 |
| 契约/graph_contract | graph_copy | C++ | 7.334000 | 2.315000 | 3.17× |  |
| 综合 | dictionary_int_hit | C++ | 2.364000 | 0.754000 | 3.14× |  |
| 新增/sqlite | sqlite_pool | C++ | 137.792000 | 44.204900 | 3.12× | TX 与 C++ 测真实池；其 SQLite 归还策略为关闭重开。Java/Python 参考每次连接关闭，保持生命周期，未复制完整池等待/别名检查。 |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.299000 | 1.418000 | 3.03× |  |

## 完整数据

- [原有套件逐项报告](README.md)
- [新增标准库四语言逐项报告](../stdlib_retest_2026-09-30_static_execution/README.md)
- [全部 ≥3× 记录及语义边界](all_gaps.md)
- [52 模块覆盖映射](coverage.md)
- [与上一轮 TX 耗时比较](comparison.md)

通用图复制、确定性析构、动态实参绑定及 guard 生命周期的成本不能由简化 C++ 参考完全分离。
HTTP 为客户端端到端对照，数据库跨语言驱动及缓存策略不同；短项和网络测量受调度扰动，本轮不据此直接断言根因。
正式采样前补齐 ThinLTO emutls 配置，密码学初始化改用线程安全局部静态对象；头文件引入 call_once 外部 TLS 的编译单元保留原生对象。详见 [构建预检](preflight_failure.md)。

## 测量方法及完整性

采样从 2026-09-30 19:36:01 至 2026-09-30 19:45:12（北京时间），共 9.20 分钟。
默认 TX 程序使用 clang -O3、ThinLTO 及 ld.lld；C++ 参考使用现有 -O3 Release 配置，校准组分别使用 GCC 和同版本 clang。C++ ThinLTO 编译使用 -femulated-tls，链接使用 --plugin-opt=-emulated-tls；密码学一次性初始化使用线程安全局部静态对象。ws_client.cpp 因 libstdc++ 外部 TLS 依赖保留 GCC Release 原生对象；标准库其余 316 个模块使用 bitcode，第三方依赖继续使用原生对象。图契约直接链接普通 TX 运行时静态库。
本轮额外记录 ThinLTO 静态库、LLVM bitcode、ld.lld 和启动对象的 SHA-256，两套采样完成后均检查输入指纹不变；全部数据使用本轮结果，不混入历史校准值。
跨语言倍率 = 同组 TX 中位耗时 / 对应参考中位耗时，按未四舍五入值筛选 ≥3；小于 1 表示 TX 更快。启动、编译和内存另列，避免与内部耗时混算。
全量指执行仓库全部现有性能套件，以及覆盖 52 个公开标准库模块的代表负载。不表示每个 API、失败路径、并发饱和吞吐或生产环境均被测量。

## 进程、启动、编译和内存的跨语言倍率

内存为每 2 ms 轮询进程树工作集的近似峰值；缺失或零值不计算倍率。TX check 与完整编译、Python 字节码与原生编译的工作不同，保留在逐项表中，不计算对应倍率。

| 范围 | 指标 | TX | 参考 | 参考值 | TX/参考 | ≥3× |
| --- | --- | --- | --- | --- | --- | --- |
| 进程运行 | 墙钟 ms | 122.547 | C++ | 70.008 | 1.75× |  |
| 进程运行 | 墙钟 ms | 122.547 | Java | 976.313 | 0.13× |  |
| 进程运行 | 墙钟 ms | 122.547 | Python | 810.873 | 0.15× |  |
| 进程运行 | 峰值 MiB | 9.062 | C++ | 5.691 | 1.59× |  |
| 进程运行 | 峰值 MiB | 9.062 | Java | 186.375 | 0.05× |  |
| 进程运行 | 峰值 MiB | 9.062 | Python | 14.465 | 0.63× |  |
| 空程序启动 | 墙钟 ms | 46.548 | C++ | 26.565 | 1.75× |  |
| 空程序启动 | 墙钟 ms | 46.548 | Java | 212.732 | 0.22× |  |
| 空程序启动 | 墙钟 ms | 46.548 | Python | 67.653 | 0.69× |  |
| 空程序启动 | 峰值 MiB | 6.168 | C++ | — | — |  |
| 空程序启动 | 峰值 MiB | 6.168 | Java | 52.188 | 0.12× |  |
| 空程序启动 | 峰值 MiB | 6.168 | Python | 11.609 | 0.53× |  |
| 编译 | 墙钟 ms | 1691.320 | C++ full | 4743.954 | 0.36× |  |
| 编译 | 墙钟 ms | 1691.320 | Java full | 1230.949 | 1.37× |  |
| 编译 | 峰值 MiB | 186.004 | C++ full | 230.703 | 0.81× |  |
| 编译 | 峰值 MiB | 186.004 | Java full | 150.996 | 1.23× |  |

## 全部既有套件逐项对比

### 覆盖与采样

| 套件 | 范围 | 预热 / 正式轮数 |
| --- | --- | --- |
| 语言特性 | 22 项，TX/C++/Python/Java | 1 / 7，交替顺序 |
| 标准库组合 | 10 项 TX；C++ 字典另有两种参考 | 1 / 7，交替顺序 |
| 模块与新增特性 | 21 项计算、10 项特性、6 项系统；另测 Python/Java 历史组合 | 1 / 3，交替顺序 |
| 本机网络 | HTTP、Requests、WebSocket 共 3 项 TX | 1 / 3，交替顺序 |
| 外部题 | 2 题各 5 组，5 种语言；普通 TX 程序核对全部 89 组正式数据 | 2 / 17，轮换顺序 |
| 综合负载 | 18 项，TX/C++/Python/Java | 1 / 5，轮换顺序 |
| 启动 / 编译 | 4 语言空程序启动 / 5 类编译操作，含进程树内存采样 | 1 / 7；1 / 3 |
| 优化专项 | 借用、静态运行时、解析、格式化、serde、容器/调用、迭代/算术/拷贝、编码/统计、文件流、长随机数 | 1 / 7；静态运行时 1 / 3 |
| 校准对照 | 18 项综合、堆、补偿统计，GCC 和 clang | 1 / 5，交替顺序 |
| 新增契约 | 真正动态格式化两项、128 节点共享/循环图复制和 GC 两项 | 1 / 5，交替顺序 |

本页覆盖原有套件；新增标准库 29 项四语言负载见 [标准库报告](../stdlib_retest_2026-09-30_static_execution/README.md)。52 个公开模块的代表负载及完整差距汇总见 [总报告](FULL_REPORT.md)。
语言、标准库组合、综合、校准和新增契约的参考重新构建；模块审计与外部题的 C++/Java 固定参考沿用，其源码和二进制哈希已记录。

### 语言特性

四语言逐项核对固定校验值，7 轮原始数据见 [language_features.json](language_features.json)。

| 项目 | TX | C++ | Python | Java | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- | --- | --- |
| scalar_control | 0.168 | 0.234 | 23.268 | 2.503 | 0.168 | +0.0% |
| updates | 1.417 | 1.307 | 61.957 | 5.189 | 1.422 | -0.4% |
| while_logic | 0.177 | 0.183 | 31.038 | 2.388 | 0.177 | +0.0% |
| float_arithmetic | 0.355 | 0.351 | 25.541 | 1.637 | 0.360 | -1.4% |
| overloads | 0.188 | 0.188 | 19.559 | 3.963 | 0.189 | -0.5% |
| named_arguments | 0.404 | 0.375 | 27.699 | 3.266 | 0.425 | -4.9% |
| recursion | 0.035 | 0.067 | 12.473 | 1.067 | 0.041 | -14.6% |
| variadic_unpack | 11.229 | 1.609 | 3.762 | 0.966 | 12.209 | -8.0% |
| array_destructure | 0.010 | 1.091 | 4.490 | 4.958 | 0.118 | -91.5% |
| array_padded | 0.024 | 2.110 | 8.069 | 8.921 | 0.024 | +0.0% |
| dict_iteration | 7.313 | 4.234 | 20.893 | 52.562 | 9.852 | -25.8% |
| struct_operators | 0.025 | 12.736 | 82.696 | 12.912 | 0.539 | -95.4% |
| class_methods | 0.662 | 0.161 | 46.454 | 5.309 | 0.682 | -2.9% |
| virtual_interface | 0.904 | 0.433 | 36.342 | 4.074 | 0.922 | -2.0% |
| class_operator | 0.598 | 0.237 | 16.353 | 2.400 | 0.838 | -28.6% |
| runtime_cast | 0.236 | 1.844 | 12.595 | 2.348 | 0.273 | -13.6% |
| module_call | 0.024 | 3.287 | 27.271 | 6.451 | 0.025 | -4.0% |
| string_conversion | 16.576 | 3.198 | 10.037 | 15.657 | 17.674 | -6.2% |
| deep_copy | 25.082 | 2.343 | 66.079 | 6.677 | 26.444 | -5.2% |
| copy_cycle | 9.302 | 1.793 | 11.631 | 4.355 | 10.497 | -11.4% |
| deinit | 15.378 | 0.707 | 3.934 | 1.971 | 16.452 | -6.5% |
| cycle_gc | 1.542 | 0.073 | 1.740 | 2.227 | 1.544 | -0.1% |

结构体、模块调用和有界递归的静态表示允许 LLVM 继续内联、折叠或循环化，结果不能换算为一般程序的每次调用成本。语言基准中的通用语义差异见 [负载说明](../language_features/README.md)。

### 标准库组合

#### 组合

| 项目 | TX | C++ | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- |
| math | 4.000 | 2.134 | 2.000 | +100.0% |
| random | 1.000 | 0.349 | 1.000 | +0.0% |
| string | 9.000 | 2.607 | 10.000 | -10.0% |
| array | 18.000 | 12.328 | 16.000 | +12.5% |
| dict | 3.000 | — | 4.000 | -25.0% |
| path | 21.000 | 8.473 | 23.000 | -8.7% |
| fs | 43.000 | 41.482 | 41.000 | +4.9% |
| file | 1317.000 | 1313.381 | 1234.000 | +6.7% |
| io | 9.000 | 7.857 | 7.000 | +28.6% |
| time | 4.000 | 3.420 | 4.000 | +0.0% |
| dict_dynamic | — | 17.799 | — | — |
| dict_hash | — | 0.070 | — | — |

### 模块与新增特性

#### compute

| 项目 | TX | C++ | Python | Java | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- | --- | --- |
| algorithm_sort | 5.469 | 3.205 | 6.052 | 7.332 | 7.679 | -28.8% |
| bytes_hex | 8.617 | 1.837 | 3.388 | 3.312 | 11.710 | -26.4% |
| cancel_status | 0.573 | 0.378 | 18.327 | 0.547 | 0.650 | -11.8% |
| cbor_roundtrip | 22.235 | — | — | — | 40.044 | -44.5% |
| crypto_sha256 | 8.852 | 4.517 | 5.715 | 15.224 | 12.645 | -30.0% |
| csv_parse | 24.532 | — | 15.942 | — | 36.991 | -33.7% |
| decimal_add | 10.978 | — | 4.143 | 3.133 | 14.592 | -24.8% |
| dictionary_contains | 0.362 | 0.184 | 7.852 | 1.825 | 0.400 | -9.5% |
| encoding_utf8 | 4.330 | — | 1.864 | 4.282 | 5.491 | -21.1% |
| env_get | 9.844 | 52.055 | 5.569 | 2.155 | 17.033 | -42.2% |
| format_text | 1.231 | 0.823 | 2.034 | 8.868 | 2.687 | -54.2% |
| json_parse | 23.194 | — | 14.075 | 24.451 | 32.457 | -28.5% |
| math_sqrt | 4.153 | 2.099 | 61.468 | 2.484 | 3.134 | +32.5% |
| parse_int | 11.408 | 4.732 | 9.523 | 1.923 | 14.054 | -18.8% |
| random_int | 1.390 | 0.322 | 69.696 | 2.181 | 1.514 | -8.2% |
| regex_search | 12.651 | 10.524 | 11.952 | 8.802 | 63.799 | -80.2% |
| serde_json | 13.898 | — | 35.075 | 39.343 | 14.747 | -5.8% |
| statistics_mean | 4.457 | 0.521 | 281.474 | 2.089 | 8.843 | -49.6% |
| test_assert | 7.654 | 0.000 | 3.922 | 0.197 | 12.305 | -37.8% |
| unicode_nfc | 5.347 | — | 2.139 | 13.679 | 6.828 | -21.7% |
| xml_parse | 32.989 | — | 73.282 | 168.754 | 37.483 | -12.0% |

跨语言校验值差异：`{"random_int": {"TX": "100098265", "C++": "100098265", "Python": "99869743", "Java": "99985624"}}`。

#### features

| 项目 | TX | C++ | Python | Java | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- | --- | --- |
| vector_push | 1.938 | 0.484 | 4.142 | 2.107 | 2.045 | -5.2% |
| vector_index | 0.146 | 0.207 | 24.857 | 2.560 | 0.177 | -17.5% |
| map_lookup | 0.325 | 0.188 | 5.337 | 2.095 | 0.422 | -23.0% |
| set_contains | 0.259 | 0.170 | 8.308 | 1.278 | 0.318 | -18.6% |
| heap_push_pop | 29.891 | 3.029 | 17.187 | 8.579 | 7.925 | +277.2% |
| queue_push_pop | 1.119 | 0.292 | 8.322 | 3.603 | 1.323 | -15.4% |
| iterator_snapshot | 1.468 | 0.046 | 4.121 | 1.269 | 0.165 | +789.7% |
| option_value | 0.025 | 0.013 | 18.088 | 1.377 | 0.026 | -3.8% |
| function_value | 0.286 | 0.163 | 6.164 | 0.381 | 0.145 | +97.2% |
| closure_bind | 0.190 | 0.164 | 8.398 | 0.616 | 0.188 | +1.1% |

#### system

| 项目 | TX | C++ | Python | Java | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- | --- | --- |
| debug_location | 5.391 | — | — | — | 5.816 | -7.3% |
| error_stack | 1.953 | — | — | — | 2.434 | -19.8% |
| file_stream_rw | 843.336 | 828.336 | 863.506 | 924.854 | 865.019 | -2.5% |
| log_event | 7.738 | — | — | — | 8.256 | -6.3% |
| process_spawn | 907.729 | 2100.479 | 939.541 | 7026.159 | 568.936 | +59.5% |
| system_os | 2.363 | — | — | — | 2.382 | -0.8% |

#### legacy_other

| 项目 | Python | Java | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- |
| string | 1.876 | 10.126 | — | — |
| array | 3.216 | 10.401 | — | — |
| dict_hash | 0.699 | 2.544 | — | — |
| path | 50.588 | 35.034 | — | — |
| fs | 21.381 | 72.121 | — | — |
| file | 1339.722 | 1390.993 | — | — |
| io | 0.434 | 16.579 | — | — |
| time | 12.841 | 2.683 | — | — |

### 本机网络

#### http

| 项目 | TX | C++ | Python | Java | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- | --- | --- |
| httpx_get | 975.974 | — | — | — | 997.729 | -2.2% |
| requests_get | 981.354 | — | 1124.466 | — | 1081.947 | -9.3% |
| http_get | — | 935.731 | — | 253.905 | — | — |

#### websocket

| 项目 | TX | C++ | Python | Java | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- | --- | --- |
| websocket_echo | 894.903 | 880.701 | 511.816 | 404.010 | 935.728 | -4.4% |

标准库组合为整数毫秒计时。`test_assert` 的成功断言参考可被优化消除，不列入有效差距排名；统计均值使用后面的补偿统计校准结果。HTTP 客户端命名与实现不同，差距清单将对应请求次数的比例注明为端到端观察。

### 综合负载

完整 64 位容器、解析结果及 payload schema 对照，18 项每轮核对固定校验值。历史 `format_dynamic` 为局部不可变模板，本次会进入静态路径；真正动态模板另见新增契约。

| 项目 | TX | C++ | Python | Java | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- | --- | --- |
| vector_scan_1k | 0.262 | 0.118 | 25.430 | 2.147 | 0.278 | -5.8% |
| vector_scan_100k | 2.479 | 1.189 | 235.182 | 7.553 | 2.460 | +0.8% |
| vector_index_sequential | 0.301 | 0.314 | 70.298 | 2.907 | 0.308 | -2.3% |
| vector_index_strided | 0.298 | 0.304 | 71.099 | 3.215 | 0.361 | -17.5% |
| map_hit_128 | 1.606 | 0.847 | 34.376 | 10.700 | 1.696 | -5.3% |
| map_hit_8192 | 2.215 | 0.916 | 42.140 | 11.762 | 2.179 | +1.7% |
| map_hit_10_percent | 2.522 | 1.573 | 48.110 | 9.883 | 2.712 | -7.0% |
| dictionary_int_hit | 2.364 | 0.754 | 14.477 | 4.361 | 1.194 | +98.0% |
| dictionary_text_hit | 3.157 | 4.400 | 16.786 | 3.355 | 5.264 | -40.0% |
| dictionary_mostly_miss | 0.845 | 0.765 | 15.540 | 5.393 | 0.907 | -6.8% |
| format_literal | 6.574 | 0.782 | 5.172 | 16.084 | 7.371 | -10.8% |
| format_dynamic | 6.923 | 1.041 | 10.459 | 29.876 | 7.537 | -8.1% |
| encoding_literal | 12.491 | 2.866 | 5.637 | 5.993 | 12.816 | -2.5% |
| encoding_dynamic | 11.221 | 2.785 | 5.495 | 5.003 | 11.586 | -3.2% |
| serde_short_text | 9.825 | 0.976 | 17.383 | 18.377 | 6.964 | +41.1% |
| serde_long_text | 8.712 | 4.834 | 19.777 | 21.388 | 9.001 | -3.2% |
| parse_valid | 0.830 | 1.745 | 13.102 | 13.163 | 1.038 | -20.0% |
| parse_invalid | 0.745 | 6.248 | 76.473 | 177.627 | 0.978 | -23.8% |

#### 同轮 GCC / clang 校准

GCC 与 clang 各有独立配对采样。下表 TX 耗时来自 GCC 配对轮；倍率各自使用对应配对轮 TX 中位数。

| 项目 | TX（GCC 轮） | GCC | clang | TX/GCC | TX/clang |
| --- | --- | --- | --- | --- | --- |
| vector_scan_1k | 0.260 | 0.117 | 0.066 | 2.22× | 3.95× |
| vector_scan_100k | 2.416 | 1.173 | 0.838 | 2.06× | 2.90× |
| vector_index_sequential | 0.294 | 0.333 | 0.235 | 0.88× | 1.26× |
| vector_index_strided | 0.293 | 0.300 | 0.240 | 0.98× | 1.25× |
| map_hit_128 | 1.518 | 0.830 | 0.356 | 1.83× | 4.36× |
| map_hit_8192 | 2.008 | 0.844 | 0.576 | 2.38× | 3.50× |
| map_hit_10_percent | 1.898 | 1.508 | 1.255 | 1.26× | 1.62× |
| dictionary_int_hit | 1.328 | 0.746 | 0.881 | 1.78× | 1.46× |
| dictionary_text_hit | 2.239 | 4.366 | 4.383 | 0.51× | 0.52× |
| dictionary_mostly_miss | 0.838 | 0.813 | 0.881 | 1.03× | 0.95× |
| format_literal | 6.306 | 0.782 | 0.647 | 8.06× | 9.81× |
| format_dynamic | 6.219 | 1.044 | 0.919 | 5.96× | 6.78× |
| encoding_literal | 11.582 | 2.806 | 3.227 | 4.13× | 3.63× |
| encoding_dynamic | 11.300 | 2.664 | 2.845 | 4.24× | 3.89× |
| serde_short_text | 6.943 | 0.975 | 1.025 | 7.12× | 7.63× |
| serde_long_text | 8.432 | 4.412 | 5.102 | 1.91× | 1.68× |
| parse_valid | 0.764 | 1.704 | 0.971 | 0.45× | 0.80× |
| parse_invalid | 0.727 | 6.149 | 5.427 | 0.12× | 0.14× |
| heap_push_pop | 29.625 | 3.656 | 4.759 | 8.10× | 7.32× |
| statistics_mean | 4.520 | 2.935 | 4.187 | 1.54× | 1.07× |

### 外部题

本轮重建 TX 普通程序和计时程序。mini-filesystem 正式数据 44/44、not-yet-on-stage 45/45；五语言每次计时执行均核对答案。

#### mini-filesystem

| 输入 | TX | C++ | Python | Java | JavaScript | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| fanout-2000 | 11.182 | 1.671 | 4.042 | 7.545 | 5.481 | 12.302 | -9.1% |
| deep-pwd-2000 | 12.568 | 15.227 | 11.968 | 33.507 | 10.217 | 14.531 | -13.5% |
| moves-2000 | 5.442 | 1.017 | 2.120 | 5.941 | 1.922 | 6.191 | -12.1% |
| random-2000-1 | 6.399 | 0.997 | 2.598 | 6.291 | 2.175 | 7.198 | -11.1% |
| linklong-2000 | 32.177 | 109.947 | 454.160 | 273.926 | 121.377 | 38.952 | -17.4% |

#### not-yet-on-stage

| 输入 | TX | C++ | Python | Java | JavaScript | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| all-free-max | 0.993 | 1.416 | 44.048 | 4.624 | 3.741 | 1.047 | -5.2% |
| forced-increasing-max | 4.299 | 1.418 | 77.089 | 5.628 | 5.242 | 4.836 | -11.1% |
| forced-decreasing-max | 1.010 | 1.438 | 44.891 | 4.992 | 3.936 | 1.038 | -2.7% |
| shuffled-tight-max-1 | 3.280 | 1.925 | 64.534 | 5.629 | 5.049 | 3.094 | +6.0% |
| alternating-tight-max | 2.701 | 1.435 | 60.739 | 5.613 | 4.635 | 2.908 | -7.1% |

### 优化专项

各项目逐轮核对校验值稳定，单位 ms。

| 专项 | 项目 | 中位数 | 最小 | 最大 | 轮数 | 上一轮 TX | 变化 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| borrowing | dictionary | 3.298 | 3.258 | 3.668 | 7 | 3.553 | -7.2% |
| borrowing | map | 2.878 | 2.789 | 3.151 | 7 | 3.292 | -12.6% |
| borrowing | set | 2.550 | 2.478 | 3.041 | 7 | 2.556 | -0.2% |
| borrowing | queue | 1.258 | 1.162 | 1.351 | 7 | 1.507 | -16.5% |
| borrowing | cancel | 5.504 | 5.334 | 6.117 | 7 | 4.759 | +15.7% |
| static_runtime | vector_foreach | 1.263 | 1.197 | 2.821 | 3 | 1.773 | -28.8% |
| static_runtime | serde_json | 13.982 | 13.440 | 16.445 | 3 | 13.422 | +4.2% |
| static_runtime | format_text | 13.734 | 13.641 | 14.043 | 3 | 12.819 | +7.1% |
| static_runtime | statistics_mean | 10.430 | 10.157 | 10.689 | 3 | 18.249 | -42.8% |
| static_runtime | encoding_utf8 | 22.987 | 21.827 | 23.846 | 3 | 23.025 | -0.2% |
| parse_paths | local_valid | 0.766 | 0.762 | 0.783 | 7 | 1.019 | -24.8% |
| parse_paths | local_invalid | 0.750 | 0.735 | 0.795 | 7 | 0.982 | -23.6% |
| parse_paths | full_valid | 0.878 | 0.859 | 1.009 | 7 | 2.113 | -58.4% |
| parse_paths | full_invalid | 3.241 | 3.212 | 3.387 | 7 | 5.330 | -39.2% |
| format_paths | format_literal | 7.315 | 7.175 | 7.521 | 7 | 7.311 | +0.1% |
| format_paths | format_local | 7.316 | 7.095 | 7.899 | 7 | 6.998 | +4.5% |
| format_paths | format_dynamic | 19.273 | 18.940 | 34.271 | 7 | 17.266 | +11.6% |
| serde_paths | serde_json_short | 7.210 | 7.102 | 7.653 | 7 | 6.848 | +5.3% |
| serde_paths | serde_cbor_short | 4.732 | 4.572 | 4.882 | 7 | 4.873 | -2.9% |
| serde_paths | serde_json_long | 8.425 | 8.301 | 8.940 | 7 | 8.226 | +2.4% |
| serde_paths | serde_cbor_long | 8.341 | 8.250 | 9.139 | 7 | 8.993 | -7.3% |
| paths_09_11 | struct_fields | 0.662 | 0.655 | 0.718 | 7 | 0.653 | +1.4% |
| paths_09_11 | class_methods | 1.630 | 1.492 | 1.647 | 7 | 1.511 | +7.9% |
| paths_09_11 | module_call | 0.024 | 0.023 | 0.024 | 7 | 0.026 | -7.7% |
| paths_09_11 | function_value | 2.810 | 2.781 | 3.015 | 7 | 1.416 | +98.4% |
| paths_09_11 | closure_bind | 1.645 | 1.623 | 1.685 | 7 | 2.071 | -20.6% |
| paths_09_11 | heap_push_pop | 29.949 | 29.157 | 30.820 | 7 | 7.916 | +278.3% |
| paths_12_14 | snapshot | 14.787 | 14.559 | 16.609 | 7 | 3.112 | +375.2% |
| paths_12_14 | vector_1k | 0.240 | 0.236 | 0.347 | 7 | 0.239 | +0.4% |
| paths_12_14 | vector_100k | 0.297 | 0.281 | 0.370 | 7 | 0.283 | +4.9% |
| paths_12_14 | division | 0.398 | 0.390 | 0.438 | 7 | 0.398 | +0.0% |
| paths_12_14 | recursion | 0.001 | 0.000 | 0.001 | 7 | 0.001 | +0.0% |
| paths_12_14 | static_spread | 19.563 | 19.197 | 20.632 | 7 | 22.114 | -11.5% |
| paths_12_14 | dynamic_spread | 19.422 | 18.876 | 20.060 | 7 | 22.070 | -12.0% |
| paths_12_14 | deep_copy | 78.753 | 78.004 | 102.069 | 7 | 85.426 | -7.8% |
| paths_15_16 | hex_encode_short | 18.549 | 16.618 | 22.525 | 7 | 18.035 | +2.9% |
| paths_15_16 | hex_decode_short | 31.879 | 28.286 | 45.863 | 7 | 30.902 | +3.2% |
| paths_15_16 | hex_encode_large | 4.566 | 4.239 | 7.440 | 7 | 4.565 | +0.0% |
| paths_15_16 | hex_decode_large | 6.752 | 5.899 | 9.153 | 7 | 8.824 | -23.5% |
| paths_15_16 | utf8_encode_short | 24.570 | 22.218 | 27.000 | 7 | 22.791 | +7.8% |
| paths_15_16 | utf8_decode_short | 14.067 | 13.046 | 20.020 | 7 | 12.924 | +8.8% |
| paths_15_16 | utf8_encode_large | 16.209 | 15.455 | 24.272 | 7 | 2.052 | +689.9% |
| paths_15_16 | utf8_decode_large | 32.357 | 30.633 | 39.740 | 7 | 3.623 | +793.1% |
| paths_15_16 | mean_short | 5.125 | 4.599 | 7.137 | 7 | 8.017 | -36.1% |
| paths_15_16 | mean_large | 25.333 | 22.742 | 26.045 | 7 | 40.586 | -37.6% |
| file_io | file_stream_rw | 888.247 | 871.428 | 938.443 | 7 | 863.366 | +2.9% |

长随机数：TX 41.000 ms；C++ 10.105 ms；固定校验和 2500067985。

### 新增契约对照

动态模板交替两种宽度并包含函数传参，C++ 使用 `std::vformat`。图对照为 128 个节点、共享及循环边，复制使用身份映射，GC 扫描外部引用；100 轮均验证原图和副本的 256 个观察标记全部释放。图 TX 一栏直接链接当前 TX 运行时，包含通用运行时机制成本，不代表 .tx 前端生成代码或类析构/复活的全部语义。

| 项目 | TX / TX 运行时 | C++ | TX/C++ | 上一轮 TX | 变化 | 校验值 |
| --- | --- | --- | --- | --- | --- | --- |
| format_alternating | 17.629 | 3.262 | 5.40× | 16.046 | +9.9% | 520000 |
| format_parameter | 17.913 | 3.239 | 5.53× | 16.380 | +9.4% | 520000 |
| graph_copy | 7.334 | 2.315 | 3.17× | 6.696 | +9.5% | 12800 |
| graph_gc | 10.423 | 8.457 | 1.23× | 10.139 | +2.8% | 25600 |

### 进程、启动、编译和内存

内存每 2 ms 轮询进程树工作集，是近似采样峰值。墙钟时间包括启动和退出，不能与子项内部计时直接相加比较。

| 类型 | 语言 / 操作 | 墙钟 ms | 峰值 MiB | 上一轮墙钟 ms | 变化 |
| --- | --- | --- | --- | --- | --- |
| 进程运行 | TX | 122.547 | 9.062 | 112.139 | +9.3% |
| 进程运行 | C++ | 70.008 | 5.691 | 58.476 | +19.7% |
| 进程运行 | Python | 810.873 | 14.465 | 798.845 | +1.5% |
| 进程运行 | Java | 976.313 | 186.375 | 913.071 | +6.9% |
| 空程序启动 | TX | 46.548 | 6.168 | 27.501 | +69.3% |
| 空程序启动 | C++ | 26.565 | — | 22.421 | +18.5% |
| 空程序启动 | Python | 67.653 | 11.609 | 62.064 | +9.0% |
| 空程序启动 | Java | 212.732 | 52.188 | 198.152 | +7.4% |
| 编译 | TX check | 379.139 | 9.699 | 169.752 | +123.3% |
| 编译 | TX full | 1691.320 | 186.004 | 1210.812 | +39.7% |
| 编译 | C++ full | 4743.954 | 230.703 | 4359.065 | +8.8% |
| 编译 | Python bytecode | 88.758 | 13.586 | 80.546 | +10.2% |
| 编译 | Java full | 1230.949 | 150.996 | 1170.295 | +5.2% |

### 差距及复现

完整的 ≥3× 原始倍率见 [gaps.md](gaps.md) 和 [gaps.json](gaps.json)，含参考语义差异说明；不能把不同套件的同名项目累计为独立热点。所有上一轮 TX 变化见 [comparison.md](comparison.md) 和 [comparison.json](comparison.json)。

- [主采样脚本](run.py)、[汇总脚本](summarize.py)、[输入及工具链指纹](manifest.json)、[完成记录](completed.json)。
- [语言样本](language_features.json)、[语言日志](language_features.log)、[标准库/模块/网络/外部题样本](performance_retest.json)。
- [综合/启动/编译/内存样本](diverse.json)、[校准样本](equivalence.json)、[专项样本](special.json)、[动态格式样本](format_contract.json)、[图对照样本](graph_contract.json)。
- 复现命令：`python -X utf8 -B benchmarks/performance_retest_2026-09-30_static_execution/run.py`。脚本拒绝覆盖已有 manifest，重跑请使用新归档目录。
- 沿用本机被 Git 忽略的模块审计、参考程序和 `E:/Project/problems` 正式数据；归档不是脱离这些本机数据即可独立执行的套件。
- 本轮只新增复测及汇总记录，未修改编译器性能实现，也未运行无关功能测试。

## 29 项标准库四语言完整对比

版本 `f1cc9a5368d01d8a9b413facf2495af8f63e5c57`。预热 1 轮、正式 5 轮，轮换四语言执行顺序，内部计时中位数，单位 ms。
29 项全部完成，逐轮校验值一致。使用当前已修正的日志参考，不混入历史校准样本。
同后端 C++ 对照衡量公开调用路径成本；Java 为短命 JVM，不能外推到充分预热的稳态服务器。

| 负载 | TX | C++ | Java | Python | TX/C++ | TX/Java | TX/Python |
| --- | --- | --- | --- | --- | --- | --- | --- |
| thread_spawn_join | 13.854 | 14.397 | 26.029 | 17.329 | 0.96× | 0.53× | 0.80× |
| mutex_uncontended | 20.517 | 0.481 | 6.021 | 15.241 | 42.65× | 3.41× | 1.35× |
| atomic_add | 0.384 | 0.178 | 1.117 | 15.662 | 2.16× | 0.34× | 0.02× |
| channel_send_recv | 1.978 | 0.787 | 3.932 | 30.664 | 2.51× | 0.50× | 0.06× |
| task_spawn_wait | 7.506 | 6.949 | 21.862 | 17.057 | 1.08× | 0.34× | 0.44× |
| sqlite_insert | 11.422 | 8.403 | 28.877 | 1.915 | 1.36× | 0.40× | 5.97× |
| sqlite_read | 26.133 | 8.819 | 27.484 | 9.418 | 2.96× | 0.95× | 2.77× |
| sqlite_savepoint | 2.622 | 1.862 | 6.717 | 0.653 | 1.41× | 0.39× | 4.01× |
| sqlite_pool | 137.792 | 44.205 | 77.012 | 52.103 | 3.12× | 1.79× | 2.64× |
| sqlite_async | 54.773 | 46.803 | 74.162 | 52.462 | 1.17× | 0.74× | 1.04× |
| migration_recheck | 2.117 | 2.120 | 10.750 | 1.183 | 1.00× | 0.20× | 1.79× |
| postgres_insert | 189.458 | 178.003 | 277.867 | 258.501 | 1.06× | 0.68× | 0.73× |
| postgres_read | 41.141 | 23.134 | 31.155 | 18.738 | 1.78× | 1.32× | 2.20× |
| postgres_savepoint | 115.403 | 111.057 | 116.987 | 114.004 | 1.04× | 0.99× | 1.01× |
| secret_equal | 14.163 | 8.730 | 26.825 | 11.534 | 1.62× | 0.53× | 1.23× |
| argon2_hash_verify | 163.263 | 172.649 | 525.134 | 171.046 | 0.95× | 0.31× | 0.95× |
| ed25519_sign | 17.126 | 17.178 | 411.655 | 16.779 | 1.00× | 0.04× | 1.02× |
| ed25519_verify | 24.329 | 24.418 | 349.294 | 45.542 | 1.00× | 0.07× | 0.53× |
| x509_parse_der | 2.965 | 2.541 | 6.919 | 3.014 | 1.17× | 0.43× | 0.98× |
| dns_localhost | 0.266 | 34.211 | 55.988 | 42.820 | 0.01× | 0.00× | 0.01× |
| udp_echo | 34.354 | 25.189 | 43.975 | 30.689 | 1.36× | 0.78× | 1.12× |
| ipc_echo | 12.771 | 8.530 | 12.044 | 9.483 | 1.50× | 1.06× | 1.35× |
| tls_handshake | 144.254 | 25.348 | 228.463 | 73.864 | 5.69× | 0.63× | 1.95× |
| async_file_rw | 38.012 | 32.663 | 83.577 | 76.585 | 1.16× | 0.45× | 0.50× |
| test_parameterized | 0.223 | 0.056 | 0.826 | 1.513 | 3.97× | 0.27× | 0.15× |
| test_property | 0.334 | 0.046 | 1.107 | 2.317 | 7.18× | 0.30× | 0.14× |
| log_filtered | 0.901 | 0.422 | 4.021 | 6.419 | 2.14× | 0.22× | 0.14× |
| log_file | 31.396 | 28.779 | 52.741 | 79.633 | 1.09× | 0.60× | 0.39× |
| profile_spans | 0.471 | 0.227 | 1.058 | 0.337 | 2.07× | 0.45× | 1.40× |

### ≥3× 项目

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 可比边界 |
| --- | --- | --- | --- | --- | --- | --- |
| 新增/concurrency | mutex_uncontended | C++ | 20.517000 | 0.481100 | 42.65× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 新增/diagnostics | test_property | C++ | 0.334000 | 0.046500 | 7.18× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 新增/sqlite | sqlite_insert | Python | 11.422000 | 1.914800 | 5.97× | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| 新增/network | tls_handshake | C++ | 144.254000 | 25.348400 | 5.69× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 新增/sqlite | sqlite_savepoint | Python | 2.622000 | 0.653100 | 4.01× | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| 新增/diagnostics | test_parameterized | C++ | 0.223000 | 0.056200 | 3.97× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 新增/concurrency | mutex_uncontended | Java | 20.517000 | 6.021400 | 3.41× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 新增/sqlite | sqlite_pool | C++ | 137.792000 | 44.204900 | 3.12× | TX 与 C++ 测真实池；其 SQLite 归还策略为关闭重开。Java/Python 参考每次连接关闭，保持生命周期，未复制完整池等待/别名检查。 |

### 工作量及边界

| 负载 | 模块 | 工作量 | 边界 |
| --- | --- | --- | --- |
| thread_spawn_join | thread | 100 次启动线程并 join | C++ std::thread；Java 平台线程；Python threading。含线程创建/退出。 |
| mutex_uncontended | sync | 100000 次无竞争加锁、取值、加一、写回、解锁 | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| atomic_add | sync | 100000 次顺序一致 int64 加一 | C++ std::atomic、Java AtomicLong；Python 没有对应公开原子整数，使用 Lock 实现线程安全加法，不能当成原子指令性能。 |
| channel_send_recv | channel | 容量 1，20000 次同线程 send/recv | C++ mutex/condition_variable 队列；Java ArrayBlockingQueue；Python Queue。都是立即成功路径；TX 另有取消及句柄规则，不代表多生产者吞吐。 |
| task_spawn_wait | task | 500 次提交并等待返回 1 | 参考使用单工作线程池；TX 使用任务运行时和 scope。包含组内初次线程池启动，不代表饱和并发吞吐。 |
| sqlite_insert | db | 单事务参数绑定插入 2000 行 | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| sqlite_read | db | 2000 行×10 遍，读取整数及文本并求和 | C++ 使用同一后端及行快照；Java JDBC、Python sqlite3。TX/C++ 差值包含 ABI、option、行读取接口与生成代码开销。 |
| sqlite_savepoint | db | 100 次事务/保存点/写入/回滚/提交 | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| sqlite_pool | db | 100 次获取、SELECT 42、归还 | TX 与 C++ 测真实池；其 SQLite 归还策略为关闭重开。Java/Python 参考每次连接关闭，保持生命周期，未复制完整池等待/别名检查。 |
| sqlite_async | db,task | 100 次异步 SELECT 42 并等待 | TX 任务运行时；C++ 同异步数据库后端加单线程执行器；Java/Python 单线程池。都包含单独事务与连接关闭。 |
| migration_recheck | db | 100 次重查已应用迁移及 schema_version | C++ 同迁移后端；Java/Python 实现相同长度分帧 SHA-256、两次 IMMEDIATE 事务及账本读取；只覆盖幂等成功路径。 |
| postgres_insert | db | TLS 单事务参数绑定插入 2000 行 | C++ 同 libpq 后端；Java PostgreSQL JDBC；Python psycopg。驱动 prepare 缓存策略不同；共用临时 PostgreSQL 18.4，不连接已有数据库。 |
| postgres_read | db | TLS 2000 行×10 遍，读取整数和文本 | TX/C++ libpq 单行模式；Java/Python 默认结果获取策略不同。C++ 同后端可用于定位包装层开销，跨驱动比例是端到端负载观察。 |
| postgres_savepoint | db | TLS 100 次保存点回滚事务 | C++ 同后端；Java/Python 各自驱动。所有操作在真实服务器执行，受本机网络调度影响。 |
| secret_equal | secret | 20000 次 1024 字节常量时间比较 | C++ 同 secret 后端；Java MessageDigest.isEqual；Python hmac.compare_digest。Java/Python 普通字节数组没有 TX 秘密句柄的保护与清零生命周期。 |
| argon2_hash_verify | password | 3 次 Argon2id 哈希并验证 | m=19456 KiB、t=2、p=1、v=19、16 字节随机 salt、32 字节输出；C++ 同 Argon2 后端，Java BouncyCastle，Python argon2-cffi；含 PHC 编解码。 |
| ed25519_sign | public_key | 1024 字节消息签名 500 次 | 共用临时随机 seed；C++ 同 libsodium 后端；Java JCA Ed25519；Python cryptography。密钥导入不计时，签名随后验签。 |
| ed25519_verify | public_key | 1024 字节消息验签 500 次 | 三类后端验证各自生成的签名；同一 seed 和消息，成功次数逐轮核对。 |
| x509_parse_der | x509 | 同一 DER 证书解析/编码 500 次 | C++ 同 Windows 证书后端；Java CertificateFactory 可能缓存；Python cryptography。测重复证书热路径，不外推到大量不同证书。 |
| dns_localhost | dns | 100 次 localhost 地址解析 | TX 对 localhost 有专用路径；C++ getaddrinfo、Java InetAddress、Python getaddrinfo 各有不同缓存策略。不能用于比较远端 DNS。 |
| udp_echo | socket | 500 次 1024 字节本机 UDP 回声 | 四语言使用同一 Python 回声服务，逐次完整比较 payload。服务端调度也计入端到端时间。 |
| ipc_echo | ipc | 500 次 TXIP 命名管道回声 | 使用同一服务器、22 字节帧和 CBOR 整数 42。C++/Java/Python 固定整数编解码；TX 为通用 CBOR 与资源句柄，不代表任意对象 IPC 差距。 |
| tls_handshake | tls | 10 次新 TCP+TLS 1.2 握手及单字节回声 | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| async_file_rw | async_file | 32 次 64 KiB 定位写读，合计传输 4 MiB | TX IOCP、Java AsynchronousFileChannel；C++/Python 工作线程执行定位 I/O。全部逐次等待并核对字节，不代表大量在途 I/O 吞吐。 |
| test_parameterized | test | 20000 次带索引的成功回调 | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| test_property | test | 20000 次生成器与成功谓词 | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| log_filtered | log | 100000 次被过滤的惰性 debug 日志 | C++ 调用真实 TX 日志后端级别检查，Java Logger，Python logging；各参考仅在级别允许时构造字段，TX 另有闭包/错误边界。废弃初版 C++ volatile 检查倍率。 |
| log_file | log | 2000 条带上下文和敏感字段遮蔽的 JSONL | C++ 同日志后端；Java/Python 固定 schema 构造。全部每条 flush、进程内加锁；计时外核对行数、索引、请求标识、遮蔽值。未覆盖轮转压力。 |
| profile_spans | profile | 1000 次分段计时并保留记录 | TX/C++ 启用真实分析器；Java/Python 仅保存时间段记录，没有同时启用 CPU/分配采样。仅为 API 负载参考，不能宣称完整 profiler 等价。 |

标准库原始数据见 [results.json](../stdlib_retest_2026-09-30_static_execution/results.json)、[manifest.json](../stdlib_retest_2026-09-30_static_execution/manifest.json)、[completed.json](../stdlib_retest_2026-09-30_static_execution/completed.json)。
采样期间源码、工具链和参考程序指纹未变化。服务为本机临时实例，秘密夹具不进入归档。

## 与上一轮的全部 TX 耗时变化

| 套件 | 项目 | 上一轮 TX ms | 本轮 TX ms | 变化 |
| --- | --- | --- | --- | --- |
| 语言特性 | scalar_control | 0.168000 | 0.168000 | +0.0% |
| 语言特性 | updates | 1.422000 | 1.417000 | -0.4% |
| 语言特性 | while_logic | 0.177000 | 0.177000 | +0.0% |
| 语言特性 | float_arithmetic | 0.360000 | 0.355000 | -1.4% |
| 语言特性 | overloads | 0.189000 | 0.188000 | -0.5% |
| 语言特性 | named_arguments | 0.425000 | 0.404000 | -4.9% |
| 语言特性 | recursion | 0.041000 | 0.035000 | -14.6% |
| 语言特性 | variadic_unpack | 12.209000 | 11.229000 | -8.0% |
| 语言特性 | array_destructure | 0.118000 | 0.010000 | -91.5% |
| 语言特性 | array_padded | 0.024000 | 0.024000 | +0.0% |
| 语言特性 | dict_iteration | 9.852000 | 7.313000 | -25.8% |
| 语言特性 | struct_operators | 0.539000 | 0.025000 | -95.4% |
| 语言特性 | class_methods | 0.682000 | 0.662000 | -2.9% |
| 语言特性 | virtual_interface | 0.922000 | 0.904000 | -2.0% |
| 语言特性 | class_operator | 0.838000 | 0.598000 | -28.6% |
| 语言特性 | runtime_cast | 0.273000 | 0.236000 | -13.6% |
| 语言特性 | module_call | 0.025000 | 0.024000 | -4.0% |
| 语言特性 | string_conversion | 17.674000 | 16.576000 | -6.2% |
| 语言特性 | deep_copy | 26.444000 | 25.082000 | -5.2% |
| 语言特性 | copy_cycle | 10.497000 | 9.302000 | -11.4% |
| 语言特性 | deinit | 16.452000 | 15.378000 | -6.5% |
| 语言特性 | cycle_gc | 1.544000 | 1.542000 | -0.1% |
| library/组合 | math | 2.000000 | 4.000000 | +100.0% |
| library/组合 | random | 1.000000 | 1.000000 | +0.0% |
| library/组合 | string | 10.000000 | 9.000000 | -10.0% |
| library/组合 | array | 16.000000 | 18.000000 | +12.5% |
| library/组合 | dict | 4.000000 | 3.000000 | -25.0% |
| library/组合 | path | 23.000000 | 21.000000 | -8.7% |
| library/组合 | fs | 41.000000 | 43.000000 | +4.9% |
| library/组合 | file | 1234.000000 | 1317.000000 | +6.7% |
| library/组合 | io | 7.000000 | 9.000000 | +28.6% |
| library/组合 | time | 4.000000 | 4.000000 | +0.0% |
| audit/compute | algorithm_sort | 7.679000 | 5.469000 | -28.8% |
| audit/compute | bytes_hex | 11.710000 | 8.617000 | -26.4% |
| audit/compute | cancel_status | 0.650000 | 0.573000 | -11.8% |
| audit/compute | cbor_roundtrip | 40.044000 | 22.235000 | -44.5% |
| audit/compute | crypto_sha256 | 12.645000 | 8.852000 | -30.0% |
| audit/compute | csv_parse | 36.991000 | 24.532000 | -33.7% |
| audit/compute | decimal_add | 14.592000 | 10.978000 | -24.8% |
| audit/compute | dictionary_contains | 0.400000 | 0.362000 | -9.5% |
| audit/compute | encoding_utf8 | 5.491000 | 4.330000 | -21.1% |
| audit/compute | env_get | 17.033000 | 9.844000 | -42.2% |
| audit/compute | format_text | 2.687000 | 1.231000 | -54.2% |
| audit/compute | json_parse | 32.457000 | 23.194000 | -28.5% |
| audit/compute | math_sqrt | 3.134000 | 4.153000 | +32.5% |
| audit/compute | parse_int | 14.054000 | 11.408000 | -18.8% |
| audit/compute | random_int | 1.514000 | 1.390000 | -8.2% |
| audit/compute | regex_search | 63.799000 | 12.651000 | -80.2% |
| audit/compute | serde_json | 14.747000 | 13.898000 | -5.8% |
| audit/compute | statistics_mean | 8.843000 | 4.457000 | -49.6% |
| audit/compute | test_assert | 12.305000 | 7.654000 | -37.8% |
| audit/compute | unicode_nfc | 6.828000 | 5.347000 | -21.7% |
| audit/compute | xml_parse | 37.483000 | 32.989000 | -12.0% |
| audit/features | vector_push | 2.045000 | 1.938000 | -5.2% |
| audit/features | vector_index | 0.177000 | 0.146000 | -17.5% |
| audit/features | map_lookup | 0.422000 | 0.325000 | -23.0% |
| audit/features | set_contains | 0.318000 | 0.259000 | -18.6% |
| audit/features | heap_push_pop | 7.925000 | 29.891000 | +277.2% |
| audit/features | queue_push_pop | 1.323000 | 1.119000 | -15.4% |
| audit/features | iterator_snapshot | 0.165000 | 1.468000 | +789.7% |
| audit/features | option_value | 0.026000 | 0.025000 | -3.8% |
| audit/features | function_value | 0.145000 | 0.286000 | +97.2% |
| audit/features | closure_bind | 0.188000 | 0.190000 | +1.1% |
| audit/system | debug_location | 5.816000 | 5.391000 | -7.3% |
| audit/system | error_stack | 2.434000 | 1.953000 | -19.8% |
| audit/system | file_stream_rw | 865.019000 | 843.336000 | -2.5% |
| audit/system | log_event | 8.256000 | 7.738000 | -6.3% |
| audit/system | process_spawn | 568.936000 | 907.729000 | +59.5% |
| audit/system | system_os | 2.382000 | 2.363000 | -0.8% |
| network/http | httpx_get | 997.729000 | 975.974000 | -2.2% |
| network/http | requests_get | 1081.947000 | 981.354000 | -9.3% |
| network/websocket | websocket_echo | 935.728000 | 894.903000 | -4.4% |
| 综合 | vector_scan_1k | 0.278000 | 0.262000 | -5.8% |
| 综合 | vector_scan_100k | 2.460000 | 2.479000 | +0.8% |
| 综合 | vector_index_sequential | 0.308000 | 0.301000 | -2.3% |
| 综合 | vector_index_strided | 0.361000 | 0.298000 | -17.5% |
| 综合 | map_hit_128 | 1.696000 | 1.606000 | -5.3% |
| 综合 | map_hit_8192 | 2.179000 | 2.215000 | +1.7% |
| 综合 | map_hit_10_percent | 2.712000 | 2.522000 | -7.0% |
| 综合 | dictionary_int_hit | 1.194000 | 2.364000 | +98.0% |
| 综合 | dictionary_text_hit | 5.264000 | 3.157000 | -40.0% |
| 综合 | dictionary_mostly_miss | 0.907000 | 0.845000 | -6.8% |
| 综合 | format_literal | 7.371000 | 6.574000 | -10.8% |
| 综合 | format_dynamic | 7.537000 | 6.923000 | -8.1% |
| 综合 | encoding_literal | 12.816000 | 12.491000 | -2.5% |
| 综合 | encoding_dynamic | 11.586000 | 11.221000 | -3.2% |
| 综合 | serde_short_text | 6.964000 | 9.825000 | +41.1% |
| 综合 | serde_long_text | 9.001000 | 8.712000 | -3.2% |
| 综合 | parse_valid | 1.038000 | 0.830000 | -20.0% |
| 综合 | parse_invalid | 0.978000 | 0.745000 | -23.8% |
| 校准/diverse | vector_scan_1k | 0.260000 | 0.260000 | +0.0% |
| 校准/diverse_clang | vector_scan_1k | 0.258000 | 0.261000 | +1.2% |
| 校准/diverse | vector_scan_100k | 2.441000 | 2.416000 | -1.0% |
| 校准/diverse_clang | vector_scan_100k | 2.402000 | 2.434000 | +1.3% |
| 校准/diverse | vector_index_sequential | 0.299000 | 0.294000 | -1.7% |
| 校准/diverse_clang | vector_index_sequential | 0.293000 | 0.295000 | +0.7% |
| 校准/diverse | vector_index_strided | 0.363000 | 0.293000 | -19.3% |
| 校准/diverse_clang | vector_index_strided | 0.350000 | 0.299000 | -14.6% |
| 校准/diverse | map_hit_128 | 1.671000 | 1.518000 | -9.2% |
| 校准/diverse_clang | map_hit_128 | 1.640000 | 1.552000 | -5.4% |
| 校准/diverse | map_hit_8192 | 2.093000 | 2.008000 | -4.1% |
| 校准/diverse_clang | map_hit_8192 | 2.011000 | 2.015000 | +0.2% |
| 校准/diverse | map_hit_10_percent | 2.271000 | 1.898000 | -16.4% |
| 校准/diverse_clang | map_hit_10_percent | 2.212000 | 2.039000 | -7.8% |
| 校准/diverse | dictionary_int_hit | 1.222000 | 1.328000 | +8.7% |
| 校准/diverse_clang | dictionary_int_hit | 1.169000 | 1.289000 | +10.3% |
| 校准/diverse | dictionary_text_hit | 5.170000 | 2.239000 | -56.7% |
| 校准/diverse_clang | dictionary_text_hit | 5.186000 | 2.281000 | -56.0% |
| 校准/diverse | dictionary_mostly_miss | 0.900000 | 0.838000 | -6.9% |
| 校准/diverse_clang | dictionary_mostly_miss | 0.887000 | 0.833000 | -6.1% |
| 校准/diverse | format_literal | 7.327000 | 6.306000 | -13.9% |
| 校准/diverse_clang | format_literal | 7.018000 | 6.346000 | -9.6% |
| 校准/diverse | format_dynamic | 7.514000 | 6.219000 | -17.2% |
| 校准/diverse_clang | format_dynamic | 7.458000 | 6.229000 | -16.5% |
| 校准/diverse | encoding_literal | 12.220000 | 11.582000 | -5.2% |
| 校准/diverse_clang | encoding_literal | 11.625000 | 11.720000 | +0.8% |
| 校准/diverse | encoding_dynamic | 11.627000 | 11.300000 | -2.8% |
| 校准/diverse_clang | encoding_dynamic | 11.880000 | 11.079000 | -6.7% |
| 校准/diverse | serde_short_text | 7.057000 | 6.943000 | -1.6% |
| 校准/diverse_clang | serde_short_text | 6.862000 | 7.818000 | +13.9% |
| 校准/diverse | serde_long_text | 8.923000 | 8.432000 | -5.5% |
| 校准/diverse_clang | serde_long_text | 8.853000 | 8.588000 | -3.0% |
| 校准/diverse | parse_valid | 1.012000 | 0.764000 | -24.5% |
| 校准/diverse_clang | parse_valid | 0.987000 | 0.781000 | -20.9% |
| 校准/diverse | parse_invalid | 0.978000 | 0.727000 | -25.7% |
| 校准/diverse_clang | parse_invalid | 0.969000 | 0.747000 | -22.9% |
| 校准/heap | heap_push_pop | 7.673000 | 29.625000 | +286.1% |
| 校准/heap_clang | heap_push_pop | 7.691000 | 34.858000 | +353.2% |
| 校准/statistics | statistics_mean | 7.960000 | 4.520000 | -43.2% |
| 校准/statistics_clang | statistics_mean | 7.983000 | 4.485000 | -43.8% |
| 外部题/mini-filesystem | fanout-2000 | 12.302000 | 11.182000 | -9.1% |
| 外部题/mini-filesystem | deep-pwd-2000 | 14.531000 | 12.568000 | -13.5% |
| 外部题/mini-filesystem | moves-2000 | 6.191000 | 5.442000 | -12.1% |
| 外部题/mini-filesystem | random-2000-1 | 7.198000 | 6.399000 | -11.1% |
| 外部题/mini-filesystem | linklong-2000 | 38.952000 | 32.177000 | -17.4% |
| 外部题/not-yet-on-stage | all-free-max | 1.047000 | 0.993000 | -5.2% |
| 外部题/not-yet-on-stage | forced-increasing-max | 4.836000 | 4.299000 | -11.1% |
| 外部题/not-yet-on-stage | forced-decreasing-max | 1.038000 | 1.010000 | -2.7% |
| 外部题/not-yet-on-stage | shuffled-tight-max-1 | 3.094000 | 3.280000 | +6.0% |
| 外部题/not-yet-on-stage | alternating-tight-max | 2.908000 | 2.701000 | -7.1% |
| 专项/borrowing | dictionary | 3.553000 | 3.298000 | -7.2% |
| 专项/borrowing | map | 3.292000 | 2.878000 | -12.6% |
| 专项/borrowing | set | 2.556000 | 2.550000 | -0.2% |
| 专项/borrowing | queue | 1.507000 | 1.258000 | -16.5% |
| 专项/borrowing | cancel | 4.759000 | 5.504000 | +15.7% |
| 专项/static_runtime | vector_foreach | 1.773000 | 1.263000 | -28.8% |
| 专项/static_runtime | serde_json | 13.422000 | 13.982000 | +4.2% |
| 专项/static_runtime | format_text | 12.819000 | 13.734000 | +7.1% |
| 专项/static_runtime | statistics_mean | 18.249000 | 10.430000 | -42.8% |
| 专项/static_runtime | encoding_utf8 | 23.025000 | 22.987000 | -0.2% |
| 专项/parse_paths | local_valid | 1.019000 | 0.766000 | -24.8% |
| 专项/parse_paths | local_invalid | 0.982000 | 0.750000 | -23.6% |
| 专项/parse_paths | full_valid | 2.113000 | 0.878000 | -58.4% |
| 专项/parse_paths | full_invalid | 5.330000 | 3.241000 | -39.2% |
| 专项/format_paths | format_literal | 7.311000 | 7.315000 | +0.1% |
| 专项/format_paths | format_local | 6.998000 | 7.316000 | +4.5% |
| 专项/format_paths | format_dynamic | 17.266000 | 19.273000 | +11.6% |
| 专项/serde_paths | serde_json_short | 6.848000 | 7.210000 | +5.3% |
| 专项/serde_paths | serde_cbor_short | 4.873000 | 4.732000 | -2.9% |
| 专项/serde_paths | serde_json_long | 8.226000 | 8.425000 | +2.4% |
| 专项/serde_paths | serde_cbor_long | 8.993000 | 8.341000 | -7.3% |
| 专项/paths_09_11 | struct_fields | 0.653000 | 0.662000 | +1.4% |
| 专项/paths_09_11 | class_methods | 1.511000 | 1.630000 | +7.9% |
| 专项/paths_09_11 | module_call | 0.026000 | 0.024000 | -7.7% |
| 专项/paths_09_11 | function_value | 1.416000 | 2.810000 | +98.4% |
| 专项/paths_09_11 | closure_bind | 2.071000 | 1.645000 | -20.6% |
| 专项/paths_09_11 | heap_push_pop | 7.916000 | 29.949000 | +278.3% |
| 专项/paths_12_14 | snapshot | 3.112000 | 14.787000 | +375.2% |
| 专项/paths_12_14 | vector_1k | 0.239000 | 0.240000 | +0.4% |
| 专项/paths_12_14 | vector_100k | 0.283000 | 0.297000 | +4.9% |
| 专项/paths_12_14 | division | 0.398000 | 0.398000 | +0.0% |
| 专项/paths_12_14 | recursion | 0.001000 | 0.001000 | +0.0% |
| 专项/paths_12_14 | static_spread | 22.114000 | 19.563000 | -11.5% |
| 专项/paths_12_14 | dynamic_spread | 22.070000 | 19.422000 | -12.0% |
| 专项/paths_12_14 | deep_copy | 85.426000 | 78.753000 | -7.8% |
| 专项/paths_15_16 | hex_encode_short | 18.035000 | 18.549000 | +2.9% |
| 专项/paths_15_16 | hex_decode_short | 30.902000 | 31.879000 | +3.2% |
| 专项/paths_15_16 | hex_encode_large | 4.565000 | 4.566000 | +0.0% |
| 专项/paths_15_16 | hex_decode_large | 8.824000 | 6.752000 | -23.5% |
| 专项/paths_15_16 | utf8_encode_short | 22.791000 | 24.570000 | +7.8% |
| 专项/paths_15_16 | utf8_decode_short | 12.924000 | 14.067000 | +8.8% |
| 专项/paths_15_16 | utf8_encode_large | 2.052000 | 16.209000 | +689.9% |
| 专项/paths_15_16 | utf8_decode_large | 3.623000 | 32.357000 | +793.1% |
| 专项/paths_15_16 | mean_short | 8.017000 | 5.125000 | -36.1% |
| 专项/paths_15_16 | mean_large | 40.586000 | 25.333000 | -37.6% |
| 专项/file_io | file_stream_rw | 863.366000 | 888.247000 | +2.9% |
| 专项 | random_long | 34.000000 | 41.000000 | +20.6% |
| 契约/format_contract | format_alternating | 16.046000 | 17.629000 | +9.9% |
| 契约/format_contract | format_parameter | 16.380000 | 17.913000 | +9.4% |
| 契约/graph_contract | graph_copy | 6.696000 | 7.334000 | +9.5% |
| 契约/graph_contract | graph_gc | 10.139000 | 10.423000 | +2.8% |
| 进程运行 | TX | 112.139300 | 122.547100 | +9.3% |
| 空程序启动 | TX | 27.500800 | 46.547500 | +69.3% |
| 编译 | TX check | 169.751800 | 379.139000 | +123.3% |
| 编译 | TX full | 1210.812200 | 1691.320200 | +39.7% |
| 新增/concurrency | thread_spawn_join | 13.894000 | 13.854000 | -0.3% |
| 新增/concurrency | mutex_uncontended | 20.630000 | 20.517000 | -0.5% |
| 新增/concurrency | atomic_add | 0.287000 | 0.384000 | +33.8% |
| 新增/concurrency | channel_send_recv | 2.036000 | 1.978000 | -2.8% |
| 新增/concurrency | task_spawn_wait | 7.247000 | 7.506000 | +3.6% |
| 新增/sqlite | sqlite_insert | 11.744000 | 11.422000 | -2.7% |
| 新增/sqlite | sqlite_read | 26.512000 | 26.133000 | -1.4% |
| 新增/sqlite | sqlite_savepoint | 2.710000 | 2.622000 | -3.2% |
| 新增/sqlite | sqlite_pool | 139.236000 | 137.792000 | -1.0% |
| 新增/sqlite | sqlite_async | 54.127000 | 54.773000 | +1.2% |
| 新增/sqlite | migration_recheck | 2.033000 | 2.117000 | +4.1% |
| 新增/postgres | postgres_insert | 202.260000 | 189.458000 | -6.3% |
| 新增/postgres | postgres_read | 45.568000 | 41.141000 | -9.7% |
| 新增/postgres | postgres_savepoint | 121.967000 | 115.403000 | -5.4% |
| 新增/security | secret_equal | 16.526000 | 14.163000 | -14.3% |
| 新增/security | argon2_hash_verify | 159.154000 | 163.263000 | +2.6% |
| 新增/security | ed25519_sign | 17.087000 | 17.126000 | +0.2% |
| 新增/security | ed25519_verify | 24.950000 | 24.329000 | -2.5% |
| 新增/security | x509_parse_der | 2.739000 | 2.965000 | +8.3% |
| 新增/network | dns_localhost | 0.305000 | 0.266000 | -12.8% |
| 新增/network | udp_echo | 35.372000 | 34.354000 | -2.9% |
| 新增/network | ipc_echo | 12.739000 | 12.771000 | +0.3% |
| 新增/network | tls_handshake | 114.903000 | 144.254000 | +25.5% |
| 新增/async_file | async_file_rw | 39.435000 | 38.012000 | -3.6% |
| 新增/diagnostics | test_parameterized | 0.377000 | 0.223000 | -40.8% |
| 新增/diagnostics | test_property | 0.664000 | 0.334000 | -49.7% |
| 新增/diagnostics | log_filtered | 1.024000 | 0.901000 | -12.0% |
| 新增/diagnostics | log_file | 32.115000 | 31.396000 | -2.2% |
| 新增/profile | profile_spans | 0.441000 | 0.471000 | +6.8% |

## 全部 ≥3× 跨语言耗时记录

下表保留每个套件及参考语言；前面的按名称去重表只用于扫描热点。

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

## 全部有效运行时对照倍率

包含低于 3× 的对照。零耗时参考不计算倍率；成功断言可被优化消除、旧简单求和参考和没有参考实现的专项不进入有效倍率表。全部原始耗时仍保留在上方逐项表和 JSON 中。

| 套件 | 项目 | 参考 | TX ms | 参考 ms | TX/参考 | 边界 |
| --- | --- | --- | --- | --- | --- | --- |
| 语言特性 | scalar_control | C++ | 0.168000 | 0.234500 | 0.72× |  |
| 语言特性 | scalar_control | Python | 0.168000 | 23.268000 | 0.01× |  |
| 语言特性 | scalar_control | Java | 0.168000 | 2.503000 | 0.07× |  |
| 语言特性 | updates | C++ | 1.417000 | 1.306600 | 1.08× |  |
| 语言特性 | updates | Python | 1.417000 | 61.957100 | 0.02× |  |
| 语言特性 | updates | Java | 1.417000 | 5.188600 | 0.27× |  |
| 语言特性 | while_logic | C++ | 0.177000 | 0.183500 | 0.96× |  |
| 语言特性 | while_logic | Python | 0.177000 | 31.037700 | 0.01× |  |
| 语言特性 | while_logic | Java | 0.177000 | 2.388200 | 0.07× |  |
| 语言特性 | float_arithmetic | C++ | 0.355000 | 0.351200 | 1.01× |  |
| 语言特性 | float_arithmetic | Python | 0.355000 | 25.541500 | 0.01× |  |
| 语言特性 | float_arithmetic | Java | 0.355000 | 1.637500 | 0.22× |  |
| 语言特性 | overloads | C++ | 0.188000 | 0.187500 | 1.00× |  |
| 语言特性 | overloads | Python | 0.188000 | 19.558900 | 0.01× |  |
| 语言特性 | overloads | Java | 0.188000 | 3.963300 | 0.05× |  |
| 语言特性 | named_arguments | C++ | 0.404000 | 0.375000 | 1.08× |  |
| 语言特性 | named_arguments | Python | 0.404000 | 27.699400 | 0.01× |  |
| 语言特性 | named_arguments | Java | 0.404000 | 3.265800 | 0.12× |  |
| 语言特性 | recursion | C++ | 0.035000 | 0.067300 | 0.52× |  |
| 语言特性 | recursion | Python | 0.035000 | 12.472700 | 0.00× |  |
| 语言特性 | recursion | Java | 0.035000 | 1.067200 | 0.03× |  |
| 语言特性 | variadic_unpack | C++ | 11.229000 | 1.608500 | 6.98× | 参考不执行 TX 动态命名实参绑定 |
| 语言特性 | variadic_unpack | Python | 11.229000 | 3.761800 | 2.99× |  |
| 语言特性 | variadic_unpack | Java | 11.229000 | 0.965900 | 11.63× | 参考不执行 TX 动态命名实参绑定 |
| 语言特性 | array_destructure | C++ | 0.010000 | 1.090900 | 0.01× |  |
| 语言特性 | array_destructure | Python | 0.010000 | 4.490400 | 0.00× |  |
| 语言特性 | array_destructure | Java | 0.010000 | 4.957800 | 0.00× |  |
| 语言特性 | array_padded | C++ | 0.024000 | 2.110200 | 0.01× |  |
| 语言特性 | array_padded | Python | 0.024000 | 8.069500 | 0.00× |  |
| 语言特性 | array_padded | Java | 0.024000 | 8.921000 | 0.00× |  |
| 语言特性 | dict_iteration | C++ | 7.313000 | 4.234400 | 1.73× |  |
| 语言特性 | dict_iteration | Python | 7.313000 | 20.893400 | 0.35× |  |
| 语言特性 | dict_iteration | Java | 7.313000 | 52.561800 | 0.14× |  |
| 语言特性 | struct_operators | C++ | 0.025000 | 12.736100 | 0.00× |  |
| 语言特性 | struct_operators | Python | 0.025000 | 82.695800 | 0.00× |  |
| 语言特性 | struct_operators | Java | 0.025000 | 12.911700 | 0.00× |  |
| 语言特性 | class_methods | C++ | 0.662000 | 0.161100 | 4.11× |  |
| 语言特性 | class_methods | Python | 0.662000 | 46.454000 | 0.01× |  |
| 语言特性 | class_methods | Java | 0.662000 | 5.309200 | 0.12× |  |
| 语言特性 | virtual_interface | C++ | 0.904000 | 0.433100 | 2.09× |  |
| 语言特性 | virtual_interface | Python | 0.904000 | 36.341500 | 0.02× |  |
| 语言特性 | virtual_interface | Java | 0.904000 | 4.073700 | 0.22× |  |
| 语言特性 | class_operator | C++ | 0.598000 | 0.236700 | 2.53× |  |
| 语言特性 | class_operator | Python | 0.598000 | 16.353300 | 0.04× |  |
| 语言特性 | class_operator | Java | 0.598000 | 2.400000 | 0.25× |  |
| 语言特性 | runtime_cast | C++ | 0.236000 | 1.844300 | 0.13× |  |
| 语言特性 | runtime_cast | Python | 0.236000 | 12.595500 | 0.02× |  |
| 语言特性 | runtime_cast | Java | 0.236000 | 2.347500 | 0.10× |  |
| 语言特性 | module_call | C++ | 0.024000 | 3.286900 | 0.01× |  |
| 语言特性 | module_call | Python | 0.024000 | 27.270900 | 0.00× |  |
| 语言特性 | module_call | Java | 0.024000 | 6.451000 | 0.00× |  |
| 语言特性 | string_conversion | C++ | 16.576000 | 3.197800 | 5.18× |  |
| 语言特性 | string_conversion | Python | 16.576000 | 10.036500 | 1.65× |  |
| 语言特性 | string_conversion | Java | 16.576000 | 15.657400 | 1.06× |  |
| 语言特性 | deep_copy | C++ | 25.082000 | 2.342900 | 10.71× | 参考只复制已知形状；通用图复制另见新增契约 |
| 语言特性 | deep_copy | Python | 25.082000 | 66.078800 | 0.38× |  |
| 语言特性 | deep_copy | Java | 25.082000 | 6.677200 | 3.76× | 参考只复制已知形状；通用图复制另见新增契约 |
| 语言特性 | copy_cycle | C++ | 9.302000 | 1.793300 | 5.19× |  |
| 语言特性 | copy_cycle | Python | 9.302000 | 11.631000 | 0.80× |  |
| 语言特性 | copy_cycle | Java | 9.302000 | 4.354800 | 2.14× |  |
| 语言特性 | deinit | C++ | 15.378000 | 0.707100 | 21.75× |  |
| 语言特性 | deinit | Python | 15.378000 | 3.934000 | 3.91× |  |
| 语言特性 | deinit | Java | 15.378000 | 1.970700 | 7.80× | Java 显式 close 回调，不是 JVM 确定性析构 |
| 语言特性 | cycle_gc | C++ | 1.542000 | 0.073100 | 21.09× | 参考仅处理单节点自环；通用图 GC 另见新增契约 |
| 语言特性 | cycle_gc | Python | 1.542000 | 1.739600 | 0.89× |  |
| 语言特性 | cycle_gc | Java | 1.542000 | 2.226500 | 0.69× | 参考只识别单节点自环；通用图 GC 见契约组 |
| library/组合 | math | C++ | 4.000000 | 2.133600 | 1.87× | 整数毫秒短项，倍率精度有限 |
| library/组合 | random | C++ | 1.000000 | 0.349300 | 2.86× | 整数毫秒短项，倍率精度有限 |
| library/组合 | string | C++ | 9.000000 | 2.606800 | 3.45× | 整数毫秒短项，倍率精度有限 |
| library/组合 | array | C++ | 18.000000 | 12.327700 | 1.46× | 整数毫秒短项，倍率精度有限 |
| library/组合 | path | C++ | 21.000000 | 8.473300 | 2.48× | 整数毫秒短项，倍率精度有限 |
| library/组合 | fs | C++ | 43.000000 | 41.481800 | 1.04× | 整数毫秒短项，倍率精度有限 |
| library/组合 | file | C++ | 1317.000000 | 1313.380900 | 1.00× | 整数毫秒短项，倍率精度有限 |
| library/组合 | io | C++ | 9.000000 | 7.857100 | 1.15× | 整数毫秒短项，倍率精度有限 |
| library/组合 | time | C++ | 4.000000 | 3.419900 | 1.17× | 整数毫秒短项，倍率精度有限 |
| audit/compute | algorithm_sort | C++ | 5.469000 | 3.205000 | 1.71× |  |
| audit/compute | algorithm_sort | Python | 5.469000 | 6.052000 | 0.90× |  |
| audit/compute | algorithm_sort | Java | 5.469000 | 7.332000 | 0.75× |  |
| audit/compute | bytes_hex | C++ | 8.617000 | 1.837000 | 4.69× |  |
| audit/compute | bytes_hex | Python | 8.617000 | 3.388000 | 2.54× |  |
| audit/compute | bytes_hex | Java | 8.617000 | 3.312000 | 2.60× |  |
| audit/compute | cancel_status | C++ | 0.573000 | 0.378000 | 1.52× |  |
| audit/compute | cancel_status | Python | 0.573000 | 18.327000 | 0.03× |  |
| audit/compute | cancel_status | Java | 0.573000 | 0.547000 | 1.05× |  |
| audit/compute | crypto_sha256 | C++ | 8.852000 | 4.517000 | 1.96× |  |
| audit/compute | crypto_sha256 | Python | 8.852000 | 5.715000 | 1.55× |  |
| audit/compute | crypto_sha256 | Java | 8.852000 | 15.224000 | 0.58× |  |
| audit/compute | csv_parse | Python | 24.532000 | 15.942000 | 1.54× |  |
| audit/compute | decimal_add | Python | 10.978000 | 4.143000 | 2.65× |  |
| audit/compute | decimal_add | Java | 10.978000 | 3.133000 | 3.50× |  |
| audit/compute | dictionary_contains | C++ | 0.362000 | 0.184000 | 1.97× |  |
| audit/compute | dictionary_contains | Python | 0.362000 | 7.852000 | 0.05× |  |
| audit/compute | dictionary_contains | Java | 0.362000 | 1.825000 | 0.20× |  |
| audit/compute | encoding_utf8 | Python | 4.330000 | 1.864000 | 2.32× |  |
| audit/compute | encoding_utf8 | Java | 4.330000 | 4.282000 | 1.01× |  |
| audit/compute | env_get | C++ | 9.844000 | 52.055000 | 0.19× |  |
| audit/compute | env_get | Python | 9.844000 | 5.569000 | 1.77× |  |
| audit/compute | env_get | Java | 9.844000 | 2.155000 | 4.57× |  |
| audit/compute | format_text | C++ | 1.231000 | 0.823000 | 1.50× |  |
| audit/compute | format_text | Python | 1.231000 | 2.034000 | 0.61× |  |
| audit/compute | format_text | Java | 1.231000 | 8.868000 | 0.14× |  |
| audit/compute | json_parse | Python | 23.194000 | 14.075000 | 1.65× |  |
| audit/compute | json_parse | Java | 23.194000 | 24.451000 | 0.95× |  |
| audit/compute | math_sqrt | C++ | 4.153000 | 2.099000 | 1.98× |  |
| audit/compute | math_sqrt | Python | 4.153000 | 61.468000 | 0.07× |  |
| audit/compute | math_sqrt | Java | 4.153000 | 2.484000 | 1.67× |  |
| audit/compute | parse_int | C++ | 11.408000 | 4.732000 | 2.41× |  |
| audit/compute | parse_int | Python | 11.408000 | 9.523000 | 1.20× |  |
| audit/compute | parse_int | Java | 11.408000 | 1.923000 | 5.93× |  |
| audit/compute | random_int | C++ | 1.390000 | 0.322000 | 4.32× |  |
| audit/compute | random_int | Python | 1.390000 | 69.696000 | 0.02× | 随机序列不同，仅作原始负载观察 |
| audit/compute | random_int | Java | 1.390000 | 2.181000 | 0.64× | 随机序列不同，仅作原始负载观察 |
| audit/compute | regex_search | C++ | 12.651000 | 10.524000 | 1.20× |  |
| audit/compute | regex_search | Python | 12.651000 | 11.952000 | 1.06× |  |
| audit/compute | regex_search | Java | 12.651000 | 8.802000 | 1.44× |  |
| audit/compute | serde_json | Python | 13.898000 | 35.075000 | 0.40× |  |
| audit/compute | serde_json | Java | 13.898000 | 39.343000 | 0.35× |  |
| audit/compute | statistics_mean | Python | 4.457000 | 281.474000 | 0.02× |  |
| audit/compute | unicode_nfc | Python | 5.347000 | 2.139000 | 2.50× |  |
| audit/compute | unicode_nfc | Java | 5.347000 | 13.679000 | 0.39× |  |
| audit/compute | xml_parse | Python | 32.989000 | 73.282000 | 0.45× |  |
| audit/compute | xml_parse | Java | 32.989000 | 168.754000 | 0.20× |  |
| audit/features | vector_push | C++ | 1.938000 | 0.484000 | 4.00× |  |
| audit/features | vector_push | Python | 1.938000 | 4.142000 | 0.47× |  |
| audit/features | vector_push | Java | 1.938000 | 2.107000 | 0.92× |  |
| audit/features | vector_index | C++ | 0.146000 | 0.207000 | 0.71× |  |
| audit/features | vector_index | Python | 0.146000 | 24.857000 | 0.01× |  |
| audit/features | vector_index | Java | 0.146000 | 2.560000 | 0.06× |  |
| audit/features | map_lookup | C++ | 0.325000 | 0.188000 | 1.73× |  |
| audit/features | map_lookup | Python | 0.325000 | 5.337000 | 0.06× |  |
| audit/features | map_lookup | Java | 0.325000 | 2.095000 | 0.16× |  |
| audit/features | set_contains | C++ | 0.259000 | 0.170000 | 1.52× |  |
| audit/features | set_contains | Python | 0.259000 | 8.308000 | 0.03× |  |
| audit/features | set_contains | Java | 0.259000 | 1.278000 | 0.20× |  |
| audit/features | heap_push_pop | C++ | 29.891000 | 3.029000 | 9.87× |  |
| audit/features | heap_push_pop | Python | 29.891000 | 17.187000 | 1.74× |  |
| audit/features | heap_push_pop | Java | 29.891000 | 8.579000 | 3.48× |  |
| audit/features | queue_push_pop | C++ | 1.119000 | 0.292000 | 3.83× |  |
| audit/features | queue_push_pop | Python | 1.119000 | 8.322000 | 0.13× |  |
| audit/features | queue_push_pop | Java | 1.119000 | 3.603000 | 0.31× |  |
| audit/features | iterator_snapshot | C++ | 1.468000 | 0.046000 | 31.91× |  |
| audit/features | iterator_snapshot | Python | 1.468000 | 4.121000 | 0.36× |  |
| audit/features | iterator_snapshot | Java | 1.468000 | 1.269000 | 1.16× |  |
| audit/features | option_value | C++ | 0.025000 | 0.013000 | 1.92× |  |
| audit/features | option_value | Python | 0.025000 | 18.088000 | 0.00× |  |
| audit/features | option_value | Java | 0.025000 | 1.377000 | 0.02× |  |
| audit/features | function_value | C++ | 0.286000 | 0.163000 | 1.75× |  |
| audit/features | function_value | Python | 0.286000 | 6.164000 | 0.05× |  |
| audit/features | function_value | Java | 0.286000 | 0.381000 | 0.75× |  |
| audit/features | closure_bind | C++ | 0.190000 | 0.164000 | 1.16× |  |
| audit/features | closure_bind | Python | 0.190000 | 8.398000 | 0.02× |  |
| audit/features | closure_bind | Java | 0.190000 | 0.616000 | 0.31× |  |
| audit/system | file_stream_rw | C++ | 843.336000 | 828.336000 | 1.02× |  |
| audit/system | file_stream_rw | Python | 843.336000 | 863.506000 | 0.98× |  |
| audit/system | file_stream_rw | Java | 843.336000 | 924.854000 | 0.91× |  |
| audit/system | process_spawn | C++ | 907.729000 | 2100.479000 | 0.43× |  |
| audit/system | process_spawn | Python | 907.729000 | 939.541000 | 0.97× |  |
| audit/system | process_spawn | Java | 907.729000 | 7026.159000 | 0.13× |  |
| network/http | requests_get | Python | 981.354000 | 1124.466000 | 0.87× |  |
| network/websocket | websocket_echo | C++ | 894.903000 | 880.701000 | 1.02× |  |
| network/websocket | websocket_echo | Python | 894.903000 | 511.816000 | 1.75× |  |
| network/websocket | websocket_echo | Java | 894.903000 | 404.010000 | 2.22× |  |
| 网络观察 | httpx_get / http_get | Java | 975.974000 | 253.905000 | 3.84× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 网络观察 | requests_get / http_get | Java | 981.354000 | 253.905000 | 3.87× | 客户端实现与生命周期不同，仅作 HTTP 端到端观察 |
| 综合 | vector_scan_1k | C++ | 0.262000 | 0.118000 | 2.22× |  |
| 综合 | vector_scan_1k | Python | 0.262000 | 25.430000 | 0.01× |  |
| 综合 | vector_scan_1k | Java | 0.262000 | 2.147000 | 0.12× |  |
| 综合 | vector_scan_100k | C++ | 2.479000 | 1.189000 | 2.08× |  |
| 综合 | vector_scan_100k | Python | 2.479000 | 235.182000 | 0.01× |  |
| 综合 | vector_scan_100k | Java | 2.479000 | 7.553000 | 0.33× |  |
| 综合 | vector_index_sequential | C++ | 0.301000 | 0.314000 | 0.96× |  |
| 综合 | vector_index_sequential | Python | 0.301000 | 70.298000 | 0.00× |  |
| 综合 | vector_index_sequential | Java | 0.301000 | 2.907000 | 0.10× |  |
| 综合 | vector_index_strided | C++ | 0.298000 | 0.304000 | 0.98× |  |
| 综合 | vector_index_strided | Python | 0.298000 | 71.099000 | 0.00× |  |
| 综合 | vector_index_strided | Java | 0.298000 | 3.215000 | 0.09× |  |
| 综合 | map_hit_128 | C++ | 1.606000 | 0.847000 | 1.90× |  |
| 综合 | map_hit_128 | Python | 1.606000 | 34.376000 | 0.05× |  |
| 综合 | map_hit_128 | Java | 1.606000 | 10.700000 | 0.15× |  |
| 综合 | map_hit_8192 | C++ | 2.215000 | 0.916000 | 2.42× |  |
| 综合 | map_hit_8192 | Python | 2.215000 | 42.140000 | 0.05× |  |
| 综合 | map_hit_8192 | Java | 2.215000 | 11.762000 | 0.19× |  |
| 综合 | map_hit_10_percent | C++ | 2.522000 | 1.573000 | 1.60× |  |
| 综合 | map_hit_10_percent | Python | 2.522000 | 48.110000 | 0.05× |  |
| 综合 | map_hit_10_percent | Java | 2.522000 | 9.883000 | 0.26× |  |
| 综合 | dictionary_int_hit | C++ | 2.364000 | 0.754000 | 3.14× |  |
| 综合 | dictionary_int_hit | Python | 2.364000 | 14.477000 | 0.16× |  |
| 综合 | dictionary_int_hit | Java | 2.364000 | 4.361000 | 0.54× |  |
| 综合 | dictionary_text_hit | C++ | 3.157000 | 4.400000 | 0.72× |  |
| 综合 | dictionary_text_hit | Python | 3.157000 | 16.786000 | 0.19× |  |
| 综合 | dictionary_text_hit | Java | 3.157000 | 3.355000 | 0.94× |  |
| 综合 | dictionary_mostly_miss | C++ | 0.845000 | 0.765000 | 1.10× |  |
| 综合 | dictionary_mostly_miss | Python | 0.845000 | 15.540000 | 0.05× |  |
| 综合 | dictionary_mostly_miss | Java | 0.845000 | 5.393000 | 0.16× |  |
| 综合 | format_literal | C++ | 6.574000 | 0.782000 | 8.41× | C++ 直接拼接固定内容，不是通用格式器 |
| 综合 | format_literal | Python | 6.574000 | 5.172000 | 1.27× |  |
| 综合 | format_literal | Java | 6.574000 | 16.084000 | 0.41× |  |
| 综合 | format_dynamic | C++ | 6.923000 | 1.041000 | 6.65× | 局部不可变模板，不能代表真正动态格式化；C++ 仅实现本负载的两占位符格式 |
| 综合 | format_dynamic | Python | 6.923000 | 10.459000 | 0.66× | 局部不可变模板，不能代表真正动态格式化 |
| 综合 | format_dynamic | Java | 6.923000 | 29.876000 | 0.23× | 局部不可变模板，不能代表真正动态格式化 |
| 综合 | encoding_literal | C++ | 12.491000 | 2.866000 | 4.36× |  |
| 综合 | encoding_literal | Python | 12.491000 | 5.637000 | 2.22× |  |
| 综合 | encoding_literal | Java | 12.491000 | 5.993000 | 2.08× |  |
| 综合 | encoding_dynamic | C++ | 11.221000 | 2.785000 | 4.03× |  |
| 综合 | encoding_dynamic | Python | 11.221000 | 5.495000 | 2.04× |  |
| 综合 | encoding_dynamic | Java | 11.221000 | 5.003000 | 2.24× |  |
| 综合 | serde_short_text | C++ | 9.825000 | 0.976000 | 10.07× |  |
| 综合 | serde_short_text | Python | 9.825000 | 17.383000 | 0.57× |  |
| 综合 | serde_short_text | Java | 9.825000 | 18.377000 | 0.53× |  |
| 综合 | serde_long_text | C++ | 8.712000 | 4.834000 | 1.80× |  |
| 综合 | serde_long_text | Python | 8.712000 | 19.777000 | 0.44× |  |
| 综合 | serde_long_text | Java | 8.712000 | 21.388000 | 0.41× |  |
| 综合 | parse_valid | C++ | 0.830000 | 1.745000 | 0.48× |  |
| 综合 | parse_valid | Python | 0.830000 | 13.102000 | 0.06× |  |
| 综合 | parse_valid | Java | 0.830000 | 13.163000 | 0.06× |  |
| 综合 | parse_invalid | C++ | 0.745000 | 6.248000 | 0.12× |  |
| 综合 | parse_invalid | Python | 0.745000 | 76.473000 | 0.01× |  |
| 综合 | parse_invalid | Java | 0.745000 | 177.627000 | 0.00× |  |
| 校准/diverse | vector_scan_1k | GCC C++ | 0.260000 | 0.117000 | 2.22× |  |
| 校准/diverse | vector_scan_100k | GCC C++ | 2.416000 | 1.173000 | 2.06× |  |
| 校准/diverse | vector_index_sequential | GCC C++ | 0.294000 | 0.333000 | 0.88× |  |
| 校准/diverse | vector_index_strided | GCC C++ | 0.293000 | 0.300000 | 0.98× |  |
| 校准/diverse | map_hit_128 | GCC C++ | 1.518000 | 0.830000 | 1.83× |  |
| 校准/diverse | map_hit_8192 | GCC C++ | 2.008000 | 0.844000 | 2.38× |  |
| 校准/diverse | map_hit_10_percent | GCC C++ | 1.898000 | 1.508000 | 1.26× |  |
| 校准/diverse | dictionary_int_hit | GCC C++ | 1.328000 | 0.746000 | 1.78× |  |
| 校准/diverse | dictionary_text_hit | GCC C++ | 2.239000 | 4.366000 | 0.51× |  |
| 校准/diverse | dictionary_mostly_miss | GCC C++ | 0.838000 | 0.813000 | 1.03× |  |
| 校准/diverse | format_literal | GCC C++ | 6.306000 | 0.782000 | 8.06× | C++ 直接拼接固定内容，不是通用格式器 |
| 校准/diverse | format_dynamic | GCC C++ | 6.219000 | 1.044000 | 5.96× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| 校准/diverse | encoding_literal | GCC C++ | 11.582000 | 2.806000 | 4.13× |  |
| 校准/diverse | encoding_dynamic | GCC C++ | 11.300000 | 2.664000 | 4.24× |  |
| 校准/diverse | serde_short_text | GCC C++ | 6.943000 | 0.975000 | 7.12× |  |
| 校准/diverse | serde_long_text | GCC C++ | 8.432000 | 4.412000 | 1.91× |  |
| 校准/diverse | parse_valid | GCC C++ | 0.764000 | 1.704000 | 0.45× |  |
| 校准/diverse | parse_invalid | GCC C++ | 0.727000 | 6.149000 | 0.12× |  |
| 校准/diverse_clang | vector_scan_1k | clang C++ | 0.261000 | 0.066000 | 3.95× |  |
| 校准/diverse_clang | vector_scan_100k | clang C++ | 2.434000 | 0.838000 | 2.90× |  |
| 校准/diverse_clang | vector_index_sequential | clang C++ | 0.295000 | 0.235000 | 1.26× |  |
| 校准/diverse_clang | vector_index_strided | clang C++ | 0.299000 | 0.240000 | 1.25× |  |
| 校准/diverse_clang | map_hit_128 | clang C++ | 1.552000 | 0.356000 | 4.36× |  |
| 校准/diverse_clang | map_hit_8192 | clang C++ | 2.015000 | 0.576000 | 3.50× |  |
| 校准/diverse_clang | map_hit_10_percent | clang C++ | 2.039000 | 1.255000 | 1.62× |  |
| 校准/diverse_clang | dictionary_int_hit | clang C++ | 1.289000 | 0.881000 | 1.46× |  |
| 校准/diverse_clang | dictionary_text_hit | clang C++ | 2.281000 | 4.383000 | 0.52× |  |
| 校准/diverse_clang | dictionary_mostly_miss | clang C++ | 0.833000 | 0.881000 | 0.95× |  |
| 校准/diverse_clang | format_literal | clang C++ | 6.346000 | 0.647000 | 9.81× | C++ 直接拼接固定内容，不是通用格式器 |
| 校准/diverse_clang | format_dynamic | clang C++ | 6.229000 | 0.919000 | 6.78× | TX 局部不可变模板；C++ 仅实现本负载的两占位符格式 |
| 校准/diverse_clang | encoding_literal | clang C++ | 11.720000 | 3.227000 | 3.63× |  |
| 校准/diverse_clang | encoding_dynamic | clang C++ | 11.079000 | 2.845000 | 3.89× |  |
| 校准/diverse_clang | serde_short_text | clang C++ | 7.818000 | 1.025000 | 7.63× |  |
| 校准/diverse_clang | serde_long_text | clang C++ | 8.588000 | 5.102000 | 1.68× |  |
| 校准/diverse_clang | parse_valid | clang C++ | 0.781000 | 0.971000 | 0.80× |  |
| 校准/diverse_clang | parse_invalid | clang C++ | 0.747000 | 5.427000 | 0.14× |  |
| 校准/heap | heap_push_pop | GCC C++ | 29.625000 | 3.656000 | 8.10× |  |
| 校准/heap_clang | heap_push_pop | clang C++ | 34.858000 | 4.759000 | 7.32× |  |
| 校准/statistics | statistics_mean | GCC C++ | 4.520000 | 2.935000 | 1.54× |  |
| 校准/statistics_clang | statistics_mean | clang C++ | 4.485000 | 4.187000 | 1.07× |  |
| 外部题/mini-filesystem | fanout-2000 | C++ | 11.182000 | 1.671000 | 6.69× |  |
| 外部题/mini-filesystem | fanout-2000 | JavaScript | 11.182000 | 5.481000 | 2.04× |  |
| 外部题/mini-filesystem | fanout-2000 | Java | 11.182000 | 7.545000 | 1.48× |  |
| 外部题/mini-filesystem | fanout-2000 | Python | 11.182000 | 4.042000 | 2.77× |  |
| 外部题/mini-filesystem | deep-pwd-2000 | C++ | 12.568000 | 15.227000 | 0.83× |  |
| 外部题/mini-filesystem | deep-pwd-2000 | JavaScript | 12.568000 | 10.217000 | 1.23× |  |
| 外部题/mini-filesystem | deep-pwd-2000 | Java | 12.568000 | 33.507000 | 0.38× |  |
| 外部题/mini-filesystem | deep-pwd-2000 | Python | 12.568000 | 11.968000 | 1.05× |  |
| 外部题/mini-filesystem | moves-2000 | C++ | 5.442000 | 1.017000 | 5.35× |  |
| 外部题/mini-filesystem | moves-2000 | JavaScript | 5.442000 | 1.922000 | 2.83× |  |
| 外部题/mini-filesystem | moves-2000 | Java | 5.442000 | 5.941000 | 0.92× |  |
| 外部题/mini-filesystem | moves-2000 | Python | 5.442000 | 2.120000 | 2.57× |  |
| 外部题/mini-filesystem | random-2000-1 | C++ | 6.399000 | 0.997000 | 6.42× |  |
| 外部题/mini-filesystem | random-2000-1 | JavaScript | 6.399000 | 2.175000 | 2.94× |  |
| 外部题/mini-filesystem | random-2000-1 | Java | 6.399000 | 6.291000 | 1.02× |  |
| 外部题/mini-filesystem | random-2000-1 | Python | 6.399000 | 2.598000 | 2.46× |  |
| 外部题/mini-filesystem | linklong-2000 | C++ | 32.177000 | 109.947000 | 0.29× |  |
| 外部题/mini-filesystem | linklong-2000 | JavaScript | 32.177000 | 121.377000 | 0.27× |  |
| 外部题/mini-filesystem | linklong-2000 | Java | 32.177000 | 273.926000 | 0.12× |  |
| 外部题/mini-filesystem | linklong-2000 | Python | 32.177000 | 454.160000 | 0.07× |  |
| 外部题/not-yet-on-stage | all-free-max | C++ | 0.993000 | 1.416000 | 0.70× |  |
| 外部题/not-yet-on-stage | all-free-max | JavaScript | 0.993000 | 3.741000 | 0.27× |  |
| 外部题/not-yet-on-stage | all-free-max | Java | 0.993000 | 4.624000 | 0.21× |  |
| 外部题/not-yet-on-stage | all-free-max | Python | 0.993000 | 44.048000 | 0.02× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | C++ | 4.299000 | 1.418000 | 3.03× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | JavaScript | 4.299000 | 5.242000 | 0.82× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | Java | 4.299000 | 5.628000 | 0.76× |  |
| 外部题/not-yet-on-stage | forced-increasing-max | Python | 4.299000 | 77.089000 | 0.06× |  |
| 外部题/not-yet-on-stage | forced-decreasing-max | C++ | 1.010000 | 1.438000 | 0.70× |  |
| 外部题/not-yet-on-stage | forced-decreasing-max | JavaScript | 1.010000 | 3.936000 | 0.26× |  |
| 外部题/not-yet-on-stage | forced-decreasing-max | Java | 1.010000 | 4.992000 | 0.20× |  |
| 外部题/not-yet-on-stage | forced-decreasing-max | Python | 1.010000 | 44.891000 | 0.02× |  |
| 外部题/not-yet-on-stage | shuffled-tight-max-1 | C++ | 3.280000 | 1.925000 | 1.70× |  |
| 外部题/not-yet-on-stage | shuffled-tight-max-1 | JavaScript | 3.280000 | 5.049000 | 0.65× |  |
| 外部题/not-yet-on-stage | shuffled-tight-max-1 | Java | 3.280000 | 5.629000 | 0.58× |  |
| 外部题/not-yet-on-stage | shuffled-tight-max-1 | Python | 3.280000 | 64.534000 | 0.05× |  |
| 外部题/not-yet-on-stage | alternating-tight-max | C++ | 2.701000 | 1.435000 | 1.88× |  |
| 外部题/not-yet-on-stage | alternating-tight-max | JavaScript | 2.701000 | 4.635000 | 0.58× |  |
| 外部题/not-yet-on-stage | alternating-tight-max | Java | 2.701000 | 5.613000 | 0.48× |  |
| 外部题/not-yet-on-stage | alternating-tight-max | Python | 2.701000 | 60.739000 | 0.04× |  |
| 长随机数 | random_long | C++ | 41.000000 | 10.104800 | 4.06× |  |
| 契约/format_contract | format_alternating | C++ | 17.629000 | 3.262000 | 5.40× |  |
| 契约/format_contract | format_parameter | C++ | 17.913000 | 3.239000 | 5.53× |  |
| 契约/graph_contract | graph_copy | C++ | 7.334000 | 2.315000 | 3.17× | 直接链接 TX 运行时；不代表前端生成代码的全部语义 |
| 契约/graph_contract | graph_gc | C++ | 10.423000 | 8.457000 | 1.23× | 直接链接 TX 运行时；不代表前端生成代码的全部语义 |
| 新增/concurrency | thread_spawn_join | C++ | 13.854000 | 14.397400 | 0.96× | C++ std::thread；Java 平台线程；Python threading。含线程创建/退出。 |
| 新增/concurrency | thread_spawn_join | Java | 13.854000 | 26.028700 | 0.53× | C++ std::thread；Java 平台线程；Python threading。含线程创建/退出。 |
| 新增/concurrency | thread_spawn_join | Python | 13.854000 | 17.329400 | 0.80× | C++ std::thread；Java 平台线程；Python threading。含线程创建/退出。 |
| 新增/concurrency | mutex_uncontended | C++ | 20.517000 | 0.481100 | 42.65× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 新增/concurrency | mutex_uncontended | Java | 20.517000 | 6.021400 | 3.41× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 新增/concurrency | mutex_uncontended | Python | 20.517000 | 15.241400 | 1.35× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 新增/concurrency | atomic_add | C++ | 0.384000 | 0.177600 | 2.16× | C++ std::atomic、Java AtomicLong；Python 没有对应公开原子整数，使用 Lock 实现线程安全加法，不能当成原子指令性能。 |
| 新增/concurrency | atomic_add | Java | 0.384000 | 1.117500 | 0.34× | C++ std::atomic、Java AtomicLong；Python 没有对应公开原子整数，使用 Lock 实现线程安全加法，不能当成原子指令性能。 |
| 新增/concurrency | atomic_add | Python | 0.384000 | 15.661900 | 0.02× | C++ std::atomic、Java AtomicLong；Python 没有对应公开原子整数，使用 Lock 实现线程安全加法，不能当成原子指令性能。 |
| 新增/concurrency | channel_send_recv | C++ | 1.978000 | 0.787300 | 2.51× | C++ mutex/condition_variable 队列；Java ArrayBlockingQueue；Python Queue。都是立即成功路径；TX 另有取消及句柄规则，不代表多生产者吞吐。 |
| 新增/concurrency | channel_send_recv | Java | 1.978000 | 3.931900 | 0.50× | C++ mutex/condition_variable 队列；Java ArrayBlockingQueue；Python Queue。都是立即成功路径；TX 另有取消及句柄规则，不代表多生产者吞吐。 |
| 新增/concurrency | channel_send_recv | Python | 1.978000 | 30.663900 | 0.06× | C++ mutex/condition_variable 队列；Java ArrayBlockingQueue；Python Queue。都是立即成功路径；TX 另有取消及句柄规则，不代表多生产者吞吐。 |
| 新增/concurrency | task_spawn_wait | C++ | 7.506000 | 6.949300 | 1.08× | 参考使用单工作线程池；TX 使用任务运行时和 scope。包含组内初次线程池启动，不代表饱和并发吞吐。 |
| 新增/concurrency | task_spawn_wait | Java | 7.506000 | 21.862200 | 0.34× | 参考使用单工作线程池；TX 使用任务运行时和 scope。包含组内初次线程池启动，不代表饱和并发吞吐。 |
| 新增/concurrency | task_spawn_wait | Python | 7.506000 | 17.057000 | 0.44× | 参考使用单工作线程池；TX 使用任务运行时和 scope。包含组内初次线程池启动，不代表饱和并发吞吐。 |
| 新增/sqlite | sqlite_insert | C++ | 11.422000 | 8.403300 | 1.36× | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| 新增/sqlite | sqlite_insert | Java | 11.422000 | 28.877300 | 0.40× | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| 新增/sqlite | sqlite_insert | Python | 11.422000 | 1.914800 | 5.97× | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| 新增/sqlite | sqlite_read | C++ | 26.133000 | 8.819400 | 2.96× | C++ 使用同一后端及行快照；Java JDBC、Python sqlite3。TX/C++ 差值包含 ABI、option、行读取接口与生成代码开销。 |
| 新增/sqlite | sqlite_read | Java | 26.133000 | 27.484300 | 0.95× | C++ 使用同一后端及行快照；Java JDBC、Python sqlite3。TX/C++ 差值包含 ABI、option、行读取接口与生成代码开销。 |
| 新增/sqlite | sqlite_read | Python | 26.133000 | 9.418000 | 2.77× | C++ 使用同一后端及行快照；Java JDBC、Python sqlite3。TX/C++ 差值包含 ABI、option、行读取接口与生成代码开销。 |
| 新增/sqlite | sqlite_savepoint | C++ | 2.622000 | 1.862300 | 1.41× | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| 新增/sqlite | sqlite_savepoint | Java | 2.622000 | 6.717100 | 0.39× | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| 新增/sqlite | sqlite_savepoint | Python | 2.622000 | 0.653100 | 4.01× | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| 新增/sqlite | sqlite_pool | C++ | 137.792000 | 44.204900 | 3.12× | TX 与 C++ 测真实池；其 SQLite 归还策略为关闭重开。Java/Python 参考每次连接关闭，保持生命周期，未复制完整池等待/别名检查。 |
| 新增/sqlite | sqlite_pool | Java | 137.792000 | 77.011900 | 1.79× | TX 与 C++ 测真实池；其 SQLite 归还策略为关闭重开。Java/Python 参考每次连接关闭，保持生命周期，未复制完整池等待/别名检查。 |
| 新增/sqlite | sqlite_pool | Python | 137.792000 | 52.102700 | 2.64× | TX 与 C++ 测真实池；其 SQLite 归还策略为关闭重开。Java/Python 参考每次连接关闭，保持生命周期，未复制完整池等待/别名检查。 |
| 新增/sqlite | sqlite_async | C++ | 54.773000 | 46.802900 | 1.17× | TX 任务运行时；C++ 同异步数据库后端加单线程执行器；Java/Python 单线程池。都包含单独事务与连接关闭。 |
| 新增/sqlite | sqlite_async | Java | 54.773000 | 74.162400 | 0.74× | TX 任务运行时；C++ 同异步数据库后端加单线程执行器；Java/Python 单线程池。都包含单独事务与连接关闭。 |
| 新增/sqlite | sqlite_async | Python | 54.773000 | 52.461700 | 1.04× | TX 任务运行时；C++ 同异步数据库后端加单线程执行器；Java/Python 单线程池。都包含单独事务与连接关闭。 |
| 新增/sqlite | migration_recheck | C++ | 2.117000 | 2.119900 | 1.00× | C++ 同迁移后端；Java/Python 实现相同长度分帧 SHA-256、两次 IMMEDIATE 事务及账本读取；只覆盖幂等成功路径。 |
| 新增/sqlite | migration_recheck | Java | 2.117000 | 10.749700 | 0.20× | C++ 同迁移后端；Java/Python 实现相同长度分帧 SHA-256、两次 IMMEDIATE 事务及账本读取；只覆盖幂等成功路径。 |
| 新增/sqlite | migration_recheck | Python | 2.117000 | 1.182800 | 1.79× | C++ 同迁移后端；Java/Python 实现相同长度分帧 SHA-256、两次 IMMEDIATE 事务及账本读取；只覆盖幂等成功路径。 |
| 新增/postgres | postgres_insert | C++ | 189.458000 | 178.003000 | 1.06× | C++ 同 libpq 后端；Java PostgreSQL JDBC；Python psycopg。驱动 prepare 缓存策略不同；共用临时 PostgreSQL 18.4，不连接已有数据库。 |
| 新增/postgres | postgres_insert | Java | 189.458000 | 277.866600 | 0.68× | C++ 同 libpq 后端；Java PostgreSQL JDBC；Python psycopg。驱动 prepare 缓存策略不同；共用临时 PostgreSQL 18.4，不连接已有数据库。 |
| 新增/postgres | postgres_insert | Python | 189.458000 | 258.501100 | 0.73× | C++ 同 libpq 后端；Java PostgreSQL JDBC；Python psycopg。驱动 prepare 缓存策略不同；共用临时 PostgreSQL 18.4，不连接已有数据库。 |
| 新增/postgres | postgres_read | C++ | 41.141000 | 23.133700 | 1.78× | TX/C++ libpq 单行模式；Java/Python 默认结果获取策略不同。C++ 同后端可用于定位包装层开销，跨驱动比例是端到端负载观察。 |
| 新增/postgres | postgres_read | Java | 41.141000 | 31.155100 | 1.32× | TX/C++ libpq 单行模式；Java/Python 默认结果获取策略不同。C++ 同后端可用于定位包装层开销，跨驱动比例是端到端负载观察。 |
| 新增/postgres | postgres_read | Python | 41.141000 | 18.738400 | 2.20× | TX/C++ libpq 单行模式；Java/Python 默认结果获取策略不同。C++ 同后端可用于定位包装层开销，跨驱动比例是端到端负载观察。 |
| 新增/postgres | postgres_savepoint | C++ | 115.403000 | 111.057000 | 1.04× | C++ 同后端；Java/Python 各自驱动。所有操作在真实服务器执行，受本机网络调度影响。 |
| 新增/postgres | postgres_savepoint | Java | 115.403000 | 116.986700 | 0.99× | C++ 同后端；Java/Python 各自驱动。所有操作在真实服务器执行，受本机网络调度影响。 |
| 新增/postgres | postgres_savepoint | Python | 115.403000 | 114.003900 | 1.01× | C++ 同后端；Java/Python 各自驱动。所有操作在真实服务器执行，受本机网络调度影响。 |
| 新增/security | secret_equal | C++ | 14.163000 | 8.730200 | 1.62× | C++ 同 secret 后端；Java MessageDigest.isEqual；Python hmac.compare_digest。Java/Python 普通字节数组没有 TX 秘密句柄的保护与清零生命周期。 |
| 新增/security | secret_equal | Java | 14.163000 | 26.825100 | 0.53× | C++ 同 secret 后端；Java MessageDigest.isEqual；Python hmac.compare_digest。Java/Python 普通字节数组没有 TX 秘密句柄的保护与清零生命周期。 |
| 新增/security | secret_equal | Python | 14.163000 | 11.533600 | 1.23× | C++ 同 secret 后端；Java MessageDigest.isEqual；Python hmac.compare_digest。Java/Python 普通字节数组没有 TX 秘密句柄的保护与清零生命周期。 |
| 新增/security | argon2_hash_verify | C++ | 163.263000 | 172.649000 | 0.95× | m=19456 KiB、t=2、p=1、v=19、16 字节随机 salt、32 字节输出；C++ 同 Argon2 后端，Java BouncyCastle，Python argon2-cffi；含 PHC 编解码。 |
| 新增/security | argon2_hash_verify | Java | 163.263000 | 525.133700 | 0.31× | m=19456 KiB、t=2、p=1、v=19、16 字节随机 salt、32 字节输出；C++ 同 Argon2 后端，Java BouncyCastle，Python argon2-cffi；含 PHC 编解码。 |
| 新增/security | argon2_hash_verify | Python | 163.263000 | 171.046300 | 0.95× | m=19456 KiB、t=2、p=1、v=19、16 字节随机 salt、32 字节输出；C++ 同 Argon2 后端，Java BouncyCastle，Python argon2-cffi；含 PHC 编解码。 |
| 新增/security | ed25519_sign | C++ | 17.126000 | 17.177600 | 1.00× | 共用临时随机 seed；C++ 同 libsodium 后端；Java JCA Ed25519；Python cryptography。密钥导入不计时，签名随后验签。 |
| 新增/security | ed25519_sign | Java | 17.126000 | 411.654900 | 0.04× | 共用临时随机 seed；C++ 同 libsodium 后端；Java JCA Ed25519；Python cryptography。密钥导入不计时，签名随后验签。 |
| 新增/security | ed25519_sign | Python | 17.126000 | 16.778900 | 1.02× | 共用临时随机 seed；C++ 同 libsodium 后端；Java JCA Ed25519；Python cryptography。密钥导入不计时，签名随后验签。 |
| 新增/security | ed25519_verify | C++ | 24.329000 | 24.418000 | 1.00× | 三类后端验证各自生成的签名；同一 seed 和消息，成功次数逐轮核对。 |
| 新增/security | ed25519_verify | Java | 24.329000 | 349.294200 | 0.07× | 三类后端验证各自生成的签名；同一 seed 和消息，成功次数逐轮核对。 |
| 新增/security | ed25519_verify | Python | 24.329000 | 45.542200 | 0.53× | 三类后端验证各自生成的签名；同一 seed 和消息，成功次数逐轮核对。 |
| 新增/security | x509_parse_der | C++ | 2.965000 | 2.540500 | 1.17× | C++ 同 Windows 证书后端；Java CertificateFactory 可能缓存；Python cryptography。测重复证书热路径，不外推到大量不同证书。 |
| 新增/security | x509_parse_der | Java | 2.965000 | 6.919000 | 0.43× | C++ 同 Windows 证书后端；Java CertificateFactory 可能缓存；Python cryptography。测重复证书热路径，不外推到大量不同证书。 |
| 新增/security | x509_parse_der | Python | 2.965000 | 3.014000 | 0.98× | C++ 同 Windows 证书后端；Java CertificateFactory 可能缓存；Python cryptography。测重复证书热路径，不外推到大量不同证书。 |
| 新增/network | dns_localhost | C++ | 0.266000 | 34.210600 | 0.01× | TX 对 localhost 有专用路径；C++ getaddrinfo、Java InetAddress、Python getaddrinfo 各有不同缓存策略。不能用于比较远端 DNS。 |
| 新增/network | dns_localhost | Java | 0.266000 | 55.987800 | 0.00× | TX 对 localhost 有专用路径；C++ getaddrinfo、Java InetAddress、Python getaddrinfo 各有不同缓存策略。不能用于比较远端 DNS。 |
| 新增/network | dns_localhost | Python | 0.266000 | 42.819700 | 0.01× | TX 对 localhost 有专用路径；C++ getaddrinfo、Java InetAddress、Python getaddrinfo 各有不同缓存策略。不能用于比较远端 DNS。 |
| 新增/network | udp_echo | C++ | 34.354000 | 25.188500 | 1.36× | 四语言使用同一 Python 回声服务，逐次完整比较 payload。服务端调度也计入端到端时间。 |
| 新增/network | udp_echo | Java | 34.354000 | 43.974800 | 0.78× | 四语言使用同一 Python 回声服务，逐次完整比较 payload。服务端调度也计入端到端时间。 |
| 新增/network | udp_echo | Python | 34.354000 | 30.688700 | 1.12× | 四语言使用同一 Python 回声服务，逐次完整比较 payload。服务端调度也计入端到端时间。 |
| 新增/network | ipc_echo | C++ | 12.771000 | 8.530500 | 1.50× | 使用同一服务器、22 字节帧和 CBOR 整数 42。C++/Java/Python 固定整数编解码；TX 为通用 CBOR 与资源句柄，不代表任意对象 IPC 差距。 |
| 新增/network | ipc_echo | Java | 12.771000 | 12.044000 | 1.06× | 使用同一服务器、22 字节帧和 CBOR 整数 42。C++/Java/Python 固定整数编解码；TX 为通用 CBOR 与资源句柄，不代表任意对象 IPC 差距。 |
| 新增/network | ipc_echo | Python | 12.771000 | 9.483300 | 1.35× | 使用同一服务器、22 字节帧和 CBOR 整数 42。C++/Java/Python 固定整数编解码；TX 为通用 CBOR 与资源句柄，不代表任意对象 IPC 差距。 |
| 新增/network | tls_handshake | C++ | 144.254000 | 25.348400 | 5.69× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 新增/network | tls_handshake | Java | 144.254000 | 228.462700 | 0.63× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 新增/network | tls_handshake | Python | 144.254000 | 73.864400 | 1.95× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 新增/async_file | async_file_rw | C++ | 38.012000 | 32.662600 | 1.16× | TX IOCP、Java AsynchronousFileChannel；C++/Python 工作线程执行定位 I/O。全部逐次等待并核对字节，不代表大量在途 I/O 吞吐。 |
| 新增/async_file | async_file_rw | Java | 38.012000 | 83.576900 | 0.45× | TX IOCP、Java AsynchronousFileChannel；C++/Python 工作线程执行定位 I/O。全部逐次等待并核对字节，不代表大量在途 I/O 吞吐。 |
| 新增/async_file | async_file_rw | Python | 38.012000 | 76.584700 | 0.50× | TX IOCP、Java AsynchronousFileChannel；C++/Python 工作线程执行定位 I/O。全部逐次等待并核对字节，不代表大量在途 I/O 吞吐。 |
| 新增/diagnostics | test_parameterized | C++ | 0.223000 | 0.056200 | 3.97× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 新增/diagnostics | test_parameterized | Java | 0.223000 | 0.825600 | 0.27× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 新增/diagnostics | test_parameterized | Python | 0.223000 | 1.513300 | 0.15× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 新增/diagnostics | test_property | C++ | 0.334000 | 0.046500 | 7.18× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 新增/diagnostics | test_property | Java | 0.334000 | 1.107300 | 0.30× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 新增/diagnostics | test_property | Python | 0.334000 | 2.317400 | 0.14× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 新增/diagnostics | log_filtered | C++ | 0.901000 | 0.422000 | 2.14× | C++ 调用真实 TX 日志后端级别检查，Java Logger，Python logging；各参考仅在级别允许时构造字段，TX 另有闭包/错误边界。废弃初版 C++ volatile 检查倍率。 |
| 新增/diagnostics | log_filtered | Java | 0.901000 | 4.020900 | 0.22× | C++ 调用真实 TX 日志后端级别检查，Java Logger，Python logging；各参考仅在级别允许时构造字段，TX 另有闭包/错误边界。废弃初版 C++ volatile 检查倍率。 |
| 新增/diagnostics | log_filtered | Python | 0.901000 | 6.418600 | 0.14× | C++ 调用真实 TX 日志后端级别检查，Java Logger，Python logging；各参考仅在级别允许时构造字段，TX 另有闭包/错误边界。废弃初版 C++ volatile 检查倍率。 |
| 新增/diagnostics | log_file | C++ | 31.396000 | 28.779500 | 1.09× | C++ 同日志后端；Java/Python 固定 schema 构造。全部每条 flush、进程内加锁；计时外核对行数、索引、请求标识、遮蔽值。未覆盖轮转压力。 |
| 新增/diagnostics | log_file | Java | 31.396000 | 52.740600 | 0.60× | C++ 同日志后端；Java/Python 固定 schema 构造。全部每条 flush、进程内加锁；计时外核对行数、索引、请求标识、遮蔽值。未覆盖轮转压力。 |
| 新增/diagnostics | log_file | Python | 31.396000 | 79.632600 | 0.39× | C++ 同日志后端；Java/Python 固定 schema 构造。全部每条 flush、进程内加锁；计时外核对行数、索引、请求标识、遮蔽值。未覆盖轮转压力。 |
| 新增/profile | profile_spans | C++ | 0.471000 | 0.227400 | 2.07× | TX/C++ 启用真实分析器；Java/Python 仅保存时间段记录，没有同时启用 CPU/分配采样。仅为 API 负载参考，不能宣称完整 profiler 等价。 |
| 新增/profile | profile_spans | Java | 0.471000 | 1.058300 | 0.45× | TX/C++ 启用真实分析器；Java/Python 仅保存时间段记录，没有同时启用 CPU/分配采样。仅为 API 负载参考，不能宣称完整 profiler 等价。 |
| 新增/profile | profile_spans | Python | 0.471000 | 0.336500 | 1.40× | TX/C++ 启用真实分析器；Java/Python 仅保存时间段记录，没有同时启用 CPU/分配采样。仅为 API 负载参考，不能宣称完整 profiler 等价。 |
