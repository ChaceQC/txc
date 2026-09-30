# 提交后标准库完整复测

版本 `da798cde02fdd8aba65393ea1d8df77850915dc3`。预热 1 轮、正式 5 轮，轮换四语言执行顺序，内部计时中位数，单位 ms。
29 项全部完成，逐轮校验值一致。使用当前已修正的日志参考，不混入历史校准样本。
同后端 C++ 对照衡量公开调用路径成本；Java 为短命 JVM，不能外推到充分预热的稳态服务器。

| 负载 | TX | C++ | Java | Python | TX/C++ | TX/Java | TX/Python |
| --- | --- | --- | --- | --- | --- | --- | --- |
| thread_spawn_join | 13.894 | 13.637 | 25.363 | 16.631 | 1.02× | 0.55× | 0.84× |
| mutex_uncontended | 20.630 | 0.374 | 5.801 | 15.841 | 55.12× | 3.56× | 1.30× |
| atomic_add | 0.287 | 0.175 | 0.854 | 16.360 | 1.64× | 0.34× | 0.02× |
| channel_send_recv | 2.036 | 0.791 | 4.382 | 31.783 | 2.57× | 0.46× | 0.06× |
| task_spawn_wait | 7.247 | 7.053 | 21.844 | 17.152 | 1.03× | 0.33× | 0.42× |
| sqlite_insert | 11.744 | 8.598 | 27.514 | 1.923 | 1.37× | 0.43× | 6.11× |
| sqlite_read | 26.512 | 9.033 | 28.793 | 9.495 | 2.93× | 0.92× | 2.79× |
| sqlite_savepoint | 2.710 | 2.052 | 7.212 | 0.645 | 1.32× | 0.38× | 4.20× |
| sqlite_pool | 139.236 | 38.425 | 71.788 | 136.348 | 3.62× | 1.94× | 1.02× |
| sqlite_async | 54.127 | 48.711 | 75.827 | 50.628 | 1.11× | 0.71× | 1.07× |
| migration_recheck | 2.033 | 2.324 | 11.855 | 0.922 | 0.87× | 0.17× | 2.20× |
| postgres_insert | 202.260 | 175.155 | 268.197 | 255.589 | 1.15× | 0.75× | 0.79× |
| postgres_read | 45.568 | 23.334 | 30.711 | 18.421 | 1.95× | 1.48× | 2.47× |
| postgres_savepoint | 121.967 | 118.973 | 103.236 | 103.253 | 1.03× | 1.18× | 1.18× |
| secret_equal | 16.526 | 8.699 | 26.833 | 11.997 | 1.90× | 0.62× | 1.38× |
| argon2_hash_verify | 159.154 | 157.101 | 518.394 | 166.894 | 1.01× | 0.31× | 0.95× |
| ed25519_sign | 17.087 | 16.604 | 404.777 | 16.965 | 1.03× | 0.04× | 1.01× |
| ed25519_verify | 24.950 | 24.261 | 356.994 | 46.313 | 1.03× | 0.07× | 0.54× |
| x509_parse_der | 2.739 | 2.479 | 6.053 | 3.105 | 1.10× | 0.45× | 0.88× |
| dns_localhost | 0.305 | 32.077 | 54.779 | 43.514 | 0.01× | 0.01× | 0.01× |
| udp_echo | 35.372 | 25.117 | 44.667 | 31.081 | 1.41× | 0.79× | 1.14× |
| ipc_echo | 12.739 | 8.882 | 12.072 | 9.777 | 1.43× | 1.06× | 1.30× |
| tls_handshake | 114.903 | 24.220 | 227.613 | 61.128 | 4.74× | 0.50× | 1.88× |
| async_file_rw | 39.435 | 33.419 | 78.175 | 59.702 | 1.18× | 0.50× | 0.66× |
| test_parameterized | 0.377 | 0.062 | 0.850 | 1.547 | 6.08× | 0.44× | 0.24× |
| test_property | 0.664 | 0.047 | 1.141 | 2.410 | 14.07× | 0.58× | 0.28× |
| log_filtered | 1.024 | 0.424 | 3.852 | 6.499 | 2.41× | 0.27× | 0.16× |
| log_file | 32.115 | 30.311 | 50.884 | 57.828 | 1.06× | 0.63× | 0.56× |
| profile_spans | 0.441 | 0.226 | 1.159 | 0.337 | 1.95× | 0.38× | 1.31× |

## ≥3× 项目

| 套件 | 项目 | 参考 | TX ms | 参考 ms | 倍率 | 可比边界 |
| --- | --- | --- | --- | --- | --- | --- |
| 新增/concurrency | mutex_uncontended | C++ | 20.630000 | 0.374300 | 55.12× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |
| 新增/diagnostics | test_property | C++ | 0.664000 | 0.047200 | 14.07× | 固定 seed=1000，参考保留两个回调；本负载不执行失败缩减。TX 还保留受检回调及测试错误处理。 |
| 新增/sqlite | sqlite_insert | Python | 11.744000 | 1.923000 | 6.11× | C++ 直接调用 TX 的同一 C++ 数据库后端；Java SQLite JDBC；Python sqlite3。SQLite 版本不同，事务内逐条执行，不使用批插入。 |
| 新增/diagnostics | test_parameterized | C++ | 0.377000 | 0.062000 | 6.08× | 参考使用 std::function / IntConsumer / Python 函数；TX 含真实测试框架和受检回调。参考没有 TX 错误上下文，不外推到失败报告。 |
| 新增/network | tls_handshake | C++ | 114.903000 | 24.220200 | 4.74× | 全部验证临时 CA 和 localhost，禁用服务端票据，Java 每次废弃 session；TX mbedTLS，C++/Python OpenSSL，Java JSSE。配置在计时外，库内部握手上下文分配计时。 |
| 新增/sqlite | sqlite_savepoint | Python | 2.710000 | 0.645100 | 4.20× | C++ 同后端；Java JDBC、Python sqlite3；在内存库中测量，不表示持久化落盘吞吐。 |
| 新增/sqlite | sqlite_pool | C++ | 139.236000 | 38.424900 | 3.62× | TX 与 C++ 测真实池；其 SQLite 归还策略为关闭重开。Java/Python 参考每次连接关闭，保持生命周期，未复制完整池等待/别名检查。 |
| 新增/concurrency | mutex_uncontended | Java | 20.630000 | 5.800600 | 3.56× | TX 为受保护值及 guard API；C++ std::mutex；Java ReentrantLock；Python Lock。衡量整段公开 API 路径，不是单条锁指令。 |

## 工作量及边界

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

原始数据见 [results.json](results.json)、[manifest.json](manifest.json)、[completed.json](completed.json)。
采样期间源码、工具链和参考程序指纹未变化。服务为本机临时实例，秘密夹具不进入归档。
