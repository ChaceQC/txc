# 网络模块：`httpx` 与 `websocket`

`httpx` 和 `websocket` 已实现同步文本与二进制接口。公开契约在 `tx/stdlib/httpx.txh`、`websocket.txh`；Windows 网络核心和 C ABI 位于 `src/stdlib/`、`src/backend/cpp/`。回调包装源码位于 `src/stdlib/`，构建时先编成目标文件，再归档进 `tx/libtxstdlib.a`；`tx/stdlib/` 只交付 `.txh`。实现基于已有 `bytes`、`error.io_error`、类型化 `map<str, str>` 和[函数参数 `fn`](function_values.md)，没有增加语言语法。客户端会话、连接池、显式代理、自动解压及按块读写见[可复用会话](httpx_sessions.md)。

服务端路由、共享监听器并发与 HTTP/2 多流边界见[服务端路由与并发](httpx_server.md)。

HTTP/3 的显式客户端、QUIC 服务端、协议回退与证书边界见[HTTP/3 与 QUIC](http3.md)。

## 目标与边界

- `httpx` 覆盖 HTTP/1.1、HTTP/2、HTTP/3 的显式客户端与对应监听入口；新增可复用 WinHTTP 客户端会话、按块传输、服务端路由与并发。HTTP/3 使用独立 MsQuic/nghttp3 路径。`websocket` 覆盖 WS/WSS 客户端和服务端。
- 同步接口由调用方控制超时和循环。服务端既能显式 `accept`、处理、`respond`，也能把不同的处理函数传给同一个 `serve_once`。回调类型在模块实现中确定，调用时仍由编译器按函数签名检查；不按字符串在运行时查找函数。
- 文本接口要求正文和消息为有效 UTF-8；二进制接口直接使用已有 `bytes` 值，不把任意字节伪装成 `str`。[文件流](bytes_file_stream.md)可直接接入新增流式接口；传输按固定大小缓冲逐块推进。
- 网络状态是进程内资源。`listener`、`connection` 为带内部编号的结构体，必须显式关闭；重复关闭无害。句柄不允许序列化后跨进程使用。进程退出时运行时兜底释放仍存活的连接。

## 公共类型

两个模块分别定义自己的 `listener` 和 `connection`，避免 HTTP 与 WS 句柄混用。字段 `id` 仅供运行时识别，不作为可持久化或可运算的网络地址。

```tx
# httpx.txh 中的文本类型
struct listener
{
    id: int
}

struct connection
{
    id: int
}

struct request
{
    connection: connection
    method: str
    target: str
    headers: map<str, str>
    body: str
}

struct response
{
    status: int
    headers: map<str, str>
    set_cookie: vector<str>
    body: str
}
```

`binary_request`、`binary_response` 与对应文本类型字段相同，仅 `body` 改为 `bytes`。`request.target` 保存路径与查询字符串，例如 `/items?page=2`；服务端不自动进行 URL 解码。请求和响应头使用 `map<str, str>`，名称按 ASCII 大小写不敏感规则规范化为小写。普通重复字段按 HTTP 允许的规则合并；`Set-Cookie` 单独放在响应的 `set_cookie` 中，按出现顺序保留。其他不能安全合并的重复字段报告 `io_error` / `protocol_error`，不会静默丢弃。响应状态码限 100～599；合法空正文与传输失败有区别。

## `httpx` 客户端

| 签名 | 行为 |
| --- | --- |
| `send(method: str, url: str, headers: map<str, str>, body: str, timeout_ms: int) -> response` | 发送完整请求并读取完整响应 |
| `get(url: str, timeout_ms: int) -> response` | 无自定义请求头的 GET 便捷接口 |
| `post(url: str, body: str, timeout_ms: int) -> response` | UTF-8 文本 POST 便捷接口 |
| `send_bytes(method: str, url: str, headers: map<str, str>, body: bytes, timeout_ms: int) -> binary_response` | 发送原始字节正文并读取字节响应 |
| `get_bytes(url: str, timeout_ms: int) -> binary_response`、`post_bytes(url: str, body: bytes, timeout_ms: int) -> binary_response` | 二进制便捷接口 |

