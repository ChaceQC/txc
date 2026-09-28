# 网络边界专项验收（11.9）

本项在 Windows x64 上以本机临时服务和 TX 程序验证七类边界。程序与脚本位于 `tests/network/network_11_9_*.tx`、`scripts/check_network_11_9.py`，已有协议、TLS 和 HTTP/3 专项脚本复用各自测试入口。

| 小点 | 行为与证据 |
| --- | --- |
| 11.9a 证书错误 | `check_requests_11_8_tls.py` 用临时 CA 验证可信双向 TLS；系统信任拒绝未知根，自定义错误根、错误主机名、缺少客户端身份均报 `security_error`，`verify=false` 明确拒绝。证书内容与密码不进入诊断。 |
| 11.9b 慢连接 | HTTP/1.1 服务端从接受连接后对完整请求头和正文共用单调时钟总时限。逐字节输入持续超过 650 ms 时，`accept` 报 `timeout`；同一监听器随后仍能回复正常请求。超时为 `0` 时保持无限等待语义。 |
| 11.9c 重定向凭据 | 同源跳转保留显式 `Authorization` 和 `Cookie`；跨源跳转删除原始认证头、Basic 认证和显式 Cookie。HTTPS 跳转到 HTTP 时也删除这些值，并不把 CA 路径或客户端身份作为 HTTP 传输配置。命名参数 `headers` 与字符串 `verify` 组合、等价的 `request_options` 路径均完成本机验证。会话 Cookie 仍按目标 URL 的域、路径和 Secure 规则重新选择。WinHTTP 自动跳转和自带 Cookie 状态均保持禁用。 |
| 11.9d 压缩与正文 | `httpx.open_session(..., decompress=true)` 对 WinHTTP 解压后的正文按 `max_response_bytes` 限制；压缩比例上限为编码字节的 100 倍加 1024 字节余量。WinHTTP 自动解压会移除原始 `Content-Length`，因此每次读取后通过 `WINHTTP_OPTION_REQUEST_STATS` 的压缩字节计数核对；统计不可用时以 `operation_failed` 关闭当前请求，不无约束地交付正文。高压缩比 gzip 和较小正文上限分别触发 `size_limit`。`requests` 默认不启用自动解压。 |
| 11.9e 非法输入 | `check_http_protocol_limits.py` 核对 HTTP/1.1 缺 Host、超大头和声明超大正文的 400/431/413；HTTP/2 非法前言、流 0 的 HEADERS 帧、超大头和声明超大正文由 `protocol_error` 或 `size_limit` 拒绝。nghttp2 的非法帧回调现在进入同一协议错误路径。 |
| 11.9f 取消状态 | 活动 TCP 异步读取取消后，原流仍可同步读取随后到达的数据；显式关闭后再读报 `connection_closed`。`check_http3_cancel.py` 用丢弃 UDP 握手包的本机端口验证活动 HTTP/3 握手及时交付 `cancelled`。 |
| 11.9g 大文件流 | 12 MiB 的 `00 ff 80` 二进制模式以 `requests.download/upload` 分块传输；下载核对文件大小及首尾字节，服务端逐块计算上传 SHA-256 与原始数据比较。调用方持有文件流并负责关闭；传输中途失败可留下部分目标文件。 |

## 验证命令与边界

构建使用 `scripts/build.ps1 -Incremental`，随后运行 `scripts/check_network_11_9.py`、`scripts/check_http_protocol_limits.py`、`scripts/check_requests_11_8_tls.py`、`scripts/check_http3_cancel.py` 和 `scripts/check_http_session_proxy.py`。上述专项均通过；正常 gzip 响应由 `check_http_session_proxy.py` 覆盖，高压缩比拒绝由 11.9 脚本覆盖。`scripts/check_http_parallel.py` 还复核了 HTTP/1.1 和 HTTP/2 正常并发请求。此记录只确认 Windows x64 本机环境；跨平台 WinHTTP 替代实现、跨网络 QUIC 迁移、长期资源上限、代理认证及全库回归仍按各自后续范围处理。
