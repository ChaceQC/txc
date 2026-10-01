# Linux 全量代表负载性能测试 — 2026-10-01

测试对象为现有 `tx/linux` 原生工具包，工作树提交 `9a8ba2e789a57a2801a04d4ef8ebae3aef5f652c`。本次没有重建或修改编译器实现，工具包实际文件 SHA-256 已记录。

完成 **30 组、181 个性能条目**（不同专项含重复负载），覆盖现有映射中 **52/52 个公开模块**。每组预热 1 轮、正式串行采样 5 轮，以内部计时中位数报告。

输入指纹稳定：**True**；未解决失败：**0**；计算/容器与历史确定性校验值核对：31 项，差异 0 项。历史 Windows 耗时不参与性能倍率计算。

## 环境与范围

- Linux-6.18.33.2-microsoft-standard-WSL2-x86_64-with-glibc2.39；Ubuntu 24.04，GCC 13.3.0、Clang 18.1.3；完整 CPU、版本与输入指纹见 `manifest.json`。
- 运行位置为 WSL2 的 `/mnt/e` Windows 挂载盘。文件、日志、进程启动和编译耗时含 WSL/跨文件系统开销，不代表原生 Linux ext4 磁盘成绩。
- TX 使用现有工具包默认优化/ThinLTO 路径；C++ 对照为 GCC `-O3 -DNDEBUG`。C++ 对照覆盖语言、容器、综合、库、格式及图契约；语言特性另含 Python 对照。没有执行 Java/JavaScript 或完整标准库四语言对照。
- 外部题使用原有 10 个性能场景，并核对全部正式输入输出。PostgreSQL 使用本次独立解包的 16.x 临时服务端，TLS、UDP、IPC、HTTP、WebSocket 均为本机服务，结束后关闭。
- 覆盖指每个模块具有代表性性能负载，不是每个 API、所有错误路径、饱和并发、长时间压力或所有网络协议全覆盖。
- 对照语义沿用各基准 README：C++ 的部分对象/深拷贝/循环回收为专门实现；固定格式拼接与通用格式器不同；倍率仅针对相应程序。

## 同轮 C++ 对照中最大的差距

| 套件 | 项目 | TX ms | C++ ms | TX/C++ |
| --- | --- | ---: | ---: | ---: |
| library | dict_hash | 1.000 | 0.064 | 15.57× |
| language | cycle_gc | 0.711 | 0.054 | 13.28× |
| diverse | serde_short_text | 5.924 | 0.530 | 11.18× |
| language | deinit | 3.872 | 0.385 | 10.05× |
| diverse | vector_scan_1k | 1.299 | 0.135 | 9.62× |
| features | queue_push_pop | 1.286 | 0.134 | 9.60× |
| language | deep_copy | 10.594 | 1.129 | 9.38× |
| mini-filesystem | moves-2000 | 2.949 | 0.341 | 8.65× |
| language | variadic_unpack | 4.068 | 0.629 | 6.47× |
| mini-filesystem | random-2000-1 | 3.273 | 0.565 | 5.79× |
| language | scalar_control | 1.339 | 0.247 | 5.42× |
| features | iterator_snapshot | 0.105 | 0.021 | 5.00× |
| mini-filesystem | fanout-2000 | 5.111 | 1.038 | 4.92× |
| language | class_methods | 0.739 | 0.170 | 4.35× |
| language | string_conversion | 4.257 | 1.113 | 3.82× |

## 全部性能条目

