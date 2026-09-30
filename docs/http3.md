# HTTP/3 与 QUIC（11.6）

`httpx` 使用 MsQuic 的 QUIC 传输和 nghttp3 1.18.0 的 HTTP/3 帧与 QPACK；Windows x64 使用 Schannel 后端，Linux x64 使用 OpenSSL 后端，编译接口固定为 MsQuic 2.6.1。客户端显式函数 `send_http3/get_http3/post_http3` 及对应 `_bytes` 版本只接受 `https://`，必须完成 TLS 1.3 与 ALPN `h3`，失败不自动改用 HTTP/2。HTTP/3 当前通过 UDP 直连，不使用系统 HTTP 代理；代理或 UDP 网络不通时可由调用方选择显式回退。原有 `send_http2/get_http2/post_http2` 仍强制 HTTP/2；原有 `send/get/post` 使用对应平台的 HTTP 客户端。

`send_http3_with_trust` / `_bytes` 接受非空 DER 信任锚列表，使用已有 X.509 验证器检查链、`server_auth` 用途、有效期和 URL 主机名，再完成 MsQuic 的延迟证书验证。普通 HTTP/3 函数使用操作系统信任与主机名验证，Linux 的系统根来自 OpenSSL 默认 CA 配置。Linux 自定义信任回调使用可移植 DER/PKCS#7 证书快照，不依赖 MsQuic 内部 OpenSSL 对象的二进制布局。验证完成前不提交应用请求；没有关闭验证的参数。自定义锚最多 64 张，单张格式在建立连接前校验。`send_http3_controlled` / `_bytes` 再接收 `cancel_token`；锚列表为空时用系统信任。取消令牌及其截止时间在握手、流启动和等响应期间检查，取消后静默关闭该次 QUIC 连接，按 `cancelled_error/cancelled` 或 `deadline_exceeded` 报错。

服务端 `listen_h3(host, port, package, password)` 使用 PKCS#12 `bytes` 和 `secret_bytes` 密码。`accept`、`accept_bytes`、`respond`、`respond_bytes`、路由处理和 `close` 与现有 `httpx.listener`/`connection` 契约相同；多条流可在不同 TX 工作线程处理。Schannel 要求能重新打开证书私钥，导入的用户 CNG 密钥只保留到监听资源及已接受连接结束，并在资源释放时删除。正常关闭的定向检查未发现用户 CNG 密钥文件数量增长；进程被强制终止时无法执行清理，可能需要由管理员清理残留密钥容器。当前接口尚不能直接引用证书存储中的既有身份。此项不把 PFX 密码或密钥内容写入错误、日志或生成源码。

Linux 服务端先检查 PKCS#12 的证书、私钥数量和匹配关系，再使用 MsQuic 的内存 PKCS#12 凭据同步加载身份；不创建临时私钥文件或持久化容器。临时密码在加载完成后清零，连接持有的凭据在配置释放时销毁。Linux 动态加载 `libmsquic.so.2`（兼容未版本化的 `libmsquic.so`），发布包提供对应运行库；缺少运行库与 Windows 一样报告 `unsupported_protocol`。

关闭监听器会清理未交付的请求。已交付但尚未回复的请求会保活其连接和监听配置，因此从监听器移除连接记录后仍可完成回复；回复或显式关闭请求句柄后释放保活引用。对端中止请求流时，服务端移除尚未交付的请求；已交付句柄会以连接关闭结果结束。已经执行的外部效果无法撤销，调用方不应因此自动重试非幂等操作。请求流结束后单独回收对应的 QUIC 与 nghttp3 状态；控制和 QPACK 流随连接关闭。

## 显式协议策略

`send_negotiated` 和 `send_negotiated_bytes` 返回 `protocol`、状态、头、Cookie 与正文。`protocol` 为实际使用的 `h3`、`h2` 或 `http/1.1`。策略如下：

| `policy` | 行为 |
| --- | --- |
| `h3_only` | 仅 HTTP/3，不降级 |
| `h3_then_h2` | HTTP/3 因超时或不支持而失败时，改用强制 HTTP/2 |
| `h3_then_system` | 同样的失败条件下，改用平台 HTTP 客户端协商 HTTP/2 或 HTTP/1.1：Windows 为 WinHTTP，Linux 为 libcurl/OpenSSL |
| `h2_only` | 仅 HTTP/2，沿用原有 HTTPS ALPN 或明文 h2c 语义 |
| `system` | 平台 HTTP 客户端自动选择 HTTP/2 或 HTTP/1.1 |

