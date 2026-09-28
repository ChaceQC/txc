# `httpx` 可复用会话与分块传输（11.4）

`httpx.open_session(proxy_url, max_connections, decompress)` 建立一个客户端会话。Windows 实现持有同一 WinHTTP session 句柄，系统按源站复用空闲 TCP/TLS 连接；`max_connections` 为每主机连接数上限，取 1～64。会话可跨多个顺序或并发请求使用，但请求句柄的各阶段由调用方串行推进。`close_session` 后不能再开始请求；已开始的请求持有会话到自身结束。请求读取到 EOF 时释放请求及连接句柄，`close_request` 可提前终止传输且可重复调用。网络资源编号只在当前进程有效。

`proxy_url=""` 使用系统代理，`"direct"` 直连，`"http://host:port"` 指定 HTTP 代理。HTTPS 目的站通过 WinHTTP 建立 CONNECT 隧道并验证源站证书。代理 URL 不接受凭据、路径或片段；代理认证尚无公开接口。`decompress=true` 启用 WinHTTP 对 gzip/deflate 的自动解压；返回的块为解压后的字节，`max_response_bytes` 约束实际交付量。解压比例限制为编码字节的 100 倍加 1024 字节余量，通过 WinHTTP 请求统计核对；无法读取统计时当前请求以 `operation_failed` 关闭。调用方按需把块解释为 UTF-8。旧 `send/get/post` 与显式 `send_http2/get_http2/post_http2` 不借用新会话，返回类型和选协议规则保持原样。

```tx
httpx.client_session session = httpx.open_session("direct", 4, true)
httpx.client_request call = httpx.begin_request(session, "POST",
    "https://example.com/upload", map<str, str>(), 3, 1048576, 5000, false)
httpx.write_request(call, bytes.from_hex("010203"))
httpx.response_head head = httpx.finish_request(call)
bool eof = false
while !eof
{
    httpx.response_chunk chunk = httpx.read_response_chunk(call, 16384)
    eof = chunk.eof
    if !eof
    {
        # 处理 chunk.data；合法空正文由首次 eof=true 表示。
    }
}
httpx.close_request(call)
httpx.close_session(session)
```

`begin_request` 的 `body_length` 固定上传总长度，取 0～`4 GiB - 1`；`write_request` 每次最多接收 16 MiB，内部以 16 KiB 块推进，累计字节不得超过声明长度。`finish_request` 只有在完整上传后才提交并返回状态、头和独立的 `Set-Cookie` 列表。`read_response_chunk` 每次请求 1～16384 字节，允许短读；`eof=true` 只表示正文结束。若响应超出 `max_response_bytes`，抛出 `io_error/size_limit` 并关闭当前请求连接；此前已交付的数据不能撤销。`close_request` 可提前结束未被其他线程操作的请求；并发中的阻塞 WinHTTP 调用仍按它自己的超时结束。未调用 `finish_request` 就读取、重复提交及关闭后的操作均报错。

`require_http2=true` 只接受 HTTPS，WinHTTP 必须协商 `h2` 才交付响应；明文 h2c 仍走原有 `send_http2*`。`false` 允许 WinHTTP 在 HTTPS 上协商 HTTP/2 或 HTTP/1.1。所有新请求禁止自动重定向，不会跨源转发凭据；`Host`、`Content-Length`、`Transfer-Encoding` 和连接管理头由运行时管理。会话的代理、压缩和连接数在创建时固定。分块接口是同步的，单次请求的连接、发送、读取超时为同一个 `timeout_ms`，总截止时间和取消由后续网络工作项统一补齐。WinHTTP 自带 Cookie 状态已禁用；客户端需要显式提供 `Cookie` 头，`requests` 使用自己的 jar。

11.8 增加 `open_secure_session(proxy_url, max_connections, decompress, anchors, include_system, client_package, client_password, allow_http2)`，供 `requests` 在池创建时固定 TLS 信任锚、PKCS#12 客户端身份和协议选择。`anchors` 是 DER 证书向量，`client_password` 为 `secret_bytes`；未配置锚和身份时仍可用于普通 HTTP。自定义锚只放宽 WinHTTP 的未知根预检，收到响应头前再用仓库的 X.509 链引擎严格校验证书链、用途、有效期和主机名。Windows WinHTTP 访问客户端私钥需要用户密钥提供者，因此身份导入会创建临时用户密钥容器，池释放时删除；进程异常终止可能留下容器。`requests` 使用该入口时默认关闭自动解压及 HTTP/2，保留旧默认行为；显式 `http2=true` 才启用并要求 ALPN `h2`。

## 验证边界

Windows x64 增量构建通过。`http_session_server/client.tx` 验证本机连续请求、分块上传与读取、合法空正文、EOF 和关闭后状态；`scripts/check_http_session_proxy.py` 使用本机显式 HTTP 代理交付 gzip 正文，验证正常解压和接收上限；11.9 的高压缩比拒绝见[网络边界专项](network_11_9.md)。原有 `http_binary_server/client.tx` 在修正 TX 回调环境参数和句柄所有权后重新通过。Windows 以外尚无 WinHTTP 实现；跨公网代理认证和 HTTP/2 h2c 会话池仍未覆盖。