| 套件 | 项目 | TX / TX runtime ms | C++ ms | Python ms |
| --- | --- | ---: | ---: | ---: |
| language | scalar_control | 1.339 | 0.247 | 21.647 |
| language | updates | 1.534 | 1.375 | 62.585 |
| language | while_logic | 0.187 | 0.185 | 29.492 |
| language | float_arithmetic | 0.389 | 0.373 | 23.929 |
| language | overloads | 0.199 | 0.198 | 18.604 |
| language | named_arguments | 0.407 | 0.395 | 24.342 |
| language | recursion | 0.043 | 0.073 | 9.935 |
| language | variadic_unpack | 4.068 | 0.629 | 3.784 |
| language | array_destructure | 0.010 | 0.871 | 4.294 |
| language | array_padded | 0.025 | 1.444 | 8.086 |
| language | dict_iteration | 5.051 | 5.224 | 20.952 |
| language | struct_operators | 0.026 | 5.065 | 80.947 |
| language | class_methods | 0.739 | 0.170 | 44.705 |
| language | virtual_interface | 1.368 | 0.504 | 36.520 |
| language | class_operator | 0.673 | 0.225 | 16.048 |
| language | runtime_cast | 0.276 | 2.043 | 11.651 |
| language | module_call | 0.025 | 1.487 | 25.963 |
| language | string_conversion | 4.257 | 1.113 | 10.468 |
| language | deep_copy | 10.594 | 1.129 | 68.233 |
| language | copy_cycle | 3.356 | 0.976 | 10.962 |
| language | deinit | 3.872 | 0.385 | 3.902 |
| language | cycle_gc | 0.711 | 0.054 | 1.319 |
| library | math | 7.000 | 2.364 | — |
| library | random | 1.000 | 0.376 | — |
| library | string | 4.000 | 1.177 | — |
| library | array | 13.000 | 11.387 | — |
| library | path | 9.000 | 2.679 | — |
| library | fs | 850.000 | 851.319 | — |
| library | file | 2464.000 | 2470.084 | — |
| library | io | 4.000 | 3.359 | — |
| library | time | 3.000 | 2.492 | — |
| library | dict_dynamic | 1.000 | 17.507 | — |
| library | dict_hash | 1.000 | 0.064 | — |
| compute | algorithm_sort | 5.372 | — | — |
| compute | bytes_hex | 3.012 | — | — |
| compute | cancel_status | 0.904 | — | — |
| compute | cbor_roundtrip | 10.888 | — | — |
| compute | crypto_sha256 | 6.205 | — | — |
| compute | csv_parse | 10.347 | — | — |
| compute | decimal_add | 3.613 | — | — |
| compute | dictionary_contains | 0.270 | — | — |
| compute | encoding_utf8 | 0.256 | — | — |
| compute | env_get | 2.716 | — | — |
| compute | format_text | 0.110 | — | — |
| compute | json_parse | 12.745 | — | — |
| compute | math_sqrt | 5.286 | — | — |
| compute | parse_int | 3.621 | — | — |
| compute | random_int | 0.818 | — | — |
| compute | regex_search | 8.082 | — | — |
| compute | serde_json | 8.790 | — | — |
| compute | statistics_mean | 4.423 | — | — |
| compute | test_assert | 2.012 | — | — |
| compute | unicode_nfc | 7.426 | — | — |
| compute | xml_parse | 30.707 | — | — |
| features | vector_push | 2.884 | 1.123 | — |
| features | vector_index | 0.232 | 0.126 | — |
| features | map_lookup | 0.513 | 0.205 | — |
| features | set_contains | 0.347 | 0.183 | — |
| features | heap_push_pop | 7.670 | 3.233 | — |
| features | queue_push_pop | 1.286 | 0.134 | — |
| features | iterator_snapshot | 0.105 | 0.021 | — |
| features | option_value | 0.026 | 0.014 | — |
| features | function_value | 0.231 | 0.151 | — |
| features | closure_bind | 0.204 | 0.151 | — |
| diverse | vector_scan_1k | 1.299 | 0.135 | — |
| diverse | vector_scan_100k | 2.879 | 1.292 | — |
| diverse | vector_index_sequential | 0.469 | 0.347 | — |
| diverse | vector_index_strided | 0.428 | 0.331 | — |
| diverse | map_hit_128 | 2.136 | 0.991 | — |
| diverse | map_hit_8192 | 2.532 | 0.917 | — |
| diverse | map_hit_10_percent | 2.133 | 1.703 | — |
| diverse | dictionary_int_hit | 0.991 | 0.822 | — |
| diverse | dictionary_text_hit | 2.614 | 5.171 | — |
| diverse | dictionary_mostly_miss | 0.604 | 0.921 | — |
| diverse | format_literal | 0.685 | 0.919 | — |
| diverse | format_dynamic | 0.649 | 1.070 | — |
| diverse | encoding_literal | 0.798 | 2.264 | — |
| diverse | encoding_dynamic | 0.844 | 2.244 | — |
| diverse | serde_short_text | 5.924 | 0.530 | — |
| diverse | serde_long_text | 5.141 | 3.664 | — |
| diverse | parse_valid | 0.668 | 1.875 | — |
| diverse | parse_invalid | 0.669 | 3.650 | — |
| borrowing | dictionary | 2.578 | — | — |
| borrowing | map | 3.501 | — | — |
| borrowing | set | 2.843 | — | — |
| borrowing | queue | 1.508 | — | — |
| borrowing | cancel | 9.013 | — | — |
| static_runtime | vector_foreach | 3.433 | — | — |
| static_runtime | serde_json | 10.422 | — | — |
| static_runtime | format_text | 1.238 | — | — |
| static_runtime | statistics_mean | 10.894 | — | — |
| static_runtime | encoding_utf8 | 1.500 | — | — |
| parse_paths | local_valid | 1.767 | — | — |
| parse_paths | local_invalid | 0.861 | — | — |
| parse_paths | full_valid | 0.828 | — | — |
| parse_paths | full_invalid | 2.913 | — | — |
| format_paths | format_literal | 2.107 | — | — |
| format_paths | format_local | 0.671 | — | — |
| format_paths | format_dynamic | 8.213 | — | — |
| serde_paths | serde_json_short | 6.548 | — | — |
| serde_paths | serde_cbor_short | 2.755 | — | — |
| serde_paths | serde_json_long | 5.021 | — | — |
| serde_paths | serde_cbor_long | 5.994 | — | — |
| paths_09_11 | struct_fields | 1.515 | — | — |
| paths_09_11 | class_methods | 1.904 | — | — |
| paths_09_11 | module_call | 0.025 | — | — |
| paths_09_11 | function_value | 2.303 | — | — |
| paths_09_11 | closure_bind | 2.014 | — | — |
| paths_09_11 | heap_push_pop | 7.879 | — | — |
| paths_12_14 | snapshot | 4.024 | — | — |
| paths_12_14 | vector_1k | 0.430 | — | — |
| paths_12_14 | vector_100k | 0.295 | — | — |
| paths_12_14 | division | 0.869 | — | — |
| paths_12_14 | recursion | 0.000 | — | — |
| paths_12_14 | static_spread | 8.117 | — | — |
| paths_12_14 | dynamic_spread | 7.931 | — | — |
| paths_12_14 | deep_copy | 27.297 | — | — |
| paths_15_16 | hex_encode_short | 5.692 | — | — |
| paths_15_16 | hex_decode_short | 8.099 | — | — |
| paths_15_16 | hex_encode_large | 3.915 | — | — |
| paths_15_16 | hex_decode_large | 4.175 | — | — |
| paths_15_16 | utf8_encode_short | 1.300 | — | — |
| paths_15_16 | utf8_decode_short | 2.612 | — | — |
| paths_15_16 | utf8_encode_large | 15.405 | — | — |
| paths_15_16 | utf8_decode_large | 30.892 | — | — |
| paths_15_16 | mean_short | 4.588 | — | — |
| paths_15_16 | mean_large | 22.969 | — | — |
| file_io | file_stream_rw | 2069.715 | — | — |
| format_contract | format_alternating | 8.090 | 3.136 | — |
| format_contract | format_parameter | 7.725 | 2.674 | — |
| concurrency | thread_spawn_join | 11.290 | — | — |
| concurrency | mutex_uncontended | 1.835 | — | — |
| concurrency | atomic_add | 0.443 | — | — |
| concurrency | channel_send_recv | 1.573 | — | — |
| concurrency | task_spawn_wait | 26.869 | — | — |
| sqlite | sqlite_insert | 13.597 | — | — |
| sqlite | sqlite_read | 17.366 | — | — |
| sqlite | sqlite_savepoint | 1.474 | — | — |
| sqlite | sqlite_pool | 6.758 | — | — |
| sqlite | sqlite_async | 9.059 | — | — |
| sqlite | migration_recheck | 1.634 | — | — |
| async_file | async_file_rw | 171.087 | — | — |
| profile | profile_spans | 0.625 | — | — |
| network | dns_localhost | 2.213 | — | — |
| network | udp_echo | 41.369 | — | — |
| network | ipc_echo | 29.826 | — | — |
| network | tls_handshake | 52.926 | — | — |
| random_long | random_long | 22.000 | — | — |
| diagnostics | test_parameterized | 1.124 | — | — |
| diagnostics | test_property | 0.313 | — | — |
| diagnostics | log_filtered | 1.524 | — | — |
| diagnostics | log_file | 1008.452 | — | — |
| graph_contract | graph_copy | 4.396 | 1.364 | — |
| graph_contract | graph_gc | 6.060 | 5.447 | — |
| security | secret_equal | 9.353 | — | — |
| security | argon2_hash_verify | 207.871 | — | — |
| security | ed25519_sign | 16.818 | — | — |
| security | ed25519_verify | 24.251 | — | — |
| security | x509_parse_der | 67.975 | — | — |
| postgres | postgres_insert | 242.776 | — | — |
| postgres | postgres_read | 24.552 | — | — |
| postgres | postgres_savepoint | 340.026 | — | — |
| system | debug_location | 3.410 | — | — |
| system | error_stack | 0.538 | — | — |
| system | file_stream_rw | 1750.237 | — | — |
| system | log_event | 5.083 | — | — |
| system | process_spawn | 204.448 | — | — |
| system | system_os | 0.648 | — | — |
| http | httpx_get | 86.726 | — | — |
| http | requests_get | 73.046 | — | — |
| ws | websocket_echo | 4517.801 | — | — |
| mini-filesystem | 正式数据答案核对 44 组通过 | — | — | — |
| mini-filesystem | fanout-2000 | 5.111 | 1.038 | — |
| mini-filesystem | deep-pwd-2000 | 10.925 | 11.757 | — |
| mini-filesystem | moves-2000 | 2.949 | 0.341 | — |
| mini-filesystem | random-2000-1 | 3.273 | 0.565 | — |
| mini-filesystem | linklong-2000 | 14.248 | 107.232 | — |
| not-yet-on-stage | 正式数据答案核对 45 组通过 | — | — | — |
| not-yet-on-stage | all-free-max | 0.786 | 1.529 | — |
| not-yet-on-stage | forced-increasing-max | 4.296 | 1.366 | — |
| not-yet-on-stage | forced-decreasing-max | 0.828 | 1.554 | — |
| not-yet-on-stage | shuffled-tight-max-1 | 2.838 | 1.697 | — |
| not-yet-on-stage | alternating-tight-max | 2.527 | 1.517 | — |

