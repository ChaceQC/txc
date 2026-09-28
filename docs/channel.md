# 有界通道的当前接口

`channel.bounded<T>(capacity)` 创建容量为 1～1,000,000 的 `channel<T>`；声明目标为 `channel<T>` 时可省略显式类型实参。`T` 必须满足 `Send`，通道句柄可以由线程闭包直接 `bind` 捕获并在多个线程之间共享。可变复合消息须在发送处唯一 `move` 或新构造；接收端取得消息所有权。普通非 Send 句柄不能作为消息。

`send(channel, item, timeout_ms, token)` 在容量满时等待；交付成功返回 true，超时返回 false，已关闭时报 `runtime_error/channel_closed`。`recv` 返回 `option<T>`：关闭后仍先排空队列，队列空且已关闭才返回 `none`；未关闭但超时报 `runtime_error/timeout`，与 EOF 明确区分。`close` 首次关闭返回 true，重复关闭返回 false，并唤醒等待者。所有等待的 `timeout_ms=-1` 表示无限等待；取消和截止时间抛出 `cancelled_error`，最多约 10 毫秒后观察到取消。

`select(vector<channel<T>>, timeout_ms, token)` 原子取出一个就绪通道中的消息，返回只读 `selected<T>`，其中 `index` 为输入向量下标、`value` 为收到的 `option<T>`。关闭且空的通道以同一下标加 `none` 表示 EOF；超时以 `index=-1` 加 `none` 表示。多个就绪通道轮转选择；空向量报错。选择与接收都先准备返回对象，再从队列移除消息，避免结果分配失败后丢失已交付消息。交付与取消同时就绪时，先交付已入队消息；关闭后也先排空。

定向用例见 [channel_behavior.tx](../tests/stdlib/channel_behavior.tx)、[channel_send_values.tx](../tests/stdlib/channel_send_values.tx) 和 [concurrency_race.tx](../tests/stdlib/concurrency_race.tx)。第 2.6 节的通用 `Send/Sync` 与唯一移动前置验收已完成；10.3 的复合元素、关闭及取消专项已通过 Windows x64 定向运行。
