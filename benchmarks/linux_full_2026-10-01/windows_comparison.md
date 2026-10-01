# Linux 与 Windows TX 性能归档对照

本报告只读取已有结果，没有重新运行性能测试。比较的是两轮实际测量，不是同一提交、同一工具链的操作系统 A/B 实验。

- Windows：2026-09-30，提交 `f1cc9a5368d01d8a9b413facf2495af8f63e5c57`，Windows 11 26200；Clang 23.1.2。
- Linux：2026-10-01，工作树 `9a8ba2e789a57a2801a04d4ef8ebae3aef5f652c` 的现有 Linux 包；WSL2 Ubuntu 24.04，Clang 18.1.3，程序位于 `/mnt/e`。
- Windows 原有主套件多为 7 轮，新增标准库/契约为 5 轮；Linux 为 5 轮。均取中位数。Linux 基础实现已经过平台适配，差异不能单独归因于操作系统。
- 对齐 180 个 TX 条目；以 ±10% 作描述性区间，Linux 耗时降低超过 10% 的有 99 项、增加超过 10% 的有 53 项。不同专项重复出现相似负载，这不是独立统计样本或显著性检验。
- 下表 L/W 小于 1 表示 Linux 更快，大于 1 表示 Linux 更慢。分组中位倍率是逐项倍率的中位数，不是总耗时比或语言综合得分。

## 分组对比

| 组 | 条目数 | L/W 中位倍率 | Linux 快 >10% | Linux 慢 >10% |
| --- | ---: | ---: | ---: | ---: |
| language | 22 | 1.04× | 7 | 6 |
| library | 10 | 0.74× | 6 | 3 |
| compute | 21 | 0.59× | 15 | 3 |
| features | 10 | 1.11× | 3 | 5 |
| diverse | 18 | 0.82× | 12 | 6 |
| borrowing | 5 | 1.20× | 1 | 4 |
| static_runtime | 5 | 0.75× | 3 | 1 |
| parse_paths | 4 | 1.05× | 1 | 2 |
| format_paths | 3 | 0.29× | 3 | 0 |
| serde_paths | 4 | 0.66× | 3 | 0 |
| paths_09_11 | 6 | 1.10× | 2 | 3 |
| paths_12_14 | 8 | 0.41× | 5 | 2 |
| paths_15_16 | 10 | 0.74× | 7 | 0 |
| file_io | 1 | 2.33× | 0 | 1 |
| format_contract | 2 | 0.45× | 2 | 0 |
| concurrency | 5 | 0.81× | 3 | 2 |
| sqlite | 6 | 0.61× | 5 | 1 |
| async_file | 1 | 4.50× | 0 | 1 |
| profile | 1 | 1.33× | 0 | 1 |
| network | 4 | 1.77× | 1 | 3 |
| random_long | 1 | 0.54× | 1 | 0 |
| diagnostics | 4 | 3.37× | 0 | 3 |
| graph_contract | 2 | 0.59× | 2 | 0 |
| security | 5 | 1.00× | 1 | 2 |
| postgres | 3 | 1.28× | 1 | 2 |
| system | 6 | 0.45× | 5 | 1 |
| http | 2 | 0.08× | 2 | 0 |
| ws | 1 | 5.05× | 0 | 1 |
| mini-filesystem | 5 | 0.51× | 5 | 0 |
| not-yet-on-stage | 5 | 0.87× | 3 | 0 |

## 启动、进程墙钟和内存

进程墙钟包含启动及输出，不能和内部计时混用。Linux GNU time 峰值 RSS 与 Windows 2 ms 采样的进程树工作集口径不同，内存值只并列展示。

| 指标 | Windows | Linux |
| --- | ---: | ---: |
| 空程序启动 ms | 46.548 | 180.000 |
| 空程序启动 MiB | 6.168 | 11.340 |
| 进程运行 ms | 122.547 | 200.000 |
| 进程运行 MiB | 9.062 | 11.812 |

