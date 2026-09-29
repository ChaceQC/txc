# 数据库公共契约、SQLite 与 PostgreSQL

`db.txh` 提供同步数据库接口。12.1 固定公共类型，12.2 接入 SQLite，12.3 接入 PostgreSQL，12.4 提供有界连接池。迁移及异步查询继续由 12.5、12.6 实现；各项完成证据在实施后登记。

## 12.3 PostgreSQL 契约

`postgres_options(host, database, user, ca_file)` 返回独立的 `postgres_config`，默认端口 5432、连接超时 5000 ms、操作超时 30000 ms、单行/值限制 1 MiB。`open_postgres(config, password: secret_bytes)` 连接；密码不进入公开结构、URL、错误或日志。强制 TCP、UTF-8、TLS 1.2 以上和 `sslmode=verify-full`，必须提供非空 CA 文件；没有关闭验证或明文降级选项。主机必须是单个 DNS 名称或 IP，不能混入连接串。密码在 libpq 内部有副本，临时转换缓冲使用后清零，连接关闭时由 libpq 释放内部副本。

占位符为 `$1`、`$2` 等，参数按位置完整绑定，`parameter_index` 接受 `$数字`。扩展协议只接受一条语句；事务控制通过 API，普通 SQL 禁止事务控制、会话角色切换和预编译语句管理。`query` 在首次 `next` 时发送，使用 libpq 单行模式，不把全部结果积累为行数组。关闭已经开始、尚未读完的游标会中断并丢弃物理连接，已收到的行快照仍可读取；事务外写入是否已执行可能无法确定，不自动重放。尚未调用 `next` 的游标可直接关闭而保留连接。单连接至多一个活动游标、至多一个事务；事务内允许该游标，错误后不绕过资源状态继续执行。

原生 bool、int2/int4/int8、float4/float8、text/varchar/bpchar/name、bytea、numeric、timestamptz 分别映射到 bool/int/float/str/bytes/decimal/datetime；NULL 保持独立。其他 OID 要求 SQL 显式转换为受支持类型，不猜测含义；非有限数字拒绝。带时区时间按 UTC 返回并保留微秒精度；PostgreSQL 本身不保存原输入偏移。INSERT 的 `insert_id` 为空，需用 `RETURNING` 显式读取。

`sqlstate(connection)` 返回最近一次服务器失败的五位 SQLSTATE；无服务器状态时为空。错误码区分 `authentication_failed/tls_failed/connection_failed/connection_lost/constraint_violation/transaction_failed/conflict/timeout/cancelled/read_only/limit_exceeded/query_failed`。不回显 libpq 原始诊断，避免泄露 SQL、密码和数据。`transaction_status` 返回 `idle/active/failed/closed`。失败事务禁止普通 SQL 和提交，只能回滚到仍存在的保存点或整体回滚；断连或操作超时丢弃连接，提交期间断连的结果不能假定回滚。

`begin` 接受 `read_committed/repeatable_read/serializable`；只读由连接选项固定。保存点沿用公共命名和生命周期规则。SQL 函数、触发器的外部效果按 PostgreSQL 原生语义处理。

密码要求 1～4096 字节且不含 NUL；不从 URL、环境或密码文件补齐空密码。地址通过现有 `dns.resolve` 在连接时限内解析，然后用 `hostaddr` 交给 libpq；原始 `host` 仍用于证书验证。连接时限覆盖地址解析、连接、TLS、认证和初始会话设置。`read_only` 设置会话的默认只读事务，不代替服务端账号权限；数据库管理员函数等仍遵循服务器权限。

SQL、参数数量及列数量上限沿用公共约束。PostgreSQL 行上限按 libpq 返回的文本格式及列名计数，因此 bytea 的十六进制表示也计入；原生 libpq 会先接收单条记录，TX 行上限不宣称是 libpq 协议缓冲区的内存硬上限。

## 12.4 有界连接池契约

`pool(sqlite_config, max_connections, max_waiters)` 和 `postgres_pool(config, password, max_connections, max_waiters)` 创建惰性池；连接上限 1～256、等待者上限 0～4096。`db_pool` 是唯一允许 Send/Sync 的数据库句柄，内部同步；借出的 `db_connection` 仍只允许借用线程使用。

`acquire(pool, timeout_ms)` 按单调时钟限制获取等待和新建连接，0 表示不排队；耗尽或等待队列已满分别报 `pool_exhausted`，等待到期报 `timeout`。不保证 FIFO。`release(connection)` 归还，重复返回 false；`close(connection)` 对池借用执行归还。旧连接、语句、游标、事务别名全部失效，下次借用创建独立句柄。最后一个借用引用释放自动归还；子资源持有借用，仍存活时不会提前归还。

