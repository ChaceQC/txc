# 子进程与管道

`process.txh` 在 Windows 上通过 `CreateProcessW` 启动程序，参数逐项传递，不调用 shell，不搜索 `PATH`，不执行 `.bat/.cmd`。`executable` 必须包含文件路径；相对路径相对于父进程当前目录解析，`cwd` 只改变子进程工作目录。中文与空格路径使用宽字符 API。命令行按 Windows CRT 的引号/反斜杠规则编码；自行解析原始命令行的程序须遵守它自己的约定。

## 7.5 启动与生命周期

### Linux 后端

Linux 使用 `posix_spawn`、`pipe2`、`poll` 与 `waitpid`，仍直接执行给定路径，不搜索 `PATH`，不隐式调用 shell。参数逐项传递给 `exec`，没有 Windows CRT 引号编码及 32767 UTF-16 单元限制，实际参数总量受系统 `ARG_MAX` 限制；环境变量名称区分大小写。只保留选择的三个标准流，空流使用 `/dev/null`。带有效 shebang 且有执行权限的脚本由内核加载。

Linux 正常退出码为 0～255；信号退出返回 `state="terminated"`、`exit_code=128+信号编号`。`terminate` 向直接子进程发送 `SIGTERM`；启用 `new_process_group` 时发给该组。`kill` 只向直接子进程发 `SIGKILL`。关闭句柄不会终止子进程，由回收线程等待退出，避免僵尸进程。父侧管道非阻塞，三路有界捕获和超时、取消、部分写入语义与下文相同。

`make_options(executable, args)` 返回可修改的 `options`：

| 字段 | 默认值与规则 |
| --- | --- |
| `executable: str`、`args: vector<str>` | 参数不含程序名；空参数、引号、尾部反斜杠原样传递。拒绝 NUL 和超过 32767 个 UTF-16 单元的命令行。 |
| `cwd: str` | 空串继承父目录；不会修改父进程目录。 |
| `env: vector<str>`、`clear_env: bool` | 默认空覆盖、继承环境。条目为 `NAME=value`，同名按 Windows 不区分大小写的规则后者覆盖前者；`NAME=` 设置空值。清空模式只传这些条目。不打印环境内容。 |
| `stdin_mode/stdout_mode/stderr_mode: str` | 默认均为 `pipe`；也支持 `inherit`、`null`。只向子进程继承选定的三个句柄。 |
| `new_process_group: bool` | 默认 `false`；`true` 创建独立控制台进程组，供显式 `terminate` 请求使用。 |

`spawn(options) -> process_child` 创建不透明句柄。赋值和 `any` 恢复共享状态；`deep_copy` 拒绝复制。进程及管道目前不满足 `Send/Sync`，不能把 TX 可变对象送入后台线程。实现使用系统异步 I/O，不依赖尚未完成的第 2.6 节。

- `id(child)` 返回进程 ID。
- `try_wait(child)` 非阻塞；`wait(child, timeout_ms)` 中 `-1` 无限等待，`0` 轮询，正数为毫秒。返回 `exit_status { state, exit_code }`：`running`、`timeout`、`exited` 或 `terminated`。只有后两者的 `exit_code` 有意义，是无符号 Windows 32 位退出值装入 TX `int`。非零正常退出仍是 `exited`；系统异常退出、控制台中断及本库强杀标为 `terminated`。Windows 无法可靠识别其他程序任意指定退出码的终止来源。
- 等待成功关闭原生进程句柄并缓存结果，重复等待返回缓存；输出管道仍可读完。超时不杀进程，仍可继续等待、读写或显式结束。
- `terminate(child)` 向独立进程组发送 `CTRL_BREAK_EVENT`，要求与父进程共享可用控制台；不支持时抛 `unsupported_operation`。这是协作请求，子进程可忽略，也可能影响同组后代。
- `kill(child)` 调用 `TerminateProcess` 强制结束直接子进程，不遍历进程树。两种请求后均须 `wait` 确认结束；对已完成进程无操作。
- `close(child)` 幂等，关闭父侧进程及管道句柄，所有别名失效；最后引用释放也会清理，但不杀仍在运行的进程。已取出的管道与 child 共用关闭状态。

