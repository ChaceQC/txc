# TCP/UDP Socket（11.2）

`socket.txh` 提供数值地址上的 TCP/UDP 接口；主机名先用 [`dns.resolve`](dns.md) 解析。IPv6 地址不带方括号。所有句柄只在当前进程有效，复制的 TX 结构体共享同一底层状态；`close` 使全部别名失效并可重复调用。句柄不满足 `Send/Sync`，异步入口在任务作用域中持有受控借用，作用域关闭前收束操作。进程退出和最后一个句柄对象释放时仍会兜底关闭。

`connect_async/accept_async` 的新建连接只通过 `task<tcp_stream>` 单向交付到等待方；这是网络运行时的受控所有权移交。用户自己创建的任务回调仍不能返回或捕获非 `Send` 的 TCP 句柄。

```tx
struct tcp_listener
{
    id: int
}
struct tcp_stream
{
    id: int
}
struct udp_socket
{
    id: int
}
struct read_result
{
    data: bytes
    eof: bool
}
struct datagram
{
    data: bytes
    host: str
    port: int
    truncated: bool
}

def listen_tcp(ip: str, port: int, backlog: int) -> tcp_listener
def listener_port(server: tcp_listener) -> int
def connect_tcp(ip: str, port: int, timeout_ms: int) -> tcp_stream
def accept_tcp(server: tcp_listener, timeout_ms: int) -> tcp_stream
def connect_async(ip: str, port: int, timeout_ms: int,
                  token: cancel_token) -> task<tcp_stream>
def accept_async(server: tcp_listener, timeout_ms: int,
                 token: cancel_token) -> task<tcp_stream>
def read(peer: tcp_stream, max_bytes: int, timeout_ms: int) -> read_result
def write(peer: tcp_stream, data: bytes, timeout_ms: int) -> int
def read_async(peer: tcp_stream, max_bytes: int, timeout_ms: int,
               token: cancel_token) -> task<read_result>
def write_async(peer: tcp_stream, data: bytes, timeout_ms: int,
                token: cancel_token) -> task<int>
def shutdown_read(peer: tcp_stream) -> void
def shutdown_write(peer: tcp_stream) -> void

def bind_udp(ip: str, port: int) -> udp_socket
def udp_port(endpoint: udp_socket) -> int
def send_to(endpoint: udp_socket, ip: str, port: int,
            data: bytes, timeout_ms: int) -> int
def receive_from(endpoint: udp_socket, max_bytes: int,
                 timeout_ms: int) -> datagram
def send_to_async(endpoint: udp_socket, ip: str, port: int,
                  data: bytes, timeout_ms: int, token: cancel_token) -> task<int>
def receive_from_async(endpoint: udp_socket, max_bytes: int,
                       timeout_ms: int, token: cancel_token) -> task<datagram>
def close(server: tcp_listener) -> void
def close(peer: tcp_stream) -> void
def close(endpoint: udp_socket) -> void
```

`port=0` 仅用于监听或 UDP 绑定，由系统选端口，之后用 `listener_port/udp_port` 查询；目的端口须为 1～65535。空监听地址表示所有本机地址。`backlog` 限 1～128。超时均为 1～60000 毫秒，使用单调时钟约束整个调用；超时不关闭有效句柄。所有传输使用非阻塞 socket 与有界等待，等待期间关闭可唤醒操作。异步操作进入现有有界任务队列，取消令牌与作用域取消在等待时生效；完成与取消交错时，以已完成的读写结果为准。

TCP 是字节流，`read` 单次最多返回 `min(max_bytes, 16 KiB)` 字节，`eof=true` 仅表示对端发送方向已经关闭；合法读取结果可比请求短。`write` 单次最多尝试 16 KiB，并返回确认写入的字节数；调用方循环写剩余部分。`data` 最多 16 MiB，空输入返回 0。`shutdown_write` 发出 TCP 半关闭；此后本端不能再写，但仍可读；`shutdown_read` 相反。关闭或不可恢复的传输失败会使句柄失效，部分已发送数据不能撤销。

UDP 每次 `send_to` 只发送一个完整报文，大小上限 65507 字节；失败时不报告成功字节数。`receive_from` 每次消费一个报文，使用固定 64 KiB 缓冲区；返回不超过调用方 `max_bytes` 的前缀，并以 `truncated=true` 表示该报文被截断，剩余部分不保留到下次读取。`max_bytes` 限 0～65507，零值只允许空报文完整返回。UDP 没有 EOF，合法空报文有 `truncated=false`。

失败分类：`io_error/invalid_argument`、`size_limit`、`timeout`、`closed_handle`、`connection_closed`、`operation_failed`；异步取消使用 `cancelled_error/cancelled` 或 `deadline_exceeded`。关闭后操作报 `closed_handle`（监听器、UDP）或 `connection_closed`（TCP）。等待超时和可恢复的 UDP 网络错误保留 `open`；TCP 对端关闭读取时仍可向对端写入，直到写方向也失效。异步任务失败由 `task.wait/await` 交付原错误。

## 定向验证（2026-09-28）

Windows x64 增量构建通过。`tests/network/socket_behavior.tx` 编译运行通过：本机 TCP 建连、异步建连/接受、二进制短读、重复 EOF、半关闭后反向写入、异步读写、UDP 报文截断与合法空报文、异步收发、预取消和重复关闭。`tests/network/socket_bad_send.tx` 在源码位置拒绝把监听句柄当作 `Send`。验证限本机回环与单进程状态，尚未覆盖跨平台和长时间网络故障注入。
