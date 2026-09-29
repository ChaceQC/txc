# 数据库第十二部分验收

2026-09-30 在 Windows x64 完成 12.5～12.7。构建使用仓库固定 SQLite 3.53.4 与 EDB libpq 18.4-1；公开接口通过 LLVM 直接调用 C ABI，代码与依赖归档到 `tx/`。本记录不代替第 13 节终态总验收。

## 证据与复现

| 范围 | 用例 | 本次结果 |
| --- | --- | --- |
| 迁移校验、连续版本、重复应用、失败恢复 | `tests/db/migration.tx`；SHA-256 预期值由 Python hashlib 独立计算 | SQLite/PostgreSQL 通过 |
| 两连接竞争与 DDL 事务边界 | `tests/db/advanced_native.cpp`；同版本仅一个执行者成功，失败 DDL 回滚，PostgreSQL CONCURRENTLY 拒绝 | 两驱动通过 |
| 异步参数快照、SQL 注入字符串、NULL/无行、约束错误、预取消、跨线程只读行值 | `tests/db/async.tx` | 两驱动通过 |
| 运行中取消、操作超时、池等待作用域取消、资源恢复、结果行/字节预算、RETURNING 写入回滚 | `tests/db/advanced_native.cpp` | 两驱动通过 |
| 公开迁移与 async/await 示例、异步参数误用 | `examples/db_migration.tx`、`examples/db_async.tx`、`tests/db/async_wrong_parameter.tx` | 3/3 通过 |
| 值类型与错误绑定、连接跨线程拒绝 | `scripts/check_db.py contracts` | 3/3 通过 |
| SQLite 参数化查询、约束、NULL/无行、保存点、回滚、关闭误用、文件/WAL/只读/忙等待、Python 互操作、原生清理 | `scripts/check_db.py sqlite` | 6/6 通过 |
| PostgreSQL 参数化查询、4000 行读取、SQLSTATE、约束、保存点及失败事务恢复 | `tests/db/postgres.tx` | 通过 |
| 池与关闭边界、TLS/认证、限额、断连、未读游标丢弃、会话清理、池等待上限与关闭唤醒 | `tests/db/pool.tx`、`tests/db/postgres_native.cpp` | 通过 |

实际执行的分组命令：

```powershell
./scripts/build.ps1 -Incremental
python scripts/check_db_postgres.py migration async advanced
python scripts/check_db.py contracts sqlite completion
python scripts/check_db_postgres.py postgres pool native
```

最后使用 `scripts/build.ps1` 确认发行产物并按仓库规则清理 `build/`。首次集成中修正了迁移 ABI 的 vector 句柄解包，并补齐 libpq 非阻塞取消函数的导入符号；上表记录的是修正后的通过结果。没有运行与数据库无关的全库测试。

PostgreSQL 脚本只在工作区临时目录初始化随机端口、本机 TLS 数据库，测试结束停止并清理实例；随机凭据不输出，不连接已有数据库。脚本依赖构建下载的固定 PostgreSQL 归档以及 Python cryptography。

## 支持与边界

- 迁移按单版本事务原子提交，历史表和锁键为库保留资源；可信部署 SQL 不应修改它们。两个驱动的非事务外部效果遵循各自原生语义。
- 异步查询使用有界工作队列和独占池借用，并显式限制结果内存预算；同步游标保留逐行读取。没有把同步连接改成可跨线程共享。
- 显式取消和操作/作用域截止时间覆盖池等待和查询运行。DNS 使用已有有界解析与显式 token，作用域取消在 DNS 返回后观察；本机文件系统调用的即时中断不作保证。PostgreSQL 原生取消传输最多等待 250 ms，然后丢弃连接。
- COMMIT 在途断连或取消可能无法确认提交，禁止自动重放；已经确认的提交结果不因随后取消被覆盖。迁移可重新连接核对版本和校验值。
- 此处覆盖 Windows x64 本机 SQLite 和临时 PostgreSQL TLS 实例。真实远程网络、协议模糊测试、跨平台和第 13 节跨模块业务链分别验收。