URL 只接受 `http://` 和 `https://`，必须包含主机，允许显式端口、路径和查询。方法名须符合 HTTP token 语法；头名称和值拒绝 CR、LF、NUL，防止注入。客户端自动计算 `Content-Length`，调用方不能覆盖 `Host`、`Content-Length`、`Transfer-Encoding` 或连接管理字段。代理沿用 Windows 系统/WinHTTP 配置，HTTPS 证书按系统信任链和主机名校验，不提供跳过证书校验的参数。

状态码 4xx/5xx 是正常的 HTTP 响应，仍返回相应的响应结构；DNS、连接、TLS、超时、协议，以及文本接口的 UTF-8 失败才进入 `io_error`。不自动跟随重定向，调用方可检查 3xx 与 `Location`，避免在跨站跳转时意外转发认证头。响应头和正文在返回前完整读取，并受下述大小限制。

显式 HTTP/2 客户端使用 `send_http2/get_http2/post_http2` 及相应的 `_bytes` 版本，参数和返回类型与上述接口对应。`https://` 由 WinHTTP 要求 ALPN `h2`，继续使用系统代理和证书信任；`http://` 使用直连的 h2c prior knowledge，不使用 WinHTTP 代理配置。两种路径都不降级到 HTTP/1.1。

### 文件流分块传输

流式接口直接使用 `binary_stream` 的当前位置，不替调用方定位、刷新或关闭文件。`stream_response` 包含 `status`、`headers`、`set_cookie` 和实际写入目标流的 `body_length: int`；`stream_request` 包含 `connection`、`method`、`target`、`headers` 和已写入目标流的 `body_length: int`，没有内存中的 `body` 字段。

| 签名 | 行为 |
| --- | --- |
| `send_stream(method: str, url: str, headers: map<str, str>, source: binary_stream, source_length: int, destination: binary_stream, max_response_bytes: int, timeout_ms: int) -> stream_response` | 从源流读取指定字节数作为请求正文，并将响应正文逐块写入目标流 |
| `get_stream(url: str, destination: binary_stream, max_response_bytes: int, timeout_ms: int) -> stream_response` | 无请求正文的流式 GET |
| `send_http2_stream(...) -> stream_response`、`get_http2_stream(...) -> stream_response` | 参数分别对应 `send_stream`、`get_stream`，要求 HTTP/2 |
| `accept_stream(server: listener, destination: binary_stream, max_request_bytes: int, timeout_ms: int) -> stream_request` | 校验请求头和长度后，逐块写入请求正文 |
| `respond_stream(peer: connection, status: int, headers: map<str, str>, set_cookie: vector<str>, source: binary_stream, body_length: int) -> void` | 从源流逐块写出响应，并结束连接 |

`source_length`、`body_length` 必须非负，源流提前结束会报 `operation_failed`，连接随后关闭；源流剩余字节不自动发送。WinHTTP 客户端上传受单次请求长度字段限制，最多 `4 GiB - 1` 字节；h2c 客户端和服务端使用 `int` 所表示的长度。`max_request_bytes`、`max_response_bytes` 必须非负，可设为 0 只允许空正文；超过上限在写入正文前或写入过程中报 `size_limit`。HTTP/1.1 流式接口使用 `Content-Length`，服务端不接收 `Transfer-Encoding: chunked` 请求；客户端可读取 WinHTTP 解码后的 chunked 响应。HTTP/2 使用 DATA 帧与流控。

写入由同步网络调用和文件流写入共同提供背压，单次内存块为 16 KiB。发生超时、I/O 失败、超过上限或调用方关闭连接时停止传输；已写入目标文件的前缀保留，调用方可自行删除或续传。流式接口不提供事务回滚，也不在同一调用中做并发取消。

## `httpx` 服务端

| 签名 | 行为 |
| --- | --- |
| `listen(host: str, port: int) -> listener` | 绑定本机地址并开始监听 |
| `accept(server: listener, timeout_ms: int) -> request` | 接收并解析一个完整请求 |
| `respond(peer: connection, value: response) -> void` | 写出响应并结束该请求连接 |
| `serve_once(server: listener, handler: fn, timeout_ms: int) -> void` | 接收一个请求，调用处理函数并回复 |
| `accept_bytes(server: listener, timeout_ms: int) -> binary_request`、`respond_bytes(peer: connection, value: binary_response) -> void` | 接收和回复原始字节正文 |
| `serve_once_bytes(server: listener, handler: fn(binary_request) -> binary_response, timeout_ms: int) -> void` | 使用二进制处理函数处理一个请求 |
| `close(server: listener) -> void`、`close(peer: connection) -> void` | 释放监听或连接资源 |