## 编译、启动与内存

编译为每个 TX 基准单次完整编译墙钟，包含链接及随包共享库安装，不是五轮编译基准。

| 程序 | 完整编译 ms |
| --- | ---: |
| language | 15110.6 |
| library | 14864.7 |
| compute | 15710.3 |
| features | 14487.9 |
| diverse | 14828.1 |
| borrowing | 14373.0 |
| static_runtime | 14804.2 |
| parse_paths | 14094.0 |
| format_paths | 13811.0 |
| serde_paths | 14193.7 |
| paths_09_11 | 14283.1 |
| paths_12_14 | 15307.2 |
| paths_15_16 | 13607.9 |
| file_io | 13069.9 |
| format_contract | 13825.4 |
| concurrency | 13944.2 |
| sqlite | 16091.1 |
| async_file | 13951.8 |
| profile | 13548.7 |
| network | 14732.9 |
| postgres | 15473.0 |
| system | 13370.2 |
| http | 21365.1 |
| ws | 14292.4 |

GNU time 记录独立进程资源；墙钟精度约 10 ms，极短启动仅供参考。RSS 为内核高水位，不与 Windows 工作集直接比较。

| 程序 | 墙钟中位数 ms | 峰值 RSS 中位数 MiB |
| --- | ---: | ---: |
| startup | 180.0 | 11.34 |
| language | 210.0 | 11.52 |
| diverse | 200.0 | 11.81 |

