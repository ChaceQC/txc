# Send/Sync 与跨线程所有权边界

本文固定 2.6 的静态规则、运行时边界和验收项；[实施清单](standard_library_plan.md)按 2.6a 基础规则和 2.6b 线程/锁集成记录进度。

## 能力判定

`Send` 表示值可在调用线程完全放弃访问后转交给另一个线程；`Sync` 表示值可由多个线程同时访问。两者在编译期按静态类型递归判定，不由运行时 `any` 内容或整数句柄值推断。

| 类型 | Send | Sync |
| --- | --- | --- |
| `int/float/bool/str/bytes/none` | 是 | 是 |
| `db_row/db_value` | 是；不可变快照/按值 SQL 值，不引用连接 | 是；字符串按值、字节与行底层只读 |
| `db_pool` | 是 | 是；内部同步；借用的连接仍禁止跨线程 |
| 图形资源 `graphics_app/window/canvas/image/surface/path/brush/font/text_layout`、`gui_control/menu` | 否；嵌套字段和容器同样拒绝 | 否；原生操作绑定 UI 线程 |
| `option<T>`、`result<T>` | T 和错误表示满足 Send 时 | T 和错误表示满足 Sync 时 |
| `vector<T>`、`map<K,V>`、`set<T>`、`ordered_map<K,V>`、`ordered_set<T>`、`heap<T>`、`queue<T>`、`deque<T>` | 元素、键和值均满足 Send，且值被唯一移动 | 否；容器本身可变 |
| 用户 `struct` | 所有字段满足 Send，且值被唯一移动 | 否；字段可写 |
| 用户 `class` | 默认否；类析构/方法尚未逐类证明 | 否 |
| `mutex<T>`、`rw_lock<T>`、`atomic<T>`、`condition`、`semaphore`、`once`、`cancel_token`、`channel<T>` | 仅在各自实现允许的 T 范围内 | 是；公开操作受内部同步保护 |
| 锁卫士、迭代器、`array/dict/any`、未检查捕获的函数闭包 | 否 | 否 |
| 文件、网络、进程及其他不透明句柄 | 默认否，除非模块逐类证明并声明 | 默认否 |
| `httpx.listener`、`httpx.client_session` | 是；共享底层登记状态 | 是；监听接入及会话池由运行时同步，关闭后所有别名失效 |
| `websocket.connection` | 是；新建连接可唯一移动到任务工作线程 | 否；同一连接的收发状态不允许并发别名操作 |

`httpx.connection` 和 `httpx.client_request` 仍按可变请求资源处理，不声明 `Sync`。共享监听器的多个 TX 线程各自在自己的运行时上下文中执行处理函数；C++ 网络线程只处理 I/O，不直接调用 TX 回调。HTTP/1.1 监听端按连接数限额及接受锁协调，HTTP/2 会话按流锁和流限额协调；接口与验证见[网络模块](network.md)。

编译期断言 `assert_send(value)` 与 `assert_sync(value)` 正常求值实参后返回 `void`；不满足规则时在实参位置报告中文诊断。`assert_send` 对可变复合值还要求当前作用域可证明该值唯一拥有且未逃逸；它只检查、不消费该值。`assert_sync` 不把普通可变容器或可写结构体视为安全共享。

## 唯一移动与闭包

显式表达式 `move(value)` 消费一个局部变量的可变 Send 值，并使该变量从此不可读写。移动仅接受简单变量名；如果参数、普通赋值别名、闭包、容器、对象字段或未知调用可能保留别名，编译器拒绝移动。条件分支、循环和异常分支中的 move 暂不支持，以免路径合并后错误地恢复已移动值。新建值、`deep_copy` 的结果以及 `move` 后重新绑定的值可继续按唯一所有权规则传递。

跨线程闭包必须由顶层 TX 函数和 `bind` 组成。标量及不可变文本/字节捕获按值复制；可变复合捕获必须写成 `bind(work, move(value))`，并且其字段/元素类型递归满足 Send。普通别名不能直接并发写。

