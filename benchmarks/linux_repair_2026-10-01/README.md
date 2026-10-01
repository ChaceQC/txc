# Linux 热点修复与定向复测

按 L/W → TX/C++ 顺序修改并测量。所有下表来自同机旧/新可执行文件交替采样：一次预热、五次正式采样、逐轮核对校验值。没有重跑全量 181 项，也没有重测 Windows。

## 实际修改

- WebSocket：不超过 16 KiB 的帧将帧头、掩码与正文合并发送，避免短帧拆包触发 Nagle/延迟确认；大帧仍分块。
- X.509：线程内弱引用记住最近成功解析的不可变 bytes 对象；不延长证书生命周期，不缓存信任、时间或吊销验证。
- Linux 非递归目录名称枚举直接使用 readdir，保持排序、符号链接可见性与错误处理。
- JSON/serde：空白判断、标点消费和字符串分支减少重复 peek，位置、限额和流式读取规则不变。
- 流式 JSON：修复已有 ASCII 批量输出超出 4096 字节块上限的问题，在 UTF-8 标量边界切块；原生边界断言保留。
- 标量/文本队列的小型 ABI 操作向 Clang 提供 always_inline 提示，使 ThinLTO 有机会合并循环中的解包及调用；保留检查和异常边界。

## 主要结果

| 位置/阶段 | 负载 | 旧 ms | 新 ms | 耗时变化 | 新 TX/C++ |
| --- | --- | ---: | ---: | ---: | ---: |
| platform | websocket_echo | 4531.291 | 120.881 | -97.3% | — |
| platform_mount | websocket_echo | 4512.118 | 140.577 | -96.9% | — |
| platform | x509_parse_der | 47.233 | 1.071 | -97.7% | — |
| platform_mount | x509_parse_der | 58.678 | 20.217 | -65.5% | — |
| platform | fs | 10.000 | 7.000 | -30.0% | — |
| platform_mount | fs | 743.000 | 752.000 | +1.2% | — |
| serde_final | serde_short_text | 4.201 | 3.464 | -17.5% | 6.75× |
| serde_final | serde_long_text | 5.078 | 4.407 | -13.2% | 1.28× |
| queue | queue_push_pop | 1.276 | 0.877 | -31.3% | 6.31× |

platform/runtime/queue/serde_final 使用 Linux /tmp 本地文件系统；platform_mount 使用 /mnt/e 挂载盘。阶段分别构建，不同阶段的绝对数字不用于声称因果。runtime 之后的队列调整只单独复测 features，JSON/serde 结果取最终 serde_final 阶段。

## 文件系统环境差距

日志的逐条 flush 与即时错误语义没有修改。下面均为同一个旧程序，区别是测试文件/程序所在文件系统。

| 旧程序负载 | /mnt/e ms | Linux 本地 ms |
| --- | ---: | ---: |
| log_file | 811.452 | 10.280 |
| async_file_rw | 159.745 | 6.400 |
| fs | 743.000 | 10.000 |
| file | 2609.000 | 63.000 |

因此本次不把 /mnt/e 的日志、文件和目录高倍率标为已消除。真实 Linux 部署应在本地文件系统重测，不能通过取消 flush 或隐藏 I/O 错误换取分数。

## 验证与剩余问题

已通过 JSON 行为/流式原生边界、serde TX/原生直接路径、队列调用效果和容器行为、目录名称/中文/悬空链接/错误路径、X.509 原有安全验证以及 Linux WebSocket 分片/控制帧验证。完整输出见 checks.json。
队列机器码直接调用数：`{"old": 3, "new": 0}`。普通库与 ThinLTO 库均已构建，验证同时使用二者。
任务提交等待的 L/W 差距尚未解决；本轮旧/新约同一水平。对象析构、深拷贝、循环 GC、小向量扫描仍存在 TX/C++ 差距，本次没有修改其算法或生成代码，不能把采样波动认作修复。X.509 加速适用于同一不可变对象的重复解析，不代表首次或大量不同证书解析的同等收益。
Windows 工具包本次未重建，公共 C++ 文件的 Windows 原生验证尚未执行。Linux 交付位于 tx/linux，源码和二进制指纹见 delivery.json。

## 全部定向样本的中位数