## 迁移修正与证据

首轮原始失败保留在 `results.json`：图契约误用不存在的 TX 源码、random_long 两行格式、证书长度校验和日志跨轮累积。PostgreSQL 因缺少 initdb 中断首轮；补充入口分别修复夹具并重测，结果见 `extra_results.json`。初始失败样本不进入最终表格。

图契约使用原 C++ 基准的 TX runtime/reference 两种模式，只把 Windows 内存采集替换为 Linux getrusage；旧 pagefile 字段在 Linux 不适用。日志每轮删除前次测试文件，并核对 2000 行索引、请求上下文和遮蔽字段。证书校验为 DER 字节数 × 500。

- `manifest.json`、`extra_manifest.json`：环境、源码、工具包与外部输入指纹。
- `extra_primary_end.json`、`extra_completed.json`：首轮结束及补充阶段的指纹核对。
- `results.json`、`extra_results.json`：逐轮原始输出、计时与校验值。
- `extra_resources.json`、`extra_postgres_version.json`：资源样本与隔离数据库版本。
- `summary.json`：合并结果、差距和覆盖映射。

复现入口：WSL Ubuntu 中运行本目录 `run.py`，再用 `uv run --no-project --with websockets --with cryptography python scripts/run_linux_performance_extra.py` 执行补充，最后运行 `scripts/report_linux_performance.py`。入口拒绝覆盖归档；下一轮须使用新的归档目录。

