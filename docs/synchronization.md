# 同步原语的当前接口

导入 `sync.txh` 后，可创建 `mutex<T>`、`rw_lock<T>` 和 `atomic<T>`。`mutex/rw_lock` 的 `T` 须满足 `Send`，可变复合初值及新值须唯一 `move` 或新构造；`atomic<T>` 只支持 `int/bool`。`lock/read_lock/write_lock` 返回不满足 `Send/Sync` 的卫士。卫士最后一个别名离开作用域时释放锁，也可调用对应 `*_close` 提前释放；关闭后访问报 `runtime_error/invalid_state`。编译器拒绝卫士复制、存入 `any`、普通函数转交或逃逸。`rw_read_guard<T>` 只有读取入口，写入必须持有 `rw_write_guard<T>`。`get` 对可变复合值返回独立深拷贝，不能将受保护对象的可变别名带出锁外；安全同步句柄仍共享其同步状态。

`condition.wait(signal, guard, timeout_ms, token)` 在等待时暂时释放 `mutex_guard<T>` 所持的锁，返回前重新取得锁。`notify_one/notify_all` 分别唤醒一个/所有等待者；伪唤醒可能发生，调用方须在持锁状态循环检查自己的条件。`wait` 返回 true 表示收到通知或伪唤醒，false 表示超时；取消抛出 `cancelled_error`。`semaphore.acquire` 成功返回 true，超时返回 false，取消抛错；`release` 不允许超过声明上限。`once.run_once` 成功后只执行一次，失败时允许其他调用重试，递归调用同一 `once` 报错。

所有等待的 `timeout_ms=-1` 表示无限等待，其他负值报错。取消状态在最多约 10 毫秒的等待片段后重新检查；当前没有把取消源直接订阅到这些条件变量。可用的原子顺序整数为 0=relaxed、1=acquire、2=release、3=acq_rel、4=seq_cst；load 禁止 release/acq_rel，store 禁止 acquire/acq_rel。compare_exchange 失败读取使用 relaxed。整数 fetch_add 检查溢出，返回更新前的值。同步原语不保证公平性；锁顺序死锁由调用方避免。`await` 已接入；声明锁卫士的词法作用域内，编译器在源码位置拒绝 `await`。

定向用例见 [sync_behavior.tx](../tests/stdlib/sync_behavior.tx)、[sync_send_values.tx](../tests/stdlib/sync_send_values.tx) 与 [sync_cancel.tx](../tests/stdlib/sync_cancel.tx)。第 2.6 节的唯一移动及跨线程句柄验收已完成；10.2 的复合值、取消等待和卫士清理专项已通过 Windows x64 定向运行。