严格接口可捕获 `process_error`：`invalid_argument`（参数/策略/限额错误）、`spawn_failed`（启动未交付）、`wait_failed`、`invalid_state`（已关闭或流策略不符）、`unsupported_operation`、`terminate_failed`、`pipe_failed`。系统错误只记录操作名称与错误号，不包含命令参数、环境和管道正文。`wait_with_cancel(child, timeout_ms, token)` 的取消抛 `cancelled_error/cancelled` 或 `deadline_exceeded`，不结束子进程；已完成结果优先。取消检测延迟最多一个约 10 ms 的等待片段（另加系统调度时间）。

## 7.6 流式管道与有界捕获

`stdin_pipe/stdout_pipe/stderr_pipe(child)` 返回 `process_pipe`，只允许获取设为 `pipe` 的流。`close_pipe` 幂等，关闭输入管道向子进程发出 EOF。管道不支持寻址，短读短写为正常情况：

- `read_pipe(pipe, max_bytes, timeout_ms) -> pipe_chunk { data: bytes, state: str }` 返回 `data`、`eof` 或 `timeout`；单次大小为 1～16 MiB。
- `write_pipe(pipe, data, timeout_ms) -> pipe_write { written: int, state: str }` 尝试写入至多 16 MiB，返回实际前缀长度以及 `written`、`closed` 或 `timeout`；可从 `written` 处继续。`closed` 表示对端停止读取。父侧输入管道使用非阻塞字节写入，缓冲不足时返回短写或等待重试；不取消可能已产生不明前缀的阻塞写入。
- 调用者用 `spawn` 手工交互时须及时消费两路输出；直接在输出未读时无限 `wait` 或只写不读可能遇到应用级背压。`run` 负责三路同时推进。

`make_limits(input, timeout_ms, max_output_bytes)` 返回 `run_limits { input, timeout_ms, max_output_bytes, stop_action }`，`stop_action` 默认为 `keep`，也可显式为 `terminate` 或 `kill`。输入硬上限 64 MiB；两路输出合计硬上限 64 MiB，可设更小（包括 0），超时范围为 `-1` 或 0～4294967294 ms。`run(options, limits)` / `run_with_cancel(options, limits, token)` 要求三流均为 `pipe`，返回：

`completed { child: process_child, status: exit_status, stdout: bytes, stderr: bytes, reason: str, input_written: int }`

`reason` 为 `completed`、`timeout`、`output_limit`、`cancelled`、`deadline_exceeded`、`pipe_failed` 或 `terminate_failed`。预检或启动失败仍抛异常；启动后的可预期停止返回句柄、状态、已捕获字节及输入实际写入长度，便于继续处理外部效果。两路使用独立重叠读取，stdin 同时分块写入；读写缓冲固定大小，捕获不超过合计上限。EOF 与退出都到达才算 `completed`；子进程退出但后代持有管道时仍受同一超时约束。

超时、输出超限、取消、管道失败统一执行明确的 `stop_action`：`keep` 保留运行状态与管道；`terminate` 只请求协作结束，不无限等待，失败返回 `terminate_failed`；`kill` 请求强杀并确认直接子进程结束。已发出的 I/O 在返回前取消并收尾，绝无后台 TX 对象访问。输出超限保留受限前缀；停止时已经从系统管道读入但容不下的字节不会重新放回管道。`input_written` 表示送入系统管道的字节数，不证明子进程已处理。停止及关管道可能有部分外部效果，不承诺回滚。

启动与等待示例见 [process.tx](../examples/process.tx)，有界捕获示例见 [process_capture.tx](../examples/process_capture.tx)；针对空参数、引号、双路背压、超时及资源清理的证据见实施记录。

## 实施记录

