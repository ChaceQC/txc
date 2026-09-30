# 新增标准库四语言全量补测

版本 `bc483e587ba3dea1fb2f566339d227fd216ca59f`。正式采样北京时间 2026-09-30 07:48:01 至 07:49:48；日志参考修正后的 diagnostics 定向校准完成于 07:52:08。

本次新增 29 个代表负载，覆盖原套件遗漏的 15 个模块及 test/log 扩展。每个负载都有 TX、C++、Java、Python 四方实现，预热 1 轮、正式 5 轮、轮换语言顺序；每次使用新进程和临时目录，逐项核对固定校验值。表中是程序内部计时中位数，单位 ms；不含进程启动。

同后端 C++ 对照用于分离 TX 公开调用路径与原生调用的开销，不能当作另一套独立数据库或密码学实现。Java/Python 使用各自标准库及驱动；下方逐项说明不同语义、缓存和生命周期。Java 是短命 JVM 负载，未做 JMH 式充分 JIT 预热，不能外推为长驻服务器的稳态表现。

## 四语言结果

| 负载 | TX | C++ | Java | Python | TX/C++ | TX/Java | TX/Python |
| --- | --- | --- | --- | --- | --- | --- | --- |
| thread_spawn_join | 14.472 | 13.788 | 24.567 | 16.435 | 1.05× | 0.59× | 0.88× |
| mutex_uncontended | 88.653 | 0.371 | 6.065 | 15.457 | 238.64× | 14.62× | 5.74× |
| atomic_add | 18.229 | 0.175 | 0.893 | 15.606 | 103.99× | 20.42× | 1.17× |
| channel_send_recv | 21.700 | 0.792 | 3.709 | 31.367 | 27.40× | 5.85× | 0.69× |
| task_spawn_wait | 7.511 | 6.972 | 22.573 | 16.894 | 1.08× | 0.33× | 0.44× |
| sqlite_insert | 11.518 | 8.347 | 25.236 | 2.017 | 1.38× | 0.46× | 5.71× |
| sqlite_read | 70.523 | 9.123 | 26.298 | 9.744 | 7.73× | 2.68× | 7.24× |
| sqlite_savepoint | 2.536 | 1.900 | 7.090 | 0.674 | 1.33× | 0.36× | 3.76× |
| sqlite_pool | 81.674 | 38.883 | 72.906 | 57.304 | 2.10× | 1.12× | 1.43× |
| sqlite_async | 52.326 | 49.199 | 73.860 | 50.588 | 1.06× | 0.71× | 1.03× |
| migration_recheck | 2.135 | 2.082 | 10.520 | 0.936 | 1.03× | 0.20× | 2.28× |
| postgres_insert | 194.082 | 174.562 | 283.019 | 261.071 | 1.11× | 0.69× | 0.74× |
| postgres_read | 87.601 | 23.160 | 33.521 | 17.761 | 3.78× | 2.61× | 4.93× |
| postgres_savepoint | 127.049 | 105.580 | 107.404 | 96.147 | 1.20× | 1.18× | 1.32× |
| secret_equal | 16.461 | 8.918 | 26.976 | 12.043 | 1.85× | 0.61× | 1.37× |
| argon2_hash_verify | 161.069 | 159.156 | 538.739 | 176.491 | 1.01× | 0.30× | 0.91× |
| ed25519_sign | 17.338 | 17.069 | 436.166 | 17.374 | 1.02× | 0.04× | 1.00× |
| ed25519_verify | 24.818 | 24.670 | 388.696 | 47.691 | 1.01× | 0.06× | 0.52× |
| x509_parse_der | 3.049 | 2.647 | 7.129 | 3.183 | 1.15× | 0.43× | 0.96× |
| dns_localhost | 0.305 | 34.121 | 56.632 | 41.333 | 0.01× | 0.01× | 0.01× |
| udp_echo | 36.365 | 25.620 | 44.360 | 30.038 | 1.42× | 0.82× | 1.21× |
| ipc_echo | 13.849 | 8.872 | 12.353 | 9.979 | 1.56× | 1.12× | 1.39× |
| tls_handshake | 266.349 | 26.951 | 249.359 | 72.734 | 9.88× | 1.07× | 3.66× |
| async_file_rw | 41.834 | 33.910 | 79.788 | 45.422 | 1.23× | 0.52× | 0.92× |
| test_parameterized | 4.361 | 0.058 | 0.838 | 1.562 | 74.80× | 5.20× | 2.79× |
| test_property | 4.389 | 0.056 | 1.107 | 2.327 | 78.38× | 3.97× | 1.89× |
| log_filtered | 45.575 | 0.443 | 4.152 | 6.527 | 102.92× | 10.98× | 6.98× |
| log_file | 37.050 | 29.157 | 51.959 | 32.780 | 1.27× | 0.71× | 1.13× |
| profile_spans | 0.437 | 0.224 | 1.158 | 0.347 | 1.95× | 0.38× | 1.26× |