## 模块覆盖

| 模块 | 已采样代表负载 |
| --- | --- |
| algorithm | algorithm_sort |
| array | array |
| async_file | async_file_rw |
| bytes | bytes_hex |
| cancel | cancel_status |
| cbor | cbor_roundtrip |
| channel | channel_send_recv |
| crypto | crypto_sha256 |
| csv | csv_parse |
| db | migration_recheck, postgres_insert, postgres_read, postgres_savepoint, sqlite_async, sqlite_insert, sqlite_pool, sqlite_read, sqlite_savepoint |
| debug | debug_location |
| decimal | decimal_add |
| dictionary | dictionary_contains |
| dns | dns_localhost |
| encoding | encoding_utf8 |
| env | env_get |
| error | error_stack |
| file | file |
| file_stream | file_stream_rw |
| format | format_text |
| fs | fs |
| httpx | httpx_get |
| io | io |
| ipc | ipc_echo |
| json | json_parse |
| log | log_event, log_file, log_filtered |
| math | math_sqrt |
| parse | parse_int |
| password | argon2_hash_verify |
| path | path |
| process | process_spawn |
| profile | profile_spans |
| public_key | ed25519_sign, ed25519_verify |
| random | random_int, random_long |
| regex | regex_search |
| requests | requests_get |
| secret | secret_equal |
| serde | serde_json, serde_short_text |
| socket | udp_echo |
| statistics | statistics_mean |
| string | string |
| sync | atomic_add, mutex_uncontended |
| system | system_os |
| task | sqlite_async, task_spawn_wait |
| test | test_assert, test_parameterized, test_property |
| thread | thread_spawn_join |
| time | time |
| tls | tls_handshake |
| unicode | unicode_nfc |
| websocket | websocket_echo |
| x509 | x509_parse_der |
| xml | xml_parse |
