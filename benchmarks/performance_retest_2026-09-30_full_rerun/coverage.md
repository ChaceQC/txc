# 52 个公开模块的本轮性能覆盖映射

原有性能负载覆盖 37 个模块，本轮新增 15 个遗漏模块，并扩展 test/log。下表逐模块列出实际运行的负载；有导入但没有独立工作量的辅助模块不单独计入新增覆盖。代表负载覆盖不等于所有 API 全覆盖。

| 模块 | 原有本轮负载 | 新增四语言负载 |
| --- | --- | --- |
| algorithm | algorithm_sort | — |
| array | library/array | — |
| async_file | — | async_file_rw |
| bytes | bytes_hex | — |
| cancel | cancel_status | — |
| cbor | cbor_roundtrip | — |
| channel | — | channel_send_recv |
| crypto | crypto_sha256 | — |
| csv | csv_parse | — |
| db | — | sqlite_insert, sqlite_read, sqlite_savepoint, sqlite_pool, sqlite_async, migration_recheck, postgres_insert, postgres_read, postgres_savepoint |
| debug | debug_location | — |
| decimal | decimal_add | — |
| dictionary | dictionary_contains | — |
| dns | — | dns_localhost |
| encoding | encoding_utf8 | — |
| env | env_get | — |
| error | error_stack | — |
| file | library/file | — |
| file_stream | file_stream_rw | — |
| format | format_text / format_contract | — |
| fs | library/fs | — |
| httpx | httpx_get | — |
| io | library/io | — |
| ipc | — | ipc_echo |
| json | json_parse | — |
| log | log_event | log_filtered, log_file |
| math | math_sqrt | — |
| parse | parse_int | — |
| password | — | argon2_hash_verify |
| path | library/path | — |
| process | process_spawn | — |
| profile | — | profile_spans |
| public_key | — | ed25519_sign, ed25519_verify |
| random | random_int / random_long | — |
| regex | regex_search | — |
| requests | requests_get | — |
| secret | — | secret_equal |
| serde | serde_json / serde_short_text | — |
| socket | — | udp_echo |
| statistics | statistics_mean（校准） | — |
| string | library/string | — |
| sync | — | mutex_uncontended, atomic_add |
| system | system_os | — |
| task | — | task_spawn_wait, sqlite_async |
| test | test_assert（不进入差距排名） | test_parameterized, test_property |
| thread | — | thread_spawn_join |
| time | library/time | — |
| tls | — | tls_handshake |
| unicode | unicode_nfc | — |
| websocket | websocket_echo | — |
| x509 | — | x509_parse_der |
| xml | xml_parse | — |

