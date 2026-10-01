# 本机 IPC 与版本化 CBOR 消息

Linux 后端使用非阻塞 Unix domain stream socket，监听名映射到当前有效 UID 下的抽象命名空间；客户端和服务端均检查 `SO_PEERCRED` 的 UID。相同名称由不同用户各自监听，不创建或遗留磁盘 socket 文件。Windows 后端仍使用本机命名管道。两端共用下文的 TXIP/CBOR 帧、取消、超时和半包恢复逻辑；进程标准流适配使用 POSIX 管道。

`ipc.txh` 提供 `listen/accept/connect` 本机命名管道，以及 `from_process_pipes(reader, writer, max_bytes)` 把 `process.stdout_pipe` 和 `process.stdin_pipe` 作为同一双向消息流。管道名只接受 1～64 个 ASCII 字母、数字、下划线和连字符；Windows 服务端使用拒绝远程客户端的字节管道。`ipc_stream` 和 `ipc_listener` 是不透明的非 `Send` 句柄；关闭流也关闭被适配的进程管道别名。`max_bytes` 须为 1～16 MiB，限制单个 CBOR 载荷。

每条消息为 20 字节定长头 `TXIP`、小端 32 位版本号、小端 64 位发送方会话号、小端 32 位载荷长度，后接规范 CBOR。`send(stream, version, value, timeout_ms, token)` 在写入前编码并检查限额；版本为 1～4294967295。`recv` 解码并返回 `message`，其中 `value` 是动态 `any`，调用方应按已协商版本显式恢复类型。调用方重连后可比较 `session` 来辨认新进程；同一流内发送方会话号变化时返回 `restarted` 并仍交付该条消息。

`message.state` 为 `data/restarted/eof/incomplete/too_large/timeout/cancelled/invalid_frame/invalid_cbor/error/closed`。只有在完整帧边界读到对端关闭才是 `eof`；头或载荷中途关闭是 `incomplete`。`received` 是当前帧已读字节数；超时和取消保留缓冲，下一次 `recv` 从半包继续。超限或不完整帧使流关闭，不能把剩余字节误认作下一帧。`send_result.state` 为 `sent/timeout/cancelled/closed/partial/error`，`written` 计入帧头和载荷；超时或取消后的部分写入会关闭流，调用方须重新连接。`error_code` 给出稳定的非空传输错误码。所有阻塞点都检查取消令牌和单调超时；`timeout_ms=-1` 表示无限等待。`close` 可重复调用，返回是否首次关闭。

把进程管道交给 `from_process_pipes` 后，不应再通过 `process.read_pipe/write_pipe` 并行读写相同句柄；两套读取路径会竞争同一帧字节。会话号仅用于诊断重启，不是身份认证凭据。

定向复核时，先用 `g++ -std=c++23 -O2 -municode tests/stdlib/ipc_protocol_helper.cpp -o tx_build/ipc_protocol_helper.exe` 生成原生对端，再由 `txc` 编译运行 `tests/stdlib/ipc_named_behavior.tx`、`ipc_process_behavior.tx` 和 `ipc_boundaries.tx`。原生对端用于构造半包、超限帧与会话号变化；测试程序不会写入仓库跟踪文件。
