# 网络模块设计：`httpx` 与 `ws`

本文是下一阶段的接口与实现设计，**网络模块尚未实现**。先确定 TX 可见的类型、调用流程、失败语义和实施边界，再增加 `.txh`、C++23 标准库实现及 C ABI。设计基于当前 Windows 工具链、已有 `error.io_error`、类型化 `map<str, str>` 和[函数参数 `fn`](function_values.md)，不修改语言语法。

## 目标与边界

- `httpx` 同时覆盖 HTTP 客户端和服务端；`ws` 同时覆盖 WebSocket 客户端和服务端。客户端支持 HTTPS/WSS，服务端第一阶段监听 HTTP/WS 明文连接。服务端 TLS 需要证书加载和 Schannel 生命周期契约，后续单独补充。
- 同步接口由调用方控制超时和循环。服务端既能显式 `accept`、处理、`respond`，也能把不同的处理函数传给同一个 `serve_once`。回调类型在模块实现中确定，调用时仍由编译器按函数签名检查；不按字符串在运行时查找函数。
- 正文和 WebSocket 消息第一阶段都是有效 UTF-8 文本。任意二进制正文、二进制帧、流式上传下载和文件传输按[字节值与文件流设计](bytes_file_stream.md)后续接入，不把任意字节伪装成 `str`。
- 网络状态是进程内资源。`listener`、`connection` 为带内部编号的结构体，必须显式关闭；重复关闭无害。句柄不允许序列化后跨进程使用。进程退出时运行时兜底释放仍存活的连接。

## 公共类型

建议两个模块分别定义自己的 `listener` 和 `connection`，避免 HTTP 与 WS 句柄混用。字段 `id` 仅供运行时识别，不作为可持久化或可运算的网络地址。

```tx
# httpx.txh 中的类型草案
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

`request.target` 保存路径与查询字符串，例如 `/items?page=2`；服务端不自动进行 URL 解码。请求和响应头使用 `map<str, str>`，名称按 ASCII 大小写不敏感规则规范化为小写。普通重复字段按 HTTP 允许的规则合并；不能安全合并的 `Set-Cookie` 单独放在 `response.set_cookie` 中，按出现顺序保留。其他不能安全合并的重复字段应报告 `io_error` / `protocol_error`，不能静默丢弃。`response.status` 限 100～599。请求正文与响应正文均为 UTF-8 `str`；合法空正文与传输失败有区别。

## `httpx` 客户端

| 建议签名 | 行为 |
| --- | --- |
| `send(method: str, url: str, headers: map<str, str>, body: str, timeout_ms: int) -> response` | 发送完整请求并读取完整响应 |
| `get(url: str, timeout_ms: int) -> response` | 无自定义请求头的 GET 便捷接口 |
| `post(url: str, body: str, timeout_ms: int) -> response` | UTF-8 文本 POST 便捷接口 |

URL 只接受 `http://` 和 `https://`，必须包含主机，允许显式端口、路径和查询。方法名须符合 HTTP token 语法；头名称和值拒绝 CR、LF、NUL，防止注入。客户端自动计算 `Content-Length`，调用方不能覆盖 `Host`、`Content-Length`、`Transfer-Encoding` 或连接管理字段。代理沿用 Windows 系统/WinHTTP 配置，HTTPS 证书按系统信任链和主机名校验，不提供跳过证书校验的参数。

状态码 4xx/5xx 是正常的 HTTP 响应，仍返回 `response`；DNS、连接、TLS、超时、协议和 UTF-8 失败才进入 `io_error`。第一阶段不自动跟随重定向，调用方可检查 3xx 与 `Location`，避免在跨站跳转时意外转发认证头。响应头和正文在返回前完整读取，并受下述大小限制。

## `httpx` 服务端

| 建议签名 | 行为 |
| --- | --- |
| `listen(host: str, port: int) -> listener` | 绑定本机地址并开始监听 |
| `accept(server: listener, timeout_ms: int) -> request` | 接收并解析一个完整请求 |
| `respond(peer: connection, value: response) -> void` | 写出响应并结束该请求连接 |
| `serve_once(server: listener, handler: fn, timeout_ms: int) -> void` | 接收一个请求，调用处理函数并回复 |
| `close(server: listener) -> void`、`close(peer: connection) -> void` | 释放监听或连接资源 |

`handler: fn` 在 `serve_once` 的 TX 实现中调用为 `handler(request)`，其返回值明确为 `httpx.response`，因此推断出的签名是 `fn(httpx.request) -> httpx.response`。用户可在同一个 `serve_once` 上传入任意多个符合此签名的普通函数，例如静态路由、JSON 路由或健康检查处理函数。`serve_once` 只处理一个请求，服务循环由 TX 程序编写；处理函数抛错时关闭本次连接并向调用方传播原错误。