归还先终止游标、关闭语句、回滚事务。PostgreSQL 无活动结果时使用 `DISCARD ALL` 清理临时对象、会话参数、角色、监听和锁，再恢复固定 UTF-8/时间/只读设置；无法清理或存在未读结果则丢弃连接。SQLite 关闭物理连接并在下次借用重新打开，保证 temp 表及连接局部状态不残留；因此池拒绝 `:memory:`，文件内容保持不变。池不会把不确定的提交或失败查询重新发送。

`close_pool` 重复返回 false，立即关闭空闲连接并唤醒等待者（`pool_closed`）；已借出的连接允许当前借用完成，归还时销毁。池关闭后不再发出新借用。池持有密码的独立秘密副本，关闭后清零；关闭原输入 `secret_bytes` 不影响已创建的池。所有自动清理失败都丢弃物理连接，显式归还可报告清理失败。

`timeout_ms=0` 仅禁止排队；容量尚有空位时仍允许按驱动的连接超时新建连接。大于 0 时，排队和新建连接共享获取时限。SQLite 文件打开和关闭受本机文件系统行为约束，不具有强制中断任意内核文件调用的能力。

## 类型和调用

| 类型 | 表示与所有权 |
| --- | --- |
| `db.options` | SQLite 驱动、路径、只读、WAL、忙等待毫秒数和数据大小限制；由 `sqlite_options(path)` 给出默认值 |
| `db.postgres_config` | PostgreSQL 主机、端口、数据库、用户名、CA 文件、连接/操作超时及默认只读事务；密码单独传入 |
| `db_pool` | 可跨线程共享的有界池；每次借用有独立的连接句柄和创建线程 |
| `db_connection` | 不透明连接；普通赋值、传参和从 `any` 恢复共享同一状态 |
| `db_statement` | 属于一个连接的预编译语句；参数从 1 开始绑定 |
| `db.parameters` | `values: vector<db_value>`；`bind_all` 按位置绑定全部参数 |
| `db.execution` | `affected_rows: int` 与 `insert_id: option<int>`；无插入标识使用空 option |
| `db_cursor` | 逐行读取状态；`next` 返回 `option<db_row>`，不把整个结果集载入内存 |
| `db_row` | 不可变、独立的列名和值快照；游标前进、关闭或连接关闭后仍可读取 |
| `db_value` | 显式标记的动态 SQL 值；由 `null_value/int_value/float_value/bool_value/str_value/bytes_value/decimal_value/datetime_value` 创建 |
| `db_transaction` | 属于连接的事务；显式提交，未提交的最后一个引用释放时自动回滚 |

全部数据库不透明类型禁止手工构造，支持 `any` 显式恢复并检查实际类型。连接、语句、游标、事务和池禁止 `deep_copy`；不可变行和值可复制。只有 `db_pool` 声明 `Send/Sync`，其余六种类型编译期拒绝跨线程传递；原生层也检查连接创建线程，错误码为 `thread_violation`。SQL 列的动态类型只在读取/绑定边界检查，函数目标及签名在编译期确定并生成直接 C ABI 调用。

## NULL、无行、缺列与类型

- SQL NULL 是 `db_value` 的 `null` 标记。`as_*` 与 `get_*` 遇到 NULL 返回空 `option<T>`，0、false、空文本和空 BLOB 都是有值。
- `next(cursor)` 的空 `option<db_row>` 仅表示结果已结束。已结束但未关闭的游标再次读取仍返回空 option。
- 列序号从 0 开始。不存在的序号或名称抛 `database_error/column_missing`。同名列可按序号读取，名称查找存在多个匹配时抛 `ambiguous_column`。
- `get_int/get_float/get_str/get_bytes` 严格匹配 SQLite 的 INTEGER/REAL/TEXT/BLOB，不隐式解析文本或把整数转为浮点。`get_bool` 显式接受 INTEGER 0/1；其他数值抛 `type_mismatch`。值接口的布尔构造也可由 `as_bool` 读取。
- SQLite 没有原生 decimal 和带偏移时间类型：绑定为规范化 UTF-8 TEXT；`get_decimal/get_datetime` 是显式的 TEXT 解析，保留十进制精度及时间偏移。再次通过 `get_value` 取得的存储类型是 `str`，不会猜测其领域类型。格式不符抛 `type_mismatch`。
- NaN、正负无穷在 `float_value` 处拒绝，错误为 `invalid_argument`，避免 SQLite 把 NaN 当成 NULL。读取的非有限 REAL 同样拒绝，错误为 `type_mismatch`。
- `value_kind` 返回 `null/int/float/bool/str/bytes/decimal/datetime`。经 `array/any` 默认格式化行、值和资源时只显示类型标签，不输出 SQL、路径、参数或列内容；直接 `print` 不透明类型沿用已有静态限制。

## SQLite 选项与边界