`serve_once` 的处理函数签名是 `fn(httpx.request) -> httpx.response`，`serve_once_bytes` 是 `fn(httpx.binary_request) -> httpx.binary_response`。用户可在同一个监听器上传入不同的合格处理函数。每次调用只处理一个请求，服务循环由 TX 程序编写；处理函数报错时关闭本次连接并向调用方传播原错误。

显式路径适合需要先检查 `request.method`、`request.target` 再决定处理方式的程序。普通 `listen` 接受 HTTP/1.1；`respond` 与 `respond_bytes` 自动写入 `Content-Length` 与 `Connection: close`，成功后关闭 `peer`；若决定不响应，调用 `close(peer)`。关闭监听器不强行关闭已经接收的连接。HTTP/1.1 非空正文需要明确 `Content-Length`；不支持 chunked 上传、持久连接或 HTTP/3。非法请求返回明确的 400/413/431 等状态后关闭连接，内部 I/O 失败仍报告 `io_error`。

`host` 可用 `127.0.0.1`、`::1` 或本机绑定地址；空字符串表示监听所有本机地址。端口限 1～65535；不自动开放 Windows 防火墙。服务端启动默认不跨线程调度处理函数，同步 `accept` 或 `serve_once` 在调用线程运行。后续若要并发，需先确定 TX 的线程、共享对象及回调执行约束。

## HTTP/2 客户端与服务端

HTTP/2 的显式客户端接口使用 `send_http2`、`get_http2`、`post_http2`，对应二进制接口使用 `send_http2_bytes`、`get_http2_bytes`、`post_http2_bytes`；流式接口使用 `send_http2_stream`、`get_http2_stream`。参数与返回类型分别对应已有的 HTTP/1.1 接口。`https://` 使用 TLS ALPN `h2`，`http://` 使用 h2c prior knowledge；调用方明确选用 HTTP/2 时不回退 HTTP/1.1。

服务端使用 `listen_h2c(host: str, port: int) -> listener` 或 `listen_h2_tls(host: str, port: int, cert_pem: str, key_pem: str) -> listener`。TLS 证书链和私钥从 UTF-8 路径的 PEM 文件加载，要求 ALPN `h2`；私钥不在日志或诊断中输出。两个监听器都沿用 `accept`、`accept_bytes`、`accept_stream`、`respond`、`respond_bytes`、`respond_stream`、`serve_once`、`serve_routes` 和 `close`，返回的 `connection` 指向 HTTP/2 流；回复或关闭该连接只结束对应流。h2c 使用 prior knowledge，不处理 HTTP/1.1 `Upgrade: h2c`。同一 TCP/TLS 会话最多公告 16 条并发流；多条已接收的流可交由不同 TX 工作线程处理，最多跟踪 16 个会话。nghttp2 负责帧、HPACK 与流控；内存接口仍限 8 MiB，流式接口按调用方上限逐块读写。server push 不提供。

HTTP/3 客户端用 `send_http3*` 或 `send_negotiated*`，服务端用 `listen_h3(host, port, package, password)`，其余请求/响应与路由操作沿用上述接口。HTTP/3 只使用 HTTPS 与 ALPN `h3`；具体 0-RTT、迁移、取消、流限额和证书策略见[独立协议文档](http3.md)。

## `websocket` 客户端与服务端

```tx
# websocket.txh 中的文本类型
struct listener
{
    id: int
}

struct connection
{
    id: int
}

struct message
{
    open: bool
    text: str
}
```

`binary_message` 包含 `open: bool` 和 `data: bytes`，以 `open=false` 区分正常关闭与合法空字节消息。`close_status` 包含 `received/code/reason`；接收返回 `open=false` 后、关闭句柄前可查询对端关闭帧。未带状态码的关闭帧使用本地查询值 `1005`，不会把该值发到网络。

