# Linux 网络实现

Linux x86_64 与 Windows 共用 `.txh` 网络接口、错误码和服务器协议实现。平台差异由原生 socket 适配以及客户端后端处理，不需要修改 TX 源码中的导入或函数名称。

| 模块 | Linux 实现 | 保留的约束 |
| --- | --- | --- |
| TCP / UDP / 异步 socket | POSIX socket、非阻塞 I/O、poll | 超时、取消、半关闭、EOF、UDP 来源地址和截断标志 |
| DNS | c-ares 异步 getaddrinfo | A/AAAA、TTL、hosts 与系统 DNS 配置、最多 128 个去重地址、超时与取消 |
| HTTP/1.1 / HTTP/2 客户端 | libcurl multi + OpenSSL；明文 h2c 复用 nghttp2 | 连接复用、每主机连接上限、独立 Set-Cookie、流式上传下载、禁止自动重定向、响应大小和解压比例限制 |
| HTTP/1.1 / HTTP/2 服务端 | 共享协议代码、nghttp2、mbedTLS | 复用现有解析、头字段限制和 TLS 逻辑 |
| WebSocket 客户端 / 服务端 | 共享 RFC 6455 帧实现；WSS 使用 mbedTLS | 客户端独立随机掩码、分片、UTF-8 校验、ping/pong、关闭码与流式二进制消息 |

`httpx.open_session` / `open_secure_session` 的代理参数保持三种语义：`"direct"` 强制直连，空字符串采用平台默认代理设置，`http://主机:端口` 使用显式 HTTP 代理。Linux 的默认代理来自 libcurl 支持的环境变量，例如 `http_proxy`、`https_proxy` 和 `no_proxy`；不读取 Windows IE/PAC 配置。WebSocket Linux 客户端直接连接目标，WSS 通过系统信任库验证证书；WebSocket 接口本身没有代理参数。

HTTP 上传由 libcurl 回调按调用方提供的块推进，下载在调用方读取前暂停。实现不会为了模拟流式接口把完整正文读入内存或写入临时文件。`finish_request` 交付响应头，正文限额错误在读取正文时交付。解压后的总量遵循 `max_response_bytes`，并检查实际接收编码字节的 100 倍加 1024 字节容差限制。超时用于一次网络等待，调用方在分块调用之间的停顿不计入等待时间。

底层 libcurl Cookie jar 保持禁用，`httpx` 仅发送调用方显式的 Cookie 头；`requests` 继续独立管理 Domain、Path、过期和删除规则。关闭会话会移除新请求入口，已开始的请求继续持有底层会话直到自身结束。

自定义 CA 在 OpenSSL 握手过程中加入信任库，`include_system=false` 使用独立信任库。客户端 PKCS#12 身份沿用 TLS 身份接口，证书链与私钥只在内存中交给 TLS 后端，不能记录或输出密钥。Linux 的 libcurl 必须使用 OpenSSL 后端；HTTP/2 支持由其构建特性提供。

POSIX 发送使用 `MSG_NOSIGNAL`，连接关闭产生可捕获的错误而不会结束 TX 进程。监听、同步等待和异步分派使用 poll，避免 Linux `fd_set` 的文件描述符范围限制。新建和接受的 socket 设置 close-on-exec，防止继承到子进程。

## 定向验证

`scripts/check_linux_network.py` 使用 `TXC_TOOL_DIR` 指定的工具包，执行已有 DNS 和 socket 行为用例，以及回环 HTTP 会话和 WebSocket 客户端用例。HTTP 检查连接复用、Cookie、连续分块上传、chunked 下载、解压大小限制和读取超时；WebSocket 检查客户端掩码、文本分片中的 ping/pong、二进制收发和关闭握手。`scripts/check_http_session_proxy.py` 继续检查显式代理与解压限额；`scripts/check_ws_11_7.py` 检查共享 Upgrade、WSS 与任务接口。

本文描述已实现路径。Linux 全部依赖链接、端到端测试和 GitHub 工作流结果应以当前执行日志为准，不能把语法检查视为远端 CI 或生产网络环境验收。