非 Windows 平台和第 13 节全库终态验收独立推进。

### 7.5 实施记录（2026-09-27）

- **代码：** 公开 `options/exit_status` 与不透明 `process_child/process_pipe`，接通语义分析、静态 C ABI/LLVM 调用、动态恢复、禁止深复制与资源释放。启动使用宽字符路径、CRT 参数编码、环境快照和受限继承句柄列表；等待缓存退出状态，显式终止与自动清理分开。
- **构建：** `scripts/build.ps1` 完整构建 Windows x64 的编译器、静态库及兼容指纹成功，临时 `build/` 已清理。
- **定向验证：** `scripts/check_process.py lifecycle` 通过中文/空格可执行文件路径与工作目录、空参数/引号/反斜杠/shell 元字符、环境清空及不区分大小写覆盖、非零退出、重复等待、超时与取消保留进程、强杀、关闭别名及启动失败。
- **平台边界：** 无独立控制台进程组时协作终止稳定报 `unsupported_operation`；7.7 的原生用例进一步在独立隐藏控制台内验证了 `CTRL_BREAK_EVENT` 到达子进程组并由处理器正常退出。不承诺跨平台或任意第三方命令行解析规则。

### 7.6 实施记录（2026-09-27）

- **代码：** 三路管道、短读/短写和 EOF、管道别名与关闭、有界 `run` 及共享取消令牌接通公开接口与直接 ABI。stdout/stderr 独立重叠读取，stdin 使用分块非阻塞写入；停止前取消并回收全部读取请求。默认 `keep` 返回子进程和部分结果，`terminate/kill` 必须显式选择。
- **构建：** 完整重建 `txc.exe`、`libtxstdlib.a` 和兼容指纹成功，临时 `build/` 已清理。
- **定向验证：** `scripts/check_process.py pipes` 通过 256 KiB 输入与双路各 256 KiB 预输出的背压场景，确认输出字节及回显完整；覆盖任意二进制、精确捕获上限、无输出、短读及 EOF、短写至缓冲耗尽、写满超时、别名关闭、默认超时保留、显式强杀、输出超限、令牌截止、启动前取消和协作终止不支持时返回可管理句柄。
- **平台边界：** 输出按两路合计字节数限制，停止时可能已消耗未捕获的输出前缀；写入计数不证明子进程已处理。当前 Windows 管道实现不依赖 TX 并发支持，也不承诺后代进程树回收。

### 7.7 边界验证（2026-09-27）

`scripts/check_process.py` 可按 `lifecycle/pipes/contracts/resources/filesystem/diagnostics` 选择定向组，本轮各组均已通过；原生组在隐藏控制台运行，文件夹位于 `tx_build/` 下独立的中文空格临时目录。未运行全库测试。

- `contracts` 覆盖非法环境条目、参数 NUL、无效流策略、拒绝隐式 `.cmd`、负捕获限额及子进程提前关闭输入后的实际前缀。
- `resources` 在正常和失败启动各预热一次后，连续 24 轮启动/捕获/关闭与失败启动，原生句柄数保持不变；还验证最后引用释放不杀进程、运行中主动取消、只继承选定句柄以及真实独立进程组的协作终止。
- `lifecycle/pipes` 同时验证中文和空格路径、安全传参、环境继承与覆盖、超时/取消/输出限制、关闭别名和禁止深复制；新增错误管道类型用例在 TX 源码位置给出编译诊断。
- 两个公开示例通过语法和类型检查，`system_env_extended.tx` 通过已接通 `process_spawn` 的能力回归。文件监视溢出、等待中关闭、只读拒写和链接竞争的验证及修复见[文件系统边界记录](filesystem_path.md#77-边界验证2026-09-27)。

本轮验证针对 Windows x64 工具链；第 2.6 节 Send/Sync 与唯一移动基础已完成，进程句柄仍因未逐类证明而默认非 Send/Sync。跨平台发行及第 13 节全库终态验收仍独立推进。