## ≥3× 清单

按未四舍五入的同组中位数比值筛选；同一负载对不同语言的条目不能累计为独立热点。

| 项目 | 参考 | TX ms | 参考 ms | 倍率 |
| --- | --- | --- | --- | --- |
| mutex_uncontended | C++ | 88.653 | 0.371 | 238.64× |
| atomic_add | C++ | 18.229 | 0.175 | 103.99× |
| log_filtered | C++ | 45.575 | 0.443 | 102.92× |
| test_property | C++ | 4.389 | 0.056 | 78.38× |
| test_parameterized | C++ | 4.361 | 0.058 | 74.80× |
| channel_send_recv | C++ | 21.700 | 0.792 | 27.40× |
| atomic_add | Java | 18.229 | 0.893 | 20.42× |
| mutex_uncontended | Java | 88.653 | 6.065 | 14.62× |
| log_filtered | Java | 45.575 | 4.152 | 10.98× |
| tls_handshake | C++ | 266.349 | 26.951 | 9.88× |
| sqlite_read | C++ | 70.523 | 9.123 | 7.73× |
| sqlite_read | Python | 70.523 | 9.744 | 7.24× |
| log_filtered | Python | 45.575 | 6.527 | 6.98× |
| channel_send_recv | Java | 21.700 | 3.709 | 5.85× |
| mutex_uncontended | Python | 88.653 | 15.457 | 5.74× |
| sqlite_insert | Python | 11.518 | 2.017 | 5.71× |
| test_parameterized | Java | 4.361 | 0.838 | 5.20× |
| postgres_read | Python | 87.601 | 17.761 | 4.93× |
| test_property | Java | 4.389 | 1.107 | 3.97× |
| postgres_read | C++ | 87.601 | 23.160 | 3.78× |
| sqlite_savepoint | Python | 2.536 | 0.674 | 3.76× |
| tls_handshake | Python | 266.349 | 72.734 | 3.66× |

## 工作量与可比边界

| 负载 | 模块 | 工作量 | 参考及边界 |
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
| log_filtered | log | 100000 次被过滤的惰性 debug 日志 | 采用校准样本：C++ 调用真实 TX 日志后端级别检查，Java Logger，Python logging；各参考仅在级别允许时构造字段，TX 另有闭包/错误边界。废弃初版 C++ volatile 检查倍率。 |
| log_file | log | 2000 条带上下文和敏感字段遮蔽的 JSONL | C++ 同日志后端；Java/Python 固定 schema 构造。全部每条 flush、进程内加锁；计时外核对行数、索引、请求标识、遮蔽值。未覆盖轮转压力。 |
| profile_spans | profile | 1000 次分段计时并保留记录 | TX/C++ 启用真实分析器；Java/Python 仅保存时间段记录，没有同时启用 CPU/分配采样。仅为 API 负载参考，不能宣称完整 profiler 等价。 |

## 版本、复现和证据

- Windows 11 26200，Ryzen 7 6800H，8 核 / 16 线程；GCC 13.1.0 / C++23 -O3，TX clang 23.1.2 -O3，Java 23.0.2，Python 3.12.10。
- TX/C++ SQLite 3.53.4、libpq 18.4；Java sqlite-jdbc 3.46.1.0、postgresql 42.7.4、BouncyCastle 1.78.1、slf4j-api 2.0.16；Python SQLite 3.49.1、psycopg 3.2.3（随包 libpq 14.12）、cryptography 48.0.1、argon2-cffi 25.1.0。版本不同属于结果边界。
- 所有网络地址均为本机；数据库为辅助脚本建立的临时 TLS PostgreSQL 18.4。服务停止、临时数据目录清理均由上下文管理器完成。秘密夹具只在 tx_build 中，未归档、打印或写入报告。
- 主采样与校准期间各自的源码、TX 工具链、参考二进制及 Java 依赖指纹均未改变；参见 [manifest.json](manifest.json)、[completed.json](completed.json)、[calibration_manifest.json](calibration_manifest.json)、[calibration_completed.json](calibration_completed.json)。
- [results.json](results.json) 保留最初正式样本；[calibration_results.json](calibration_results.json) 为修正 C++ 过滤日志对照后的完整 diagnostics 五轮样本；[final_results.json](final_results.json) 以校准组覆盖原组，是本报告唯一数据源。最初简化日志参考不进入最终清单。
- 准备入口：`python -X utf8 -B benchmarks/stdlib_full_2026-09-30/prepare.py`；正式入口：`python -X utf8 -B benchmarks/stdlib_full_2026-09-30/run.py`；汇总入口：`python -X utf8 -B benchmarks/stdlib_full_2026-09-30/summarize.py`。脚本拒绝覆盖完成记录；再次完整复测应使用新的归档目录。
- 补测为所有模块提供代表性能负载，不代表覆盖每个 API、失败路径、长时间负载或所有网络协议。profile 的完整 CPU/分配采样开销、日志轮转压力等不由本次代表负载证明。