| 阶段 | 套件 | 项目 | 旧 ms | 新 ms | C++ ms |
| --- | --- | --- | ---: | ---: | ---: |
| platform | library | math | 6.000 | 6.000 | — |
| platform | library | random | 1.000 | 1.000 | — |
| platform | library | string | 3.000 | 4.000 | — |
| platform | library | array | 13.000 | 14.000 | — |
| platform | library | dict | 1.000 | 1.000 | — |
| platform | library | path | 7.000 | 7.000 | — |
| platform | library | fs | 10.000 | 7.000 | — |
| platform | library | file | 63.000 | 62.000 | — |
| platform | library | io | 4.000 | 3.000 | — |
| platform | library | time | 3.000 | 3.000 | — |
| platform | security | secret_equal | 7.383 | 7.445 | — |
| platform | security | argon2_hash_verify | 192.045 | 151.009 | — |
| platform | security | ed25519_sign | 17.913 | 17.083 | — |
| platform | security | ed25519_verify | 25.074 | 25.165 | — |
| platform | security | x509_parse_der | 47.233 | 1.071 | — |
| platform | diagnostics | test_parameterized | 0.182 | 0.181 | — |
| platform | diagnostics | test_property | 0.288 | 0.290 | — |
| platform | diagnostics | log_filtered | 1.010 | 0.979 | — |
| platform | diagnostics | log_file | 10.280 | 10.369 | — |
| platform | async_file | async_file_rw | 6.400 | 6.406 | — |
| platform | concurrency | thread_spawn_join | 9.409 | 9.289 | — |
| platform | concurrency | mutex_uncontended | 1.721 | 1.840 | — |
| platform | concurrency | atomic_add | 0.425 | 0.427 | — |
| platform | concurrency | channel_send_recv | 1.526 | 1.527 | — |
| platform | concurrency | task_spawn_wait | 23.210 | 23.279 | — |
| platform | ws | websocket_echo | 4531.291 | 120.881 | — |
| platform_mount | library | math | 6.000 | 6.000 | — |
| platform_mount | library | random | 1.000 | 1.000 | — |
| platform_mount | library | string | 4.000 | 4.000 | — |
| platform_mount | library | array | 13.000 | 12.000 | — |
| platform_mount | library | dict | 1.000 | 1.000 | — |
| platform_mount | library | path | 8.000 | 8.000 | — |
| platform_mount | library | fs | 743.000 | 752.000 | — |
| platform_mount | library | file | 2609.000 | 2568.000 | — |
| platform_mount | library | io | 3.000 | 3.000 | — |
| platform_mount | library | time | 2.000 | 2.000 | — |
| platform_mount | security | secret_equal | 7.290 | 7.282 | — |
| platform_mount | security | argon2_hash_verify | 162.506 | 153.211 | — |
| platform_mount | security | ed25519_sign | 16.263 | 16.255 | — |
| platform_mount | security | ed25519_verify | 22.925 | 22.640 | — |
| platform_mount | security | x509_parse_der | 58.678 | 20.217 | — |
| platform_mount | diagnostics | test_parameterized | 0.899 | 0.765 | — |
| platform_mount | diagnostics | test_property | 0.274 | 0.270 | — |
| platform_mount | diagnostics | log_filtered | 0.950 | 0.940 | — |
| platform_mount | diagnostics | log_file | 811.452 | 795.058 | — |
| platform_mount | async_file | async_file_rw | 159.745 | 168.815 | — |
| platform_mount | concurrency | thread_spawn_join | 9.486 | 8.759 | — |
| platform_mount | concurrency | mutex_uncontended | 1.645 | 1.677 | — |
| platform_mount | concurrency | atomic_add | 0.425 | 0.401 | — |
| platform_mount | concurrency | channel_send_recv | 1.445 | 1.435 | — |
| platform_mount | concurrency | task_spawn_wait | 24.118 | 23.413 | — |
| platform_mount | ws | websocket_echo | 4512.118 | 140.577 | — |
| runtime | diverse | vector_scan_1k | 0.278 | 0.282 | 0.131 |
| runtime | diverse | vector_scan_100k | 2.656 | 2.666 | 1.310 |
| runtime | diverse | vector_index_sequential | 0.323 | 0.323 | 0.342 |
| runtime | diverse | vector_index_strided | 0.341 | 0.398 | 0.336 |
| runtime | diverse | map_hit_128 | 2.152 | 1.843 | 1.031 |
| runtime | diverse | map_hit_8192 | 2.503 | 2.365 | 0.978 |
| runtime | diverse | map_hit_10_percent | 2.079 | 2.105 | 1.752 |
| runtime | diverse | dictionary_int_hit | 0.980 | 0.967 | 0.825 |
| runtime | diverse | dictionary_text_hit | 2.600 | 2.595 | 5.200 |
| runtime | diverse | dictionary_mostly_miss | 0.585 | 0.590 | 1.114 |
| runtime | diverse | format_literal | 0.672 | 0.606 | 0.965 |
| runtime | diverse | format_dynamic | 0.690 | 0.606 | 1.082 |
| runtime | diverse | encoding_literal | 0.793 | 0.877 | 2.257 |
| runtime | diverse | encoding_dynamic | 0.828 | 0.847 | 2.231 |
| runtime | diverse | serde_short_text | 4.471 | 3.300 | 0.546 |
| runtime | diverse | serde_long_text | 5.414 | 4.503 | 3.745 |
| runtime | diverse | parse_valid | 0.718 | 0.697 | 1.924 |
| runtime | diverse | parse_invalid | 0.699 | 0.699 | 3.971 |
| runtime | features | vector_push | 1.584 | 1.677 | 1.059 |
| runtime | features | vector_index | 0.148 | 0.149 | 0.119 |
| runtime | features | map_lookup | 0.356 | 0.370 | 0.197 |
| runtime | features | set_contains | 0.277 | 0.279 | 0.171 |
| runtime | features | heap_push_pop | 7.588 | 7.187 | 3.116 |
| runtime | features | queue_push_pop | 1.165 | 1.337 | 0.136 |
| runtime | features | iterator_snapshot | 0.100 | 0.098 | 0.020 |
| runtime | features | option_value | 0.024 | 0.024 | 0.013 |
| runtime | features | function_value | 0.216 | 0.216 | 0.142 |
| runtime | features | closure_bind | 0.192 | 0.192 | 0.142 |
| runtime | language | scalar_control | 0.171 | 0.172 | 0.339 |
| runtime | language | updates | 1.456 | 1.482 | 1.356 |
| runtime | language | while_logic | 0.225 | 0.192 | 0.267 |
| runtime | language | float_arithmetic | 0.368 | 0.365 | 0.365 |
| runtime | language | overloads | 0.204 | 0.193 | 0.194 |
| runtime | language | named_arguments | 0.432 | 0.395 | 0.389 |
| runtime | language | recursion | 0.043 | 0.043 | 0.072 |
| runtime | language | variadic_unpack | 5.217 | 3.992 | 0.636 |
| runtime | language | array_destructure | 0.010 | 0.010 | 0.857 |
| runtime | language | array_padded | 0.024 | 0.025 | 1.495 |
| runtime | language | dict_iteration | 4.926 | 5.017 | 5.291 |
| runtime | language | struct_operators | 0.026 | 0.025 | 6.331 |
| runtime | language | class_methods | 0.736 | 0.758 | 0.166 |
| runtime | language | virtual_interface | 1.186 | 0.983 | 0.507 |
| runtime | language | class_operator | 0.685 | 0.652 | 0.255 |
| runtime | language | runtime_cast | 0.270 | 0.291 | 2.491 |
| runtime | language | module_call | 0.024 | 0.024 | 1.564 |
| runtime | language | string_conversion | 4.152 | 4.200 | 1.099 |
| runtime | language | deep_copy | 11.046 | 11.252 | 1.257 |
| runtime | language | copy_cycle | 3.846 | 3.245 | 1.075 |
| runtime | language | deinit | 4.996 | 3.700 | 0.389 |
| runtime | language | cycle_gc | 0.790 | 0.738 | 0.057 |
| queue | features | vector_push | 1.612 | 1.663 | 1.175 |
| queue | features | vector_index | 0.158 | 0.157 | 0.127 |
| queue | features | map_lookup | 0.387 | 0.377 | 0.216 |
| queue | features | set_contains | 0.300 | 0.297 | 0.181 |
| queue | features | heap_push_pop | 7.057 | 7.389 | 3.452 |
| queue | features | queue_push_pop | 1.276 | 0.877 | 0.139 |
| queue | features | iterator_snapshot | 0.105 | 0.105 | 0.021 |
| queue | features | option_value | 0.026 | 0.026 | 0.014 |
| queue | features | function_value | 0.239 | 0.229 | 0.150 |
| queue | features | closure_bind | 0.202 | 0.204 | 0.149 |
| serde_final | diverse | vector_scan_1k | 0.278 | 0.408 | 0.129 |
| serde_final | diverse | vector_scan_100k | 2.621 | 3.896 | 1.290 |
| serde_final | diverse | vector_index_sequential | 0.319 | 0.321 | 0.335 |
| serde_final | diverse | vector_index_strided | 0.318 | 0.318 | 0.331 |
| serde_final | diverse | map_hit_128 | 2.076 | 1.928 | 1.010 |
| serde_final | diverse | map_hit_8192 | 2.357 | 2.265 | 0.937 |
| serde_final | diverse | map_hit_10_percent | 2.035 | 2.015 | 1.660 |
| serde_final | diverse | dictionary_int_hit | 0.974 | 0.971 | 0.830 |
| serde_final | diverse | dictionary_text_hit | 2.465 | 2.528 | 5.124 |
| serde_final | diverse | dictionary_mostly_miss | 0.575 | 0.579 | 0.929 |
| serde_final | diverse | format_literal | 0.649 | 0.573 | 1.094 |
| serde_final | diverse | format_dynamic | 0.640 | 0.579 | 1.023 |
| serde_final | diverse | encoding_literal | 0.801 | 0.755 | 1.973 |
| serde_final | diverse | encoding_dynamic | 0.792 | 0.822 | 2.124 |
| serde_final | diverse | serde_short_text | 4.201 | 3.464 | 0.513 |
| serde_final | diverse | serde_long_text | 5.078 | 4.407 | 3.445 |
| serde_final | diverse | parse_valid | 0.681 | 0.646 | 1.860 |
| serde_final | diverse | parse_invalid | 0.669 | 0.644 | 3.689 |