`sqlite_options(path)` 的默认值为 `driver="sqlite"`、可读写并按需创建、`wal=false`、`busy_timeout_ms=5000`、`max_value_bytes=1048576`。此选项的驱动必须是 `sqlite`，其他值抛 `unsupported_driver`；PostgreSQL 使用独立的 `postgres_config/open_postgres`。文件路径必须是非空有效 UTF-8，不含 NUL；允许 `:memory:`，拒绝 `file:` URI。不会把路径当作连接 URL，不自动创建父目录。SQLite 自身负责文件打开的权限和竞争语义。

只读使用 `SQLITE_OPEN_READONLY`，不创建文件；只读和 WAL 设置不能同时启用。WAL 只用于文件数据库，并验证 `PRAGMA journal_mode=WAL` 实际返回 `wal`；内存数据库请求 WAL 抛 `invalid_argument`。忙等待为 0～2147483647 毫秒，0 表示立即报告 `busy`；锁等待耗尽仍报告 `busy`，不将它误报成查询无行。默认启用外键检查、禁用扩展加载及可信 schema。

SQLite 固定为 `SQLITE_THREADSAFE=1`，每个连接使用 `SQLITE_OPEN_FULLMUTEX`；TX 契约仍限制一个连接及其资源只能由创建线程操作。并发写入应打开不同连接。单连接只允许一个未结束游标；有活动游标时禁止准备/绑定/执行/开启事务或提交，抛 `invalid_state`。回滚及关闭主动终止活动游标。

SQL 一次只能包含一个实际语句，可有结尾注释和分号；空语句、NUL 或第二条语句被拒绝。占位符保留 SQLite 的 `?`、`?NNN`、`:name`、`@name`、`$name` 规则；`parameter_index` 使用包含前缀的原生名称，找不到抛 `parameter_missing`。SQLite 对 `?NNN` 的索引空洞也计入参数数量，必须显式绑定这些索引。执行前所有参数必须绑定，不能将未绑定项默认为 NULL。`bind_all` 要求数量准确，并先验证所有值再替换绑定。执行和查询结束后保留绑定，`clear_bindings` 显式清除。

`execute` 只接受不返回列的语句；SELECT 或带 RETURNING 的写入使用 `query`。`query` 的副作用在 `next` 实际执行时发生，关闭或出错可能已有外部效果，只有显式事务能保证回滚。`affected_rows` 为本语句的直接变更数，不计触发器；不修改行的 DDL 返回 0。普通表实际插入行且存在 SQLite rowid 时提供 `insert_id`；WITHOUT ROWID、虚拟表以及 UPSERT 的 UPDATE 分支不返回旧插入标识。驱动通过原生 preupdate hook 区分直接插入与触发器/更新，不解析 SQL 文本猜测。

`max_value_bytes` 为 1～67108864 字节，限制绑定值、单行快照的列名及文本/BLOB 总量，并作为 SQLite 编码行大小限制；SQLite 行编码开销也计入其限制。SQL 最大 1 MiB，参数最多 4096，列最多 1024，每连接未关闭语句最多 4096；达到大小上限报告 `limit_exceeded`。从数据库读取的 TEXT 和列名必须是有效 UTF-8，非法外部内容报告 `invalid_encoding`，不静默替换。游标读取失败会释放活动占用，已取得的行仍可用。

## 事务与资源状态

`begin(connection, mode)` 支持 SQLite 的 `deferred/immediate/exclusive`，不伪装成其他驱动的隔离级别。一个连接最多一个事务；嵌套事务使用 `savepoint/rollback_to/release_savepoint`。保存点名限 1～64 个 ASCII 字母、数字或下划线，不能重复；回滚到保存点撤销其后的修改及子保存点，保留目标保存点；释放保存点同时释放子保存点。

SQL 中的 BEGIN/COMMIT/ROLLBACK/SAVEPOINT、ATTACH/DETACH 和带设置值的 PRAGMA 被授权器拒绝，防止绕过资源状态、连接路径及固定选项；带表名等参数的只读 schema/完整性 PRAGMA 按显式清单放行且名称不区分大小写。SQLite 内部需要 ATTACH 的 VACUUM 同样受此限制；事务控制必须使用公开 API。提交失败不宣称成功，事务仍处于 SQLite 实际状态，可继续回滚；SQLite 自动回滚时同步失效事务句柄。提交后的调用不复用事务。异常传播本身不立即回滚仍有引用的事务，未提交事务最后引用释放才自动回滚；需要确定结果应显式回滚。