Linux 完整编译仅记录每个程序一次，含链接和复制运行库；Windows 编译基准的构建阶段/重复次数不同，本报告不制造编译速度倍率。

## 全部同名负载

| 组 | 负载 | Windows ms | Linux ms | L/W | 变化 | 说明 |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| language | scalar_control | 0.168 | 1.339 | 7.97× | +697.0% |  |
| language | updates | 1.417 | 1.534 | 1.08× | +8.3% |  |
| language | while_logic | 0.177 | 0.187 | 1.06× | +5.6% |  |
| language | float_arithmetic | 0.355 | 0.389 | 1.10× | +9.6% |  |
| language | overloads | 0.188 | 0.199 | 1.06× | +5.9% |  |
| language | named_arguments | 0.404 | 0.407 | 1.01× | +0.7% |  |
| language | recursion | 0.035 | 0.043 | 1.23× | +22.9% |  |
| language | variadic_unpack | 11.229 | 4.068 | 0.36× | -63.8% |  |
| language | array_destructure | 0.010 | 0.010 | 1.00× | +0.0% |  |
| language | array_padded | 0.024 | 0.025 | 1.04× | +4.2% |  |
| language | dict_iteration | 7.313 | 5.051 | 0.69× | -30.9% |  |
| language | struct_operators | 0.025 | 0.026 | 1.04× | +4.0% |  |
| language | class_methods | 0.662 | 0.739 | 1.12× | +11.6% |  |
| language | virtual_interface | 0.904 | 1.368 | 1.51× | +51.3% |  |
| language | class_operator | 0.598 | 0.673 | 1.13× | +12.5% |  |
| language | runtime_cast | 0.236 | 0.276 | 1.17× | +16.9% |  |
| language | module_call | 0.024 | 0.025 | 1.04× | +4.2% |  |
| language | string_conversion | 16.576 | 4.257 | 0.26× | -74.3% |  |
| language | deep_copy | 25.082 | 10.594 | 0.42× | -57.8% |  |
| language | copy_cycle | 9.302 | 3.356 | 0.36× | -63.9% |  |
| language | deinit | 15.378 | 3.872 | 0.25× | -74.8% |  |
| language | cycle_gc | 1.542 | 0.711 | 0.46× | -53.9% |  |
| library | math | 4.000 | 7.000 | 1.75× | +75.0% | TX 使用整数毫秒；短项比例精度有限 |
| library | random | 1.000 | 1.000 | 1.00× | +0.0% | TX 使用整数毫秒；短项比例精度有限 |
| library | string | 9.000 | 4.000 | 0.44× | -55.6% | TX 使用整数毫秒；短项比例精度有限 |
| library | array | 18.000 | 13.000 | 0.72× | -27.8% | TX 使用整数毫秒；短项比例精度有限 |
| library | path | 21.000 | 9.000 | 0.43× | -57.1% | TX 使用整数毫秒；短项比例精度有限 |
| library | fs | 43.000 | 850.000 | 19.77× | +1876.7% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| library | file | 1317.000 | 2464.000 | 1.87× | +87.1% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| library | io | 9.000 | 4.000 | 0.44× | -55.6% | TX 使用整数毫秒；短项比例精度有限 |
| library | time | 4.000 | 3.000 | 0.75× | -25.0% | TX 使用整数毫秒；短项比例精度有限 |
| library | dict | 3.000 | 1.000 | 0.33× | -66.7% | TX 使用整数毫秒；短项比例精度有限 |
| compute | algorithm_sort | 5.469 | 5.372 | 0.98× | -1.8% |  |
| compute | bytes_hex | 8.617 | 3.012 | 0.35× | -65.0% |  |
| compute | cancel_status | 0.573 | 0.904 | 1.58× | +57.8% |  |
| compute | cbor_roundtrip | 22.235 | 10.888 | 0.49× | -51.0% |  |
| compute | crypto_sha256 | 8.852 | 6.205 | 0.70× | -29.9% |  |
| compute | csv_parse | 24.532 | 10.347 | 0.42× | -57.8% |  |
| compute | decimal_add | 10.978 | 3.613 | 0.33× | -67.1% |  |
| compute | dictionary_contains | 0.362 | 0.270 | 0.75× | -25.4% |  |
| compute | encoding_utf8 | 4.330 | 0.256 | 0.06× | -94.1% |  |
| compute | env_get | 9.844 | 2.716 | 0.28× | -72.4% |  |
| compute | format_text | 1.231 | 0.110 | 0.09× | -91.1% |  |
| compute | json_parse | 23.194 | 12.745 | 0.55× | -45.1% |  |
| compute | math_sqrt | 4.153 | 5.286 | 1.27× | +27.3% |  |
| compute | parse_int | 11.408 | 3.621 | 0.32× | -68.3% |  |
| compute | random_int | 1.390 | 0.818 | 0.59× | -41.2% |  |
| compute | regex_search | 12.651 | 8.082 | 0.64× | -36.1% |  |
| compute | serde_json | 13.898 | 8.790 | 0.63× | -36.8% |  |
| compute | statistics_mean | 4.457 | 4.423 | 0.99× | -0.8% |  |
| compute | test_assert | 7.654 | 2.012 | 0.26× | -73.7% |  |
| compute | unicode_nfc | 5.347 | 7.426 | 1.39× | +38.9% |  |
| compute | xml_parse | 32.989 | 30.707 | 0.93× | -6.9% |  |
| features | vector_push | 1.938 | 2.884 | 1.49× | +48.8% |  |
| features | vector_index | 0.146 | 0.232 | 1.59× | +58.9% |  |
| features | map_lookup | 0.325 | 0.513 | 1.58× | +57.8% |  |
| features | set_contains | 0.259 | 0.347 | 1.34× | +34.0% |  |
| features | heap_push_pop | 29.891 | 7.670 | 0.26× | -74.3% |  |
| features | queue_push_pop | 1.119 | 1.286 | 1.15× | +14.9% |  |
| features | iterator_snapshot | 1.468 | 0.105 | 0.07× | -92.8% |  |
| features | option_value | 0.025 | 0.026 | 1.04× | +4.0% |  |
| features | function_value | 0.286 | 0.231 | 0.81× | -19.2% |  |
| features | closure_bind | 0.190 | 0.204 | 1.07× | +7.4% |  |
| diverse | vector_scan_1k | 0.262 | 1.299 | 4.96× | +395.8% |  |
| diverse | vector_scan_100k | 2.479 | 2.879 | 1.16× | +16.1% |  |
| diverse | vector_index_sequential | 0.301 | 0.469 | 1.56× | +55.8% |  |
| diverse | vector_index_strided | 0.298 | 0.428 | 1.44× | +43.6% |  |
| diverse | map_hit_128 | 1.606 | 2.136 | 1.33× | +33.0% |  |
| diverse | map_hit_8192 | 2.215 | 2.532 | 1.14× | +14.3% |  |
| diverse | map_hit_10_percent | 2.522 | 2.133 | 0.85× | -15.4% |  |
| diverse | dictionary_int_hit | 2.364 | 0.991 | 0.42× | -58.1% |  |
| diverse | dictionary_text_hit | 3.157 | 2.614 | 0.83× | -17.2% |  |
| diverse | dictionary_mostly_miss | 0.845 | 0.604 | 0.71× | -28.5% |  |
| diverse | format_literal | 6.574 | 0.685 | 0.10× | -89.6% |  |
| diverse | format_dynamic | 6.923 | 0.649 | 0.09× | -90.6% |  |
| diverse | encoding_literal | 12.491 | 0.798 | 0.06× | -93.6% |  |
| diverse | encoding_dynamic | 11.221 | 0.844 | 0.08× | -92.5% |  |
| diverse | serde_short_text | 9.825 | 5.924 | 0.60× | -39.7% |  |
| diverse | serde_long_text | 8.712 | 5.141 | 0.59× | -41.0% |  |
| diverse | parse_valid | 0.830 | 0.668 | 0.80× | -19.5% |  |
| diverse | parse_invalid | 0.745 | 0.669 | 0.90× | -10.2% |  |
| borrowing | dictionary | 3.298 | 2.578 | 0.78× | -21.8% |  |
| borrowing | map | 2.878 | 3.501 | 1.22× | +21.6% |  |
| borrowing | set | 2.550 | 2.843 | 1.11× | +11.5% |  |
| borrowing | queue | 1.258 | 1.508 | 1.20× | +19.9% |  |
| borrowing | cancel | 5.504 | 9.013 | 1.64× | +63.8% |  |
| static_runtime | vector_foreach | 1.263 | 3.433 | 2.72× | +171.8% |  |
| static_runtime | serde_json | 13.982 | 10.422 | 0.75× | -25.5% |  |
| static_runtime | format_text | 13.734 | 1.238 | 0.09× | -91.0% |  |
| static_runtime | statistics_mean | 10.430 | 10.894 | 1.04× | +4.4% |  |
| static_runtime | encoding_utf8 | 22.987 | 1.500 | 0.07× | -93.5% |  |
| parse_paths | local_valid | 0.766 | 1.767 | 2.31× | +130.7% |  |
| parse_paths | local_invalid | 0.750 | 0.861 | 1.15× | +14.8% |  |
| parse_paths | full_valid | 0.878 | 0.828 | 0.94× | -5.7% |  |
| parse_paths | full_invalid | 3.241 | 2.913 | 0.90× | -10.1% |  |
| format_paths | format_literal | 7.315 | 2.107 | 0.29× | -71.2% |  |
| format_paths | format_local | 7.316 | 0.671 | 0.09× | -90.8% |  |
| format_paths | format_dynamic | 19.273 | 8.213 | 0.43× | -57.4% |  |
| serde_paths | serde_json_short | 7.210 | 6.548 | 0.91× | -9.2% |  |
| serde_paths | serde_cbor_short | 4.732 | 2.755 | 0.58× | -41.8% |  |
| serde_paths | serde_json_long | 8.425 | 5.021 | 0.60× | -40.4% |  |
| serde_paths | serde_cbor_long | 8.341 | 5.994 | 0.72× | -28.1% |  |
| paths_09_11 | struct_fields | 0.662 | 1.515 | 2.29× | +128.9% |  |
| paths_09_11 | class_methods | 1.630 | 1.904 | 1.17× | +16.8% |  |
| paths_09_11 | module_call | 0.024 | 0.025 | 1.04× | +4.2% |  |
| paths_09_11 | function_value | 2.810 | 2.303 | 0.82× | -18.0% |  |
| paths_09_11 | closure_bind | 1.645 | 2.014 | 1.22× | +22.4% |  |
| paths_09_11 | heap_push_pop | 29.949 | 7.879 | 0.26× | -73.7% |  |
| paths_12_14 | snapshot | 14.787 | 4.024 | 0.27× | -72.8% |  |
| paths_12_14 | vector_1k | 0.240 | 0.430 | 1.79× | +79.2% |  |
| paths_12_14 | vector_100k | 0.297 | 0.295 | 0.99× | -0.7% |  |
| paths_12_14 | division | 0.398 | 0.869 | 2.18× | +118.3% |  |
| paths_12_14 | recursion | 0.001 | 0.000 | 0.00× | -100.0% |  |
| paths_12_14 | static_spread | 19.563 | 8.117 | 0.41× | -58.5% |  |
| paths_12_14 | dynamic_spread | 19.422 | 7.931 | 0.41× | -59.2% |  |
| paths_12_14 | deep_copy | 78.753 | 27.297 | 0.35× | -65.3% |  |
| paths_15_16 | hex_encode_short | 18.549 | 5.692 | 0.31× | -69.3% |  |
| paths_15_16 | hex_decode_short | 31.879 | 8.099 | 0.25× | -74.6% |  |
| paths_15_16 | hex_encode_large | 4.566 | 3.915 | 0.86× | -14.3% |  |
| paths_15_16 | hex_decode_large | 6.752 | 4.175 | 0.62× | -38.2% |  |
| paths_15_16 | utf8_encode_short | 24.570 | 1.300 | 0.05× | -94.7% |  |
| paths_15_16 | utf8_decode_short | 14.067 | 2.612 | 0.19× | -81.4% |  |
| paths_15_16 | utf8_encode_large | 16.209 | 15.405 | 0.95× | -5.0% |  |
| paths_15_16 | utf8_decode_large | 32.357 | 30.892 | 0.95× | -4.5% |  |
| paths_15_16 | mean_short | 5.125 | 4.588 | 0.90× | -10.5% |  |
| paths_15_16 | mean_large | 25.333 | 22.969 | 0.91× | -9.3% |  |
| file_io | file_stream_rw | 888.247 | 2069.715 | 2.33× | +133.0% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| format_contract | format_alternating | 17.629 | 8.090 | 0.46× | -54.1% |  |
| format_contract | format_parameter | 17.913 | 7.725 | 0.43× | -56.9% |  |
| concurrency | thread_spawn_join | 13.854 | 11.290 | 0.81× | -18.5% |  |
| concurrency | mutex_uncontended | 20.517 | 1.835 | 0.09× | -91.1% |  |
| concurrency | atomic_add | 0.384 | 0.443 | 1.15× | +15.4% |  |
| concurrency | channel_send_recv | 1.978 | 1.573 | 0.80× | -20.5% |  |
| concurrency | task_spawn_wait | 7.506 | 26.869 | 3.58× | +258.0% |  |
| sqlite | sqlite_insert | 11.422 | 13.597 | 1.19× | +19.0% |  |
| sqlite | sqlite_read | 26.133 | 17.366 | 0.66× | -33.5% |  |
| sqlite | sqlite_savepoint | 2.622 | 1.474 | 0.56× | -43.8% |  |
| sqlite | sqlite_pool | 137.792 | 6.758 | 0.05× | -95.1% |  |
| sqlite | sqlite_async | 54.773 | 9.059 | 0.17× | -83.5% |  |
| sqlite | migration_recheck | 2.117 | 1.634 | 0.77× | -22.8% |  |
| async_file | async_file_rw | 38.012 | 171.087 | 4.50× | +350.1% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| profile | profile_spans | 0.471 | 0.625 | 1.33× | +32.7% |  |
| network | dns_localhost | 0.266 | 2.213 | 8.32× | +732.0% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| network | udp_echo | 34.354 | 41.369 | 1.20× | +20.4% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| network | ipc_echo | 12.771 | 29.826 | 2.34× | +133.5% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| network | tls_handshake | 144.254 | 52.926 | 0.37× | -63.3% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| random_long | random_long | 41.000 | 22.000 | 0.54× | -46.3% |  |
| diagnostics | test_parameterized | 0.223 | 1.124 | 5.04× | +404.0% |  |
| diagnostics | test_property | 0.334 | 0.313 | 0.94× | -6.3% |  |
| diagnostics | log_filtered | 0.901 | 1.524 | 1.69× | +69.1% |  |
| diagnostics | log_file | 31.396 | 1008.452 | 32.12× | +3112.0% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| graph_contract | graph_copy | 7.334 | 4.396 | 0.60× | -40.1% | 直接 TX runtime 契约；仅内存采集移植为 getrusage |
| graph_contract | graph_gc | 10.423 | 6.060 | 0.58× | -41.9% | 直接 TX runtime 契约；仅内存采集移植为 getrusage |
| security | secret_equal | 14.163 | 9.353 | 0.66× | -34.0% |  |
| security | argon2_hash_verify | 163.263 | 207.871 | 1.27× | +27.3% |  |
| security | ed25519_sign | 17.126 | 16.818 | 0.98× | -1.8% |  |
| security | ed25519_verify | 24.329 | 24.251 | 1.00× | -0.3% |  |
| security | x509_parse_der | 2.965 | 67.975 | 22.93× | +2192.6% | 独立生成的证书夹具；比较相同次数，字节长度可能不同 |
| postgres | postgres_insert | 189.458 | 242.776 | 1.28× | +28.1% | 服务器 Windows 18.4 / Linux 16.15；临时 TLS 数据库 |
| postgres | postgres_read | 41.141 | 24.552 | 0.60× | -40.3% | 服务器 Windows 18.4 / Linux 16.15；临时 TLS 数据库 |
| postgres | postgres_savepoint | 115.403 | 340.026 | 2.95× | +194.6% | 服务器 Windows 18.4 / Linux 16.15；临时 TLS 数据库 |
| system | debug_location | 5.391 | 3.410 | 0.63× | -36.7% | 进程路径已移植；debug_location/system_os 的输出长度随平台改变 |
| system | error_stack | 1.953 | 0.538 | 0.28× | -72.5% | 进程路径已移植；debug_location/system_os 的输出长度随平台改变 |
| system | file_stream_rw | 843.336 | 1750.237 | 2.08× | +107.5% | 进程路径已移植；debug_location/system_os 的输出长度随平台改变 |
| system | log_event | 7.738 | 5.083 | 0.66× | -34.3% | 进程路径已移植；debug_location/system_os 的输出长度随平台改变 |
| system | process_spawn | 907.729 | 204.448 | 0.23× | -77.5% | 进程路径已移植；debug_location/system_os 的输出长度随平台改变 |
| system | system_os | 2.363 | 0.648 | 0.27× | -72.6% | 进程路径已移植；debug_location/system_os 的输出长度随平台改变 |
| http | httpx_get | 975.974 | 86.726 | 0.09× | -91.1% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| http | requests_get | 981.354 | 73.046 | 0.07× | -92.6% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| ws | websocket_echo | 894.903 | 4517.801 | 5.05× | +404.8% | 系统/网络/文件负载；Linux 为 WSL2 /mnt/e，平台后端也不同 |
| mini-filesystem | fanout-2000 | 11.182 | 5.111 | 0.46× | -54.3% |  |
| mini-filesystem | deep-pwd-2000 | 12.568 | 10.925 | 0.87× | -13.1% |  |
| mini-filesystem | moves-2000 | 5.442 | 2.949 | 0.54× | -45.8% |  |
| mini-filesystem | random-2000-1 | 6.399 | 3.273 | 0.51× | -48.9% |  |
| mini-filesystem | linklong-2000 | 32.177 | 14.248 | 0.44× | -55.7% |  |
| not-yet-on-stage | all-free-max | 0.993 | 0.786 | 0.79× | -20.8% |  |
| not-yet-on-stage | forced-increasing-max | 4.299 | 4.296 | 1.00× | -0.1% |  |
| not-yet-on-stage | forced-decreasing-max | 1.010 | 0.828 | 0.82× | -18.0% |  |
| not-yet-on-stage | shuffled-tight-max-1 | 3.280 | 2.838 | 0.87× | -13.5% |  |
| not-yet-on-stage | alternating-tight-max | 2.701 | 2.527 | 0.94× | -6.4% |  |

## 解读边界

计算、容器和对象操作的差异同时受编译器版本、生成代码及 CPU/调度状态影响。文件、日志、异步 I/O 和启动受到 /mnt/e 跨文件系统及共享库加载影响，不能由这轮数据认定原生 Linux 文件性能更差。PostgreSQL 主版本不同；网络是端到端回环负载，包含服务端线程调度。
原始来源：Windows `performance_retest_2026-09-30_static_execution`、`stdlib_retest_2026-09-30_static_execution`；Linux `final_results.json`。机器可读结果及逐项源码哈希一致性字段见 `windows_comparison.json`；哈希一致仅说明负载源码相同，不说明编译器实现相同。