| 签名 | 行为 |
| --- | --- |
| `connect(url: str, timeout_ms: int) -> connection` | 连接 `ws://` 或 `wss://` 服务端并完成握手 |
| `listen(host: str, port: int) -> listener` | 监听明文 WS 连接 |
| `listen_tls(host: str, port: int, config: tls.server_config) -> listener` | 使用 TLS 身份与客户端证书配置监听 WSS |
| `accept(server: listener, timeout_ms: int) -> connection` | 接受一个连接并完成 WebSocket 握手 |
| `upgrade(peer: httpx.connection) -> connection` | 将已由 `httpx.accept` 解析的 HTTP/1.1 连接移交给 WebSocket |
| `send_text(peer: connection, text: str) -> void` | 发送一条完整文本消息 |
| `send_binary(peer: connection, data: bytes) -> void` | 发送一条完整二进制消息 |
| `send_binary_stream(peer: connection, source: binary_stream, length: int) -> void` | 将源流分片发送为一条二进制消息 |
| `receive(peer: connection, timeout_ms: int) -> message` | 读取一条完整文本消息 |
| `receive_binary(peer: connection, timeout_ms: int) -> binary_message` | 读取一条完整二进制消息 |
| `receive_binary_stream(peer: connection, destination: binary_stream, max_message_bytes: int, timeout_ms: int) -> stream_message` | 逐块接收一条二进制消息 |
| `reply_once(peer: connection, handler: fn, timeout_ms: int) -> bool` | 读一条消息、调用处理函数、发送返回文本 |
| `reply_once_binary(peer: connection, handler: fn(bytes) -> bytes, timeout_ms: int) -> bool` | 读取字节消息、调用处理函数并回复 |
| `close(server: listener) -> void`、`close(peer: connection) -> void` | 关闭监听器或完成关闭握手并释放连接 |
| `get_close_status(peer: connection) -> close_status`、`close_with_reason(peer: connection, code: int, reason: str) -> void` | 查询对端关闭原因，或发送关闭码与最多 123 字节的 UTF-8 原因 |
| `ping(peer: connection, data: bytes) -> void`、`pong(peer: connection, data: bytes) -> void` | WS/WSS 服务端发送不超过 125 字节的控制帧；收到 ping 自动回复 pong，WinHTTP 客户端自动处理控制帧 |

`reply_once` 的处理函数签名是 `fn(str) -> str`，`reply_once_binary` 是 `fn(bytes) -> bytes`。不同处理函数可复用同一连接。返回 `false` 表示对端正常关闭且不会调用处理函数；合法空消息仍返回 `true` 并调用处理函数。显式接收接口用 `open=false` 区分正常关闭与合法空值。关闭后的连接再次收发会报错。

WebSocket 遵循 RFC 6455：校验握手、客户端掩码、帧长度及控制帧限制；接收分片消息时先合并再返回；收到 ping 自动回 pong；正常 close 返回 `open=false`。服务端发送不带掩码，客户端发送带掩码。文本读取接口收到二进制帧，或二进制读取接口收到文本帧时报告 `io_error` / `unsupported_frame`。同端口明文服务由 `httpx.listen` 创建监听器，先调用 `httpx.accept`，按路由和 Upgrade 头选择 `websocket.upgrade(request.connection)` 或普通 `httpx.respond`；移交后原 HTTP 连接失效。非法 Upgrade 会关闭这条连接并释放容量名额。HTTP/2 与 HTTP/3 流不接受 HTTP/1.1 Upgrade；WSS 当前使用独立的 TLS 监听入口。

WS 另提供 `send_binary_stream(peer: connection, source: binary_stream, length: int) -> void` 与 `receive_binary_stream(peer: connection, destination: binary_stream, max_message_bytes: int, timeout_ms: int) -> stream_message`。`stream_message` 包含 `open: bool`、`length: int`；正常关闭返回 `open=false, length=0`，合法空二进制消息返回 `open=true, length=0`。发送时一条消息拆成多个二进制延续帧，接收时逐块写入目标文件，最大长度由调用方给定。参数、部分写入和文件流所有权规则与 HTTP 流式接口相同；流中途失败时关闭 WS 连接，避免把未完成消息误当下一条消息。

## 超时、大小限制和错误