```tx
import "thread.txh" as thread

def increase(values: vector<int>) -> void
{
    values[0] = values[0] + 1
}

def main() -> int
{
    vector<int> values = vector<int>(1, 0)
    join_handle<void> worker = thread.spawn(bind(increase, move(values)))
    thread.join(worker)
    return 0
}
```

## 同步共享与运行时

`mutex<T>`、`rw_lock<T>` 和 `channel<T>` 的元素类型须满足 `Send`；可变复合初值、写入值和通道消息须唯一 `move` 或新构造。锁卫士只能在当前词法作用域使用，不能复制、返回、装入容器、传给普通函数或跨 `await`。所有受保护数据访问都必须经过相应卫士；读取复合值会得到独立拷贝，不能把可变别名带到锁外。`atomic<T>` 仍限 `int/bool`。

跨线程闭包边界复制值内容，由目标线程建立自己的 TX 句柄；普通 `any` 根句柄只在创建线程的运行时上下文中登记和注销，不为根记录加锁。`mutex`、`channel`、`cancel_token` 等可共享载荷仍由各自实现同步，根句柄不承担载荷同步。原线程登记的 C ABI 句柄不会在另一个线程直接注销。文本根句柄同样由所属线程管理；不可变文本内容使用原子引用计数，可由容器或闭包跨线程持有。循环回收节点跨线程统一登记；工作线程运行 TX 回调期间不执行自动循环扫描，回调退出时只扫描该线程登记的节点。跨上下文对象图由主线程在 join 或任务作用域收束后尝试整体扫描；只要仍有工作线程运行，整体扫描就会延后，避免扫描正在被修改的对象图。

文本首次创建时建立一个线程本地根和独立计数的内容；二者可同次分配，根从清理链表注销后，内容仍由其余根或容器引用保持存活。`str` 克隆只创建调用线程的根并增加内容引用，不复制文本字节。`text_reference` 保存内容引用，容器槽位中的借用指针仅用于已证明拥有者存活的同步只读调用，不能当作根释放或保存到调用外；需要跨线程交付 TX 句柄时在目标线程克隆为本地根。最后一次内容释放可发生在其他线程，原子计数保证此时才销毁存储。格式化先独占构建，发布后不再追加；拼接与解码也在独占临时字符串中完成后才发布。

## 验收边界

2.6a 覆盖递归 Send/Sync 判定、唯一移动、移动后不可用、普通别名/逃逸拒绝、不透明句柄默认拒绝，以及跨线程释放和循环回收安全点。2.6b 覆盖 TX `thread.spawn/join` 和 `mutex<T>`：唯一移动与闭包捕获、锁外访问拒绝、线程错误交付、跨线程释放及循环回收。第 10 节的线程、同步原语、通道和任务终态记录见[标准库计划](standard_library_plan.md#10-线程任务与进程间通信)。

**2.6 完成记录（2026-09-28）：** `assert_send/assert_sync` 现在使用统一递归类型判定；可变 Send 复合值通过 `move(name)` 消费唯一所有权，移动后访问、存在别名或已逃逸时在源码位置拒绝。线程/任务闭包捕获、容器/调用逃逸、条件路径移动和锁卫士生命周期均接入同一规则。运行时文本内部引用加互斥；跨线程闭包在目标线程建立本地句柄；GC 节点以运行时上下文 ID 全局登记，工作线程只扫描本地节点，join/任务收束后主线程尝试扫描完整对象图。Windows x64 `scripts/build.ps1` 成功并清理 `build/`；`send_sync_move.tx` 覆盖字符串向量、用户结构体和嵌套 vector 的 thread/task 唯一移动与 worker 循环回收，`thread_behavior.tx`、`sync_behavior.tx` 和 `task_behavior.tx` 也编译运行通过。别名、移动后读取、普通闭包捕获、锁卫士逃逸和不满足 Sync 的容器均在源码位置拒绝；`scripts/check_runtime_context.py` 通过，包含闭包循环及复活用例。第 10 节后续验收结果见[标准库计划](standard_library_plan.md#10-线程任务与进程间通信)。