前 3 种策略要求 HTTPS。可降级的两种策略只接受无正文的 `GET`、`HEAD`、`OPTIONS`，在发出请求前拒绝其他方法为 `io_error/replay_unsafe`，避免上传或非幂等操作被重放。证书失败、非法 HTTP/3 帧、无效输入和已收到的 HTTP 错误状态都不会触发降级。`timeout_ms` 限 1～60000；每次协议尝试各使用该上限，降级调用最多经历两次尝试。重定向始终由调用方处理，不跨源转发请求头。

## 握手、0-RTT、取消和迁移

- 握手：MsQuic 要求 ALPN `h3`，客户端确认可信链与 URL 主机名后才打开请求流。服务端证书和私钥在 Windows 经 Schannel、在 Linux 经 OpenSSL 提供。客户端每次显式 HTTP/3 调用建立一条 QUIC 连接，结束、取消或超时后释放；尚无 HTTP/3 客户端连接池。
- 0-RTT：不保存会话票据，不使用 0-RTT 流或发送标志；所有应用数据在本次握手完成后发送，避免默认重放风险。服务端不对早期数据建立处理入口。
- 取消：`send_http3_controlled*` 每 20 毫秒内检查令牌和令牌截止时间；超时、取消或错误使当次连接退出，不把未完成的响应当作空正文。已到达服务端的请求效果不能撤销，调用方不应自动重试非幂等操作。
- 迁移：客户端和服务端均启用 MsQuic 的路径迁移。迁移后连接和 HTTP/3 流编号保持不变，`httpx` 不公开手动切换本地地址或连接 ID 的接口；迁移失败按传输错误或超时返回，不自动重放请求。跨网络切换与 NAT 重绑定尚无本机专项注入证据。

## 限额与流行为

每条 QUIC 连接的服务端双向请求流上限为 16，单向控制/QPACK 流配置为 3；每个监听器最多同时接受 16 条连接，已完成待处理请求队列最多 64 条。HTTP/3 头限 64 KiB，单请求/响应的内存正文限 8 MiB，未交付的队列和数据不能无限积累。`Content-Length` 超限在读取正文前拒绝，正文实际长度仍逐块计数。服务端 `accept_stream/respond_stream` 复用接口，但当前 HTTP/3 适配在 8 MiB 内缓冲再写文件；超过 8 MiB 的真正网络分块传输应使用现有 HTTP/1.1 或 HTTP/2 路径。非法帧和超限请求会终止相应 QUIC 会话，错误以 `io_error` 报告，不以空结果代替。

## 依赖和定向验证（2026-09-28）

MsQuic Schannel NuGet 2.6.1 包 SHA-256 为 `cf09771561c16dc823454c212fbf3bc1307c0a77b9d98326f1faa256685e2a3f`；nghttp3 1.18.0 源码归档 SHA-256 为 `aad782c23d3f01bd4bb52c8bac7a553b631ef8115fd1612703df6183449fef19`。两者均为 MIT 许可证，固定校验后分别随包交付 `msquic.dll` 和静态归档，以及 `MSQUIC-LICENSE`、`NGHTTP3-LICENSE`。缺少 MsQuic 运行库时明确报 `unsupported_protocol`。

Windows x64 增量构建通过。`scripts/check_http3_server.py` 以临时证书验证独立 aioquic 客户端访问 TX 服务端、TX 客户端用自定义根证书访问该服务端、`00 ff 80` 等二进制正文往返，以及默认系统信任拒绝未知根；四条路径通过。`scripts/check_http3_parallel.py` 在一条 QUIC 连接上并行处理两条流，两个各等待 500 毫秒的处理函数约 0.51 秒完成。`scripts/check_http3_cancel.py` 让本机 UDP 端口丢弃握手包，活动请求在约 0.35 秒交付取消错误。`http_negotiation_client.tx` 的本机 `system` 和 `h2_only` 路径通过；一次公网 `h3_then_system` 在 UDP 握手超时后得到 `http/1.1` 响应。直接公网 HTTP/3 在当前网络未完成握手，因此互操作证据来自本机独立 aioquic 客户端。正常关闭前后用户 CNG 密钥文件数量均为 121；未运行全量回归或网络迁移故障注入。
