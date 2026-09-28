# HTTP 服务端路由与并发（11.5）

`httpx.route_rule` 和 `binary_route_rule` 把方法、路径模式和类型明确的 TX 处理函数组成路由表。`serve_routes` / `serve_routes_bytes` 每次接收一条请求，按表中顺序匹配并调用首个命中的处理函数；未命中返回 404 与空正文。文本处理函数为 `fn(httpx.request, httpx.route_context) -> httpx.response`，二进制处理函数使用对应的 `binary_request/binary_response`。`route_context.params` 是路径参数映射。处理函数在调用 `serve_routes` 的 TX 线程/任务上下文执行，运行时网络层不在外部 I/O 线程直接调用 TX 函数。异常沿原错误链返回并关闭该请求连接或流。

路径模式以 `/` 开头，支持完整段 `{name}`，例如 `/items/{id}`；`*` 方法匹配任意合法方法。目标的查询字符串不参与路径匹配，参数值保留原始 URL 编码，供应用自己决定解码策略。尾斜杠有意义，路由段数最多 64、参数最多 16、模式和目标各最多 8192 字节；参数名遵循 HTTP token 规则。`match_route` 可供显式 `accept` 流程复用，返回 `matched` 与 `params`，没有用空映射代替未命中。

```tx
def item(request: httpx.request,
         context: httpx.route_context) -> httpx.response
{
    return httpx.response(200, map<str, str>(), vector<str>(),
        context.params["id"])
}

vector<httpx.route_rule> routes = vector<httpx.route_rule>(
    [httpx.route_rule("GET", "/items/{id}", item)])
httpx.serve_routes(server, routes, 10000)
```

同一 `httpx.listener` 可供多个 TX 工作线程或任务调用 `accept` / `serve_routes`。`listen` 默认最多同时持有 32 个 HTTP/1.1 请求连接；`listen_with_limit` 可设 1～64。额外的接受者等待容量，由内核监听队列向新连接施加背压；名额在响应或关闭后释放。监听器的查找、容量计数和接受过程受同步保护；`close` 唤醒等待容量的调用。需要真正并发处理时，应用在有界任务作用域或线程中发起多个 `serve_routes`，各自创建路由表；静态 `Send/Sync` 规则允许共享该监听器，但连接与请求句柄仍按单次请求使用。

HTTP/2 h2c 和 TLS 监听器允许同一连接最多 16 条并发流，并同时跟踪最多 16 个 TCP/TLS 会话；每条流有独立的头和正文上限。nghttp2 承担帧、HPACK 与流控；会话上的读写由锁串行化，两个已接收的流可在不同 TX 工作线程并行调用处理函数。内存接口的请求正文上限为 8 MiB，头上限 64 KiB；超过上限的 `Content-Length` 在读取正文前拒绝。流式 `accept_stream` 在单流场景可接收调用方设定的更大上限；多个同时到达的流在绑定各自目标流前最多缓存 8 MiB，超出时拒绝该会话。服务端不提供无限制的每连接新线程或自动重试。

## 定向验证（2026-09-28）

Windows x64 增量构建通过。`scripts/check_http_parallel.py` 用两个 TX 工作线程处理同一 HTTP/1.1 监听器的两个并发连接，并以 Python `h2` 在一条 h2c 连接上同时打开两条流。每个处理函数等待 500 毫秒，实测两个请求分别在约 0.58 秒、0.53 秒完成，正文和路径参数一致。原有超过 8 MiB 的 `http2_stream_server/client.tx` 往返再次通过。`scripts/check_http_protocol_limits.py` 覆盖 HTTP/1.1 缺 Host、超大头、超大声明正文，以及 HTTP/2 错误前言、超大头和超大声明正文；均由预期错误路径拒绝。验证未覆盖 TLS 并发连接下的慢握手、长时间公平性或第 11.9 项的全部恶意输入与资源采样。
