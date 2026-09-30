# 提交后标准库完整复测

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

## ≥3× 项目

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