| 操作 | 成功后状态与重复调用 |
| --- | --- |
| `close(connection)` | 回滚未提交事务，终止游标，释放全部语句，然后关闭连接；所有资源别名失效；重复返回 false |
| `close_statement(statement)` | 终止所属游标并 finalize；重复或连接关闭后返回 false |
| `close_cursor(cursor)` | reset 语句并释放活动占用，不销毁语句；重复或上层关闭后返回 false |
| `commit(transaction)` | 提交并失效；重复调用抛 `invalid_state`；有活动游标时拒绝 |
| `rollback(transaction)` | 终止活动游标后回滚并失效；重复或连接已关闭返回 false |
| 最后一个资源引用释放 | 语句 finalize、游标 reset、事务回滚、连接关闭；子资源持有父资源，父子不构成引用环 |

底层 SQLite 错误码保留在 `extended_error_code(connection)`，表示最近一次底层失败，不用它替代 TX 稳定错误。稳定码包括 `connection_failed/query_failed/constraint_violation/busy/read_only/invalid_state/invalid_argument/parameter_missing/column_missing/ambiguous_column/type_mismatch/invalid_encoding/limit_exceeded/thread_violation/unsupported_driver`。错误信息为中文且不拼接 SQL、路径或参数内容；数据库失败可由 `error.database_error` 捕获。

## 依赖、例子与完成记录

**12.3 完成记录（2026-09-30）：** libpq 18.4-1、独立 PostgreSQL 选项与秘密密码、强制 TLS 验证、有界 DNS 和非阻塞连接等待、参数绑定、单行结果及 SQLSTATE 已接入。`tests/db/postgres.tx` 完成精确值往返、NULL/无行、4000 行读取、约束冲突和保存点恢复；原生用例完成错误 CA/主机名/密码、行上限、操作超时、只读和服务器终止验证。修正了 Windows 非阻塞建连的首次可写等待和异常事件通知。公开示例静态检查通过。

**12.4 完成记录（2026-09-30）：** 有界池、等待者限额、获取时限、关闭唤醒、显式与自动归还已交付。`tests/db/pool.tx` 验证 TX 线程共享、事务回滚及旧别名失效；`tests/db/postgres_native.cpp` 验证真实会话复用、临时表/会话参数清理、未读游标丢弃、服务器断连替换、容量/等待上限、超时、关闭中的借用与最后引用自动清理。`scripts/check_db_postgres.py` 支持 `postgres/pool/native/example` 分组，在随机端口的临时本机实例上执行，所有对应组均已通过；SQLite 原有定向回归 6/6 通过。最终 Windows x64 构建产物位于 `tx/`，只做了数据库相关验证。

PostgreSQL 固定包、校验值、GNU 导入库和运行库交付详见 [libpq 依赖说明](../third_party/postgresql/README.md)。发行不会安装或启动数据库服务；CA 与数据库账号由调用方提供。

固定 SQLite amalgamation 3.53.4：`https://www.sqlite.org/2026/sqlite-amalgamation-3530400.zip`，SHA-256 `1e71ddf93849c6a6ecf58b827c0692073d2dd7ee40196158068f7b29f422e87d`。CMake 下载并校验，C 静态库最终并入 `tx/libtxstdlib.a`；SQLite 公有领域声明随 `tx/SQLITE-LICENSE` 交付，无额外数据库 DLL。SQLITE 源码不需要用户安装或链接。

公开例子见 [sqlite.tx](../examples/sqlite.tx) 和 [postgres.tx](../examples/postgres.tx)。12.1～12.4 的记录覆盖 Windows x64 同步数据库接口，不替代 12.7 的数据库综合验收及第 13 节跨模块/跨平台验收。

**12.1 完成记录（2026-09-30）：** `.txh`、六类不透明类型、编译期签名检查、直接 ABI、值/行表示、`any` 检查及复制边界已交付。Windows x64 构建通过，`python scripts/check_db.py contracts` 3/3 通过：值构造/读取、NULL 与零/空值、完整十进制和时间偏移、错误类型/领域格式，以及错误绑定参数和 Send 静态诊断。连接、逐行读取和事务状态的实际 SQLite 路径由 12.2 单独验证。

**12.2 完成记录（2026-09-30）：** 连接、语句、绑定、游标、事务与保存点已接入固定版本 SQLite，原生错误与自动资源清理遵守上文状态表；CMake、依赖静态归档和许可证交付已接通。Windows x64 `scripts/build.ps1 -Incremental` 成功；`python scripts/check_db.py sqlite` 6/6 通过，覆盖公开示例、正常/失败查询与资源状态、文件/WAL/只读/忙等待、Python SQLite 互操作、原生线程与最后别名清理及非有限参数拒绝。随后对空 TEXT/BLOB、清除绑定、大写只读 PRAGMA 复核通过。验证脚本可按小项运行，TX 编译上限为 180 秒、程序执行上限为 15 秒。最后使用 `scripts/build.ps1` 确认完整发行产物，构建成功后清理 `build/`。未运行全库套件或其他平台验收；PostgreSQL、池、迁移、异步和数据库综合验收按后续计划推进。
