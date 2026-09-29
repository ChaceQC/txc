# 第十三部分：工具链接口与验收

本文先固定新增接口，再记录实际验证结果。原有接口继续有效；不新增语言语法。

## 13.1 测试

`fixture(name, setup, operation, teardown) -> bool` 按初始化、测试、清理三阶段执行；初始化失败跳过测试，仍执行清理。每个失败阶段增加失败计数，原生崩溃由进程隔离处理。`set_time_millis(value)`、`advance_time_millis(delta)`、`now_millis()`、`reset_time()` 提供线程局部可控时钟，可将 `test.now_millis` 作为 `fn()->int` 注入被测逻辑；不改变 `time` 模块的真实系统时钟及等待语义。负推进、溢出或未固定就推进报 `invalid_test_clock`。

- `test.parameterized(name: str, count: int, operation: fn(int)->void) -> bool` 按索引执行参数组。回调可用类型明确的闭包捕获任意具体类型的测试数据；参数不经过 `any`。每组独立记名，失败后继续后续参数；非零失败数令程序退出失败。`count` 为 0～100000。
- `test.property(name: str, seed: int, count: int, generate: fn(int,int)->int, predicate: fn(int)->bool, shrink: fn(int)->int, max_shrinks: int = 128) -> bool` 提供整数/案例编号驱动的性质测试。生成器接收原始种子和从 0 开始的编号；谓词可以由案例编号生成任意静态类型的对象。相同种子、回调和版本应产生相同输入。谓词返回 false 或抛出 TX 错误均为反例。失败报告包含种子、编号、原反例、缩减反例和步数。缩减器返回同值即停止；重复候选立即停止；最多 4096 步，不声称全局最小。生成器/缩减器抛错保留原错误并使该组失败，不能误报性质通过。`count` 为 1～100000，`max_shrinks` 为 0～4096。
- `txc test <文件或目录> [--case 路径] [--jobs 1..64] [--timeout-ms 1..86400000] [--isolation workspace|source] [--format json]`。编译顺序稳定，运行进程并行，报告顺序稳定。默认独占临时工作目录，每个 `.tx` 为一隔离组；`source` 显式恢复源码目录 cwd，不能保证用户文件写入隔离。超时从子进程启动开始计时，不包含编译。默认 30000 毫秒，超时和正常退出均清理本组子进程树及临时目录。文件/网络等外部副作用不回滚。
- JSON 报告版本 2 保留旧字段，新增 `timeouts`、每组 `duration_ms` 和 `output_truncated`。性质反例作为单行 JSON 事件保存在组的 `output` 内，避免将编译器报告和用户输出混为同一顶层协议。

## 13.2 日志

`set_file(path, max_bytes = 10485760, backups = 3)` 切换到 UTF-8 JSON Lines 文件，追加写入，达到大小上限后轮转为 `.1`～`.N`；`max_bytes` 为 256～1073741824，`backups` 为 1～32。单条记录超过上限报 `log_size_limit`，不写半条记录。`set_stderr()` 恢复标准错误；`flush()` 显式刷新。更换 sink 与写入由同一进程锁串行化；不提供多个进程共同写同一日志文件的保证。I/O 失败报 `log_io_error`，已写前缀和已经完成的轮转不回滚。

`set_level(level)` 设置最小级别，默认 `trace` 保持兼容；`enabled(level)` 可在构造字段前判断，`event_lazy(level,message,fields: fn()->dict)` 仅在级别启用时求值字段。回调失败不写入记录。`set_secret_keys(vector<str>)` 配置所有 sink 共用的额外敏感键，叠加单次 `event_redacted` 配置；按键遮蔽在序列化之前执行，不能识别普通消息正文内的任意秘密。

`set_request_context(request_id)` 设置当前执行上下文的请求标识，`clear_context()` 清空显式字段。默认线程标识来自系统线程 ID；任务有独立标识，创建线程/任务时继承请求标识。任务切换/嵌套执行恢复各自上下文，不能污染复用的工作线程。请求入口由应用显式设置关联 ID，避免信任或记录带凭据的远程请求头；字段不是身份认证信息。原有 `set_context(task_id,thread_id)` 保留。

## 验证记录

Windows x64 Release 构建通过。`check_toolchain_completion.py` 覆盖参数组、性质反例、夹具/可控时钟、隔离/并行/超时与报告；`check_completion_chains.py native` 覆盖真实原生崩溃、输出限制和后代清理。日志的同进程并发记录、轮转、敏感键、惰性求值通过 `log_extended_test.tx`，独立任务/请求上下文通过 `network_task_db_log.tx`。

敏感字段复核按所有模块共用的公开输出边界进行：`secret_bytes` 的类型规则拒绝装箱/默认打印，公钥与证书私钥使用不透明句柄；`json_writer.cpp` 只序列化普通标量、array/dict，未识别的句柄和用户对象报 `unsupported_value`，不会自行展开私有字段。`log.cpp` 在序列化之前递归处理 array/dict，默认键覆盖 `password/token/secret/api_key/apikey/private_key/privatekey/authorization/credential/cookie`，并叠加全局和单次配置。已核对数据库、Requests、TLS 的密码字段/句柄进入此路径时均被遮蔽或拒绝。普通消息、测试输出、显式 `secret.to_bytes` 后的普通数据仍由调用方负责，不能承诺扫描任意文本即可识别秘密。

总验收、依赖清单、跨模块与平台边界见[标准库终态验收](standard_library_acceptance.md)。