显式路径适合需要先检查 `request.method`、`request.target` 再决定处理方式的程序。`respond` 自动写入 `Content-Length` 与 `Connection: close`，成功后关闭 `peer`；若决定不响应，调用 `close(peer)`。关闭监听器不强行关闭已经接收的连接。第一阶段只接受 HTTP/1.1 且有明确 `Content-Length` 的文本正文；不支持 chunked 上传、持久连接、HTTP/2 或 HTTP/3。非法请求需要返回明确的 400/413/431 等状态后关闭连接，内部 I/O 失败仍报告 `io_error`。

`host` 可用 `127.0.0.1`、`::1` 或本机绑定地址；空字符串表示监听所有本机地址。端口限 1～65535；不自动开放 Windows 防火墙。服务端启动默认不跨线程调度处理函数，同步 `accept` 或 `serve_once` 在调用线程运行。后续若要并发，需先确定 TX 的线程、共享对象及回调执行约束。

## `ws` 客户端与服务端

```tx
# ws.txh 中的类型草案
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

| 建议签名 | 行为 |
| --- | --- |
| `connect(url: str, timeout_ms: int) -> connection` | 连接 `ws://` 或 `wss://` 服务端并完成握手 |
| `listen(host: str, port: int) -> listener` | 监听明文 WS 连接 |
| `accept(server: listener, timeout_ms: int) -> connection` | 接受一个连接并完成 WebSocket 握手 |
| `send_text(peer: connection, text: str) -> void` | 发送一条完整文本消息 |
| `receive(peer: connection, timeout_ms: int) -> message` | 读取一条完整文本消息 |
| `reply_once(peer: connection, handler: fn, timeout_ms: int) -> bool` | 读一条消息、调用处理函数、发送返回文本 |
| `close(server: listener) -> void`、`close(peer: connection) -> void` | 关闭监听器或完成关闭握手并释放连接 |

`reply_once` 的 TX 实现把收到的 `message.text` 传给 `handler`，要求其返回 `str`，因此推断签名是 `fn(str) -> str`。不同处理函数可以复用同一连接和同一 `reply_once`。返回 `false` 表示对端正常关闭，且不会调用处理函数；合法空文本消息仍返回 `true` 并调用处理函数。显式 `receive` 用 `message.open=false` 区分正常关闭与合法空字符串。关闭后的连接再次收发应产生明确错误。

WebSocket 遵循 RFC 6455：校验握手、客户端掩码、帧长度及控制帧限制；接收分片文本消息时先合并再返回；收到 ping 自动回 pong；正常 close 返回 `open=false`。服务端发送不带掩码，客户端发送带掩码。二进制帧第一阶段报告 `io_error` / `unsupported_frame`，不以文本解码替代。WS 与普通 HTTP 服务端可以分别监听端口；同端口路由升级需要进一步确定共享监听器和连接移交契约，本阶段不假定已支持。

## 超时、大小限制和错误

- 客户端 `timeout_ms` 必须大于零，分别约束连接、发送和读取阶段；服务端等待类接口允许 `0` 表示无限等待，其余值须为正。超时不会返回空字符串或正常关闭标记，而是 `error.io_error` / `timeout`。
- 请求头与响应头各限 64 KiB，单个 HTTP 正文和 WebSocket 文本消息各限 8 MiB；超过限制用 `size_limit`，避免无限增长的内存缓冲。
- 输入校验、协议、连接状态和系统错误归为 `error.io_error`，`code` 计划使用 `invalid_url`、`invalid_header`、`invalid_argument`、`invalid_utf8`、`timeout`、`connection_closed`、`protocol_error`、`unsupported_frame`、`size_limit`、`operation_failed`。`message` 提供中文解释，但程序分支应依据 `code`。需要重试或继续服务时使用已有 `try { } exception errors.io_error as e { }`。
- HTTP/WS 对不可信网络输入不应触发编译器崩溃；每个失败路径须关闭部分建立的 socket、请求句柄和监听资源。同步服务端不保证处理函数执行期间的并发服务能力。

## 建议实现分层与交付顺序

1. 在 `tx/stdlib/` 写公开 `.txh` 契约与配对的 TX 包装实现，先让 `serve_once`、`reply_once` 的 `fn` 调用留在 TX 层，保持回调签名在编译期确定。
2. 在 `src/stdlib/` 实现 Windows 网络客户端和服务端核心：WinHTTP 处理客户端 HTTP/HTTPS 与 WS/WSS；Winsock 处理 HTTP/WS 监听、协议解析和资源生命周期。共用 URL、UTF-8、头字段与大小限制校验，但不把 HTTP 和 WS 的协议状态混在同一对象。
3. 在 `src/backend/cpp/` 增加明确的 C ABI 入口，在 LLVM 后端声明并静态调用；更新构建脚本所需的 Windows 链接组件。连接、握手和消息状态在 C++ 对象中保存，不通过运行时名称分派选择目标函数。
4. 实施时做少量定向验证：本机 HTTP 请求/响应与两种处理函数、4xx 响应、HTTPS 证书错误；WS 握手、文本、空消息、ping/close、分片与非法帧；超时和大小限制。全量回归与性能基准另行安排。

本轮仅交付设计文档；上述接口均为待实现契约，不能当作当前标准库能力使用。
