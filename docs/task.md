# 结构化任务与异步函数

## 公开语义

`task.scope(work, max_pending, deadline_ms)` 在调用线程执行 `fn(task_scope)->T`，其中 `T` 为 `void` 或满足 `Send` 的类型。`max_pending` 限制作用域内尚未完成的任务及尚未观察的任务错误数量，`deadline_ms=-1` 表示无截止时间。作用域结束前等待所有子任务；回调失败时先取消子任务，再等待其退出，最后交付原始错误。作用域内未被 `wait` 观察到的子任务错误在退出时交付，按创建顺序选第一条。`task.cancel(scope)` 可主动取消整个作用域，`task.token(scope)` 返回与截止时间共用状态的取消令牌。

`task.spawn(scope, work)` 接受 `fn()->T`，返回 `task<T>`；`task.wait` 只交付一次结果或原始错误，第二次等待报 `invalid_state`。跨工作线程的回调遵循 `thread.spawn` 的静态捕获限制。作用域关闭后不能再提交任务。队列和工作线程数都有固定上限，超出作用域额度或队列容量时立即报告错误，不会额外创建线程。`task.after(scope, delay_ms)` 在 IOCP 事件循环上完成计时任务，不占用工作线程；负延迟报错。取消或截止时间会唤醒等待；协作取消不能强制终止正在执行的普通同步函数，作用域仍等待其退出。

文本或复合任务回调因 TX 错误返回空结果时，`wait/await` 交付原错误类别、代码、消息与调用栈；若没有待传播的 TX 错误却返回空指针，则交付 `runtime_error/task_failed`，消息指出对应结果为空。每次任务回调使用独立运行时上下文，嵌套等待执行其他任务后恢复等待者的错误上下文。

计时任务以结果写入 `task<T>` 状态为完成界线：完成记录先于取消时仍可取得成功；取消先于完成记录时交付 `cancelled_error/cancelled`。已完成的任务结果不会因后来取消而回退。任务回调可用 `task.current_token()` 取得作用域令牌，检查取消并将令牌交给支持取消的标准库接口。

`async def name(...) -> T { ... }` 的声明返回类型 `T` 是函数体内 `return` 的类型；直接调用得到 `task<T>`，并提交到当前 `task.scope`。配对 `.txh` 用相同的 `async def name(...) -> T` 声明。异步函数限顶层函数、无默认值的普通位置参数；参数和非 `void` 结果须满足 `Send`。可变复合实参在调用处须唯一 `move`，工作线程结果在完成前保存到任务状态，`wait/await` 在调用线程建立本地句柄。`await value` 只允许出现在 `async def` 内，要求 `value: task<T>`，静态结果为 `T`；等待时工作线程可执行队列中的其他任务，避免嵌套任务占满工作线程。声明了锁卫士的词法作用域内禁止 `await`，即使已显式关闭卫士，也要离开该词法作用域；同步代码通过 `task.wait` 取得结果。调用异步函数时没有当前作用域会报 `invalid_state`。

IOCP 循环负责异步计时及重叠 I/O 完成投递；文件适配见[异步文件操作](async_file.md)。Socket 的六类异步操作由独立的有界网络就绪循环推进，不在通用任务工作线程上等待；语义见 [Socket](socket.md)。HTTP 和数据库分别属于第 11、12 节。已有同步接口保持原有行为。`task.spawn` 的直接 `bind` 捕获沿用 2.6 的递归 Send/Sync 检查与唯一 `move` 规则。跨上下文任务对象图、嵌套 `await` 和生命周期专项见 [task_graph_lifecycle.tx](../tests/stdlib/task_graph_lifecycle.tx)。
