# 取消令牌与截止时间

导入 `cancel.txh` 后，`source()` 创建取消源，`token(source)` 取得可跨模块传递的只读令牌；`with_deadline_ms(timeout_ms)` 使用单调时钟创建带相对截止时间的取消源。`cancel(source)` 首次主动取消返回 true，已经取消或已到截止时间返回 false。普通赋值与 `deep_copy` 均共享同一取消状态，不会复制出互不关联的新令牌。`cancel_source` 只能触发取消，`cancel_token` 只能观察或等待；二者均为不透明内置类型，不能从整数句柄构造。

`status(token)` 与 `wait(token, timeout_ms)` 返回稳定整数状态：0 表示仍有效（对 `wait` 而言是等待超时），1 表示主动取消，2 表示截止时间已到。`wait` 的 `timeout_ms = -1` 表示持续等待直到取消或截止时间，0 表示仅检查当前状态，其他负值报 `runtime_error/invalid_argument`。取消会立即通知该令牌上的等待者；带截止时间的等待最迟在单调截止时刻醒来。截止时间与等待超时均以毫秒计，超出单调时钟可表示范围报 `runtime_error/out_of_range`。截止时间在源创建时固定，系统墙上时间调整不改变它。

竞争判定以操作的结果提交点为准：完成结果先提交则返回完成，取消状态先被观察到则返回 `cancelled_error`；同一临界区内主动取消优先于恰好同时到期的截止时间。取消是协作式信号，不会撤销已写入的文件、已发送的字节或已提交的事务。未来文件、网络、进程和数据库 API 接入令牌时，应在各自结果中报告已产生的外部效果（例如已写字节数、进程是否启动、事务是否提交）与句柄后续可用性；不能用空值伪装取消或无条件重试。`cancel` 模块本身只改变内存中的取消状态，没有外部副作用。

`cancel_source/cancel_token` 可在 `any` 中显式恢复；其文本展示不包含内部地址。现阶段 `wait` 是第一个可取消阻塞点，其他阻塞 I/O 的具体接入在对应模块小项实施。示例见[取消令牌](../examples/cancellation.tx)。

## 2.8 实施记录

- **代码：** `cancel.txh` 公开源、令牌、主动取消、单调截止时间、状态及阻塞等待；内置类型与编译器静态签名、C ABI 和动态边界接通。源与令牌共享线程安全状态，`cancel` 以条件变量唤醒等待者；普通赋值和 `deep_copy` 维持同一取消域。
- **构建：** `scripts/build.ps1` 在 Windows x64 成功生成编译器、静态库和兼容清单。
- **定向验证：** `examples/cancellation.tx` 输出活动、取消与截止状态，覆盖 `any` 恢复和 `deep_copy`；`tests/stdlib/cancel_module/main.tx` 跨 `.txh` 输出 `0`、`1`。`tests/stdlib/cancel_wake.cpp` 使用独立线程验证主动取消唤醒阻塞等待与单调截止唤醒；非法等待超时输出 `runtime_error/invalid_argument` 路径。未运行全量套件。
- **边界与验收：** 令牌目前接入自身的 `wait`；文件、socket、进程及数据库的具体取消接入分别在对应后续小项实现。外部效果报告规则已固定，但这些模块未接入前不声称其操作可取消。当前执行证据限 Windows x64。
