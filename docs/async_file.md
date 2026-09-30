# 异步文件操作

Linux 后端在有界任务工作池执行普通文件的 `pread/pwrite`，按最多 64 KiB 块处理，避免阻塞任务提交线程；文件描述符不共享游标。每块之前检查作用域和显式令牌的取消，返回已经完成的实际前缀。正在内核中执行的单次磁盘操作不能强制中断，因此取消响应还受底层文件系统影响。只接受普通文件，FIFO、设备和目录返回 `file_open_failed`，避免无界设备等待。Linux 的异步定时器使用独立条件变量事件循环，不占用文件 I/O 工作者；作用域取消会唤醒该循环。

`async_file.txh` 的 `read_at(path, offset, max_bytes, token)` 和 `write_at(path, offset, data, token)` 只能在活动的 `task.scope` 内调用，分别返回 `task<read_result>` 与 `task<write_result>`。一次操作最多传输 16 MiB；`offset` 为非负字节偏移，`max_bytes` 为 0～16 MiB。路径使用 UTF-8，Windows 运行时转换为宽字符并使用 `FILE_FLAG_OVERLAPPED` 与任务 IOCP 完成循环。已有同步 `file` 与 `file_stream` 接口不变。网络和数据库的适配分别由第 11、12 节实现。

`read_result` 包含 `data: bytes`、`eof: bool`、`cancelled: bool`、`error_code: str`。短读返回实际字节；到达 EOF 时 `eof=true`。`write_result` 包含 `written: int`、`cancelled: bool`、`created: bool`、`error_code: str`。写入从指定偏移开始，不截断旧文件；目标不存在时创建，`created` 在取消后仍准确报告是否创建了文件。取消或 I/O 失败后，`written` 是内核完成通知中的已写字节数，调用方只能按该值和实际文件内容决定重试，不能假定回滚。

两种结果的 `error_code` 为空表示没有 I/O 错误；非空表示打开、关联或读写失败，不能把它当作 EOF。`cancelled=true` 表示取消先于结果提交，已完成的字节仍在结果中；完成先于取消时返回正常结果。任务作用域的取消和显式 `cancel_token` 都会请求 `CancelIoEx`，完成通知是最终判定点。每次调用的文件句柄在结果提交前关闭，函数不返回可继续使用的句柄。调用前参数错误在源码位置抛出，未启动 I/O，不创建文件。