- 客户端 `timeout_ms` 必须大于零，分别约束连接、发送和读取阶段；服务端等待类接口允许 `0` 表示无限等待，其余值须为正。超时不会返回空字符串或正常关闭标记，而是 `error.io_error` / `timeout`。
- 请求头与响应头各限 64 KiB；整条正文或消息的内存接口限 8 MiB，包括二进制数据。流式接口由调用方给定接收上限，并维持 16 KiB 级内存块；超过限制用 `size_limit`。
- 输入校验、协议、连接状态和系统错误归为 `error.io_error`，`code` 使用 `invalid_url`、`invalid_header`、`invalid_argument`、`invalid_utf8`、`timeout`、`connection_closed`、`protocol_error`、`unsupported_frame`、`size_limit`、`operation_failed`。`message` 提供中文解释，但程序分支应依据 `code`。需要重试或继续服务时使用已有 `try { } exception errors.io_error as e { }`。
- HTTP/WS 对不可信网络输入不应触发编译器崩溃；每个失败路径须关闭部分建立的 socket、请求句柄和监听资源。同步服务端不保证处理函数执行期间的并发服务能力。

第 11.9 项增加 HTTP/1.1 完整请求读取总时限、客户端自动解压的压缩比例上限，以及证书、跳转、非法帧、取消和大文件流的本机专项证据，见[网络边界专项验收](network_11_9.md)。

## 实现分层与验证状态

1. `tx/stdlib/` 只放公开 `.txh`。`src/stdlib/httpx_bridge.tx` 与 `websocket_bridge.tx` 在构建时由 `txc emit-library-llvm` 编译并入 `libtxstdlib.a`，回调的函数签名仍由 TX 编译器静态检查。
2. `src/stdlib/` 用 WinHTTP 处理 HTTP/HTTPS 与 WS/WSS 客户端，用 Winsock 处理 HTTP/WS 明文监听；HTTP/2 h2c 客户端和服务端使用 nghttp2，TLS 服务端使用 Mbed TLS 处理 PEM 与 ALPN。共用 URL、UTF-8、头字段与大小限制校验。
3. `src/backend/cpp/` 提供明确的 C ABI 入口，LLVM 后端直接声明调用。构建脚本把固定版本的 nghttp2、nghttp3、Mbed TLS 静态库及 TX 回调包装归档进 `libtxstdlib.a`，将固定版本的 MsQuic 运行库与许可证随 `tx/` 交付；Windows 链接组件包含 `winhttp`、`ws2_32`、`bcrypt`。预编译模块使用稳定的内部类型符号。
4. 定向验证通过：编译器与标准库构建；原有本机 HTTP/WS 文本和二进制往返；HTTP/WS 超过 8 MiB 的文件流往返与 `00 ff 80` 首尾字节；HTTP/2 h2c 文本、同一连接上的两个顺序流及超过 8 MiB 的流式往返；HTTP/2 PEM/TLS 服务端与独立客户端的 ALPN `h2` 往返；WinHTTP HTTPS 客户端对公开站点的 HTTP/2 请求。11.4～11.6 补充了可复用会话、代理与解压、HTTP/1.1 并发连接、HTTP/2 与 HTTP/3 并行流、HTTP/3 证书拒绝/取消/显式回退，以及 HTTP/1.1/2 非法输入与限额的定向用例。h2c Upgrade、全量回归和性能基准未运行。

WebSocket 连接可作为 `Send` 值唯一移动到 `async def` 或 `task.spawn` 的有界工作线程，在 `task.scope` 内等待结果；它不满足 `Sync`，不得复制后并发收发。客户端 WebSocket 的单次读取超时通过等待工作线程实现；超时会关闭该连接，随后对同一句柄继续收发会得到 `connection_closed`。服务端同步处理函数仍在调用线程执行。

### 11.7 定向验证（2026-09-28）

Windows x64 增量构建及完整标准库打包通过，成功后清理 `build/`。`scripts/check_ws_11_7.py` 编译并运行四个定向 TX 程序：普通 HTTP 与 WebSocket 共用一个明文监听器完成 Upgrade、ping/pong、合法空消息和关闭原因；临时 CA 签发的证书完成 WSS 服务端握手与文本往返；`async def` 在有界任务作用域内接收和发送；TX WinHTTP 客户端读取对端关闭码与原因。脚本输出 `WS_11_7_OK`。此项的异步收发使用现有结构化任务工作线程和唯一移动的连接句柄；没有另设 WebSocket 专用异步函数。
