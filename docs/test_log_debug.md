# 测试、日志与诊断

本文保留[实施清单](standard_library_plan.md)第 3 节的基础接口及历史记录。第 13 节新增的参数化/性质测试、隔离与超时、并发日志、sink 见[工具链补齐](toolchain_completion.md)，性能分析见[profile](profile.md)；下文“后续补齐”描述的是第 3 节完成时的边界，不代表当前缺口。

## 3.1 `test.txh`

`assert_true/false(condition: bool, message: str = "")`、`assert_equal(expected, actual, message = "")`（`int/float/bool/str` 同类型重载）和 `assert_near(expected: float, actual: float, tolerance: float, message: str = "")` 在失败时抛出 `runtime_error`，稳定代码为 `assertion_failed`。错误栈记录调用断言的 TX 源码位置。浮点近似比较要求有限且非负的绝对误差；NaN 不能相等，无穷大只在相同符号时相等。误差无效时用 `invalid_tolerance` 报错，属于测试代码错误。

`assert_throws(operation: fn()->void)` 要求回调产生任一种 TX 可恢复错误；`assert_throws_code(operation: fn()->void, code: str)` 还要求稳定错误代码相同。回调正常返回或代码不符时断言失败，已匹配的错误被消费，不继续传播。预期异常的验证不包括操作系统原生崩溃。

`run_case(name: str, operation: fn()->void) -> bool` 在 `main` 中显式注册并执行一个命名测试项；回调失败时记录名称、错误代码和 TX 调用栈，继续执行后续项。`failures() -> int` 可读取本进程失败项数；即使 `main` 忘记检查计数，非零失败数也会让进程退出码至少为 1。空名称报 `invalid_name`。测试项内可使用上述断言，普通未捕获 TX 错误也记为该项失败。

`temp_directory() -> str` 为当前测试进程创建独占目录，优先放在程序所在目录；该目录不可写时回退到系统临时目录。正常进程退出时清理；由 `txc test` 启动时，运行器在用例结束后清理其临时工作区，包括原生崩溃路径。单独运行的程序若原生崩溃，系统临时目录中的夹具可能残留。`seed(value: int)` 固定当前线程现有 `random` 生成器的种子；随机算法升级后的跨版本稳定性留给 6.3 定义。

## 3.2 `txc test`

`txc test <目录>` 按相对路径排序发现 `*_test.tx`，每个源文件是一个测试组，文件自己的 `main() -> int` 可调用 `test.run_case` 显式注册命名项。`txc test <文件.tx>` 显式运行一个文件；`--case <相对路径>` 从目录发现结果中只运行该文件。每组独立编译并在独立进程运行，标准输出和标准错误被收集。返回 0 为通过；正常非零退出为失败；Windows 异常退出为崩溃；编译阶段失败为编译错误。编译错误不阻止其余项运行。最终有任一非通过项则运行器返回 1；用法错误返回 2。

`--format json` 只在标准输出写一个 UTF-8 JSON 对象，含 `summary` 的 `passed/failed/crashed/compile_errors` 计数，以及每项的 `name/status/exit_code/output`。普通格式逐项显示结果并在末尾汇总。用例输出会原样包含在报告内；测试代码不应主动输出秘密。单项进程隔离是本阶段已有的默认行为；可配置超时、并行、参数化及性质测试在 13.1 补齐。

## 3.3 `log.txh`

`event(level: str, message: str, fields: dict)` 将一个 JSON Lines 事件写到标准错误。级别只接受 `trace/debug/info/warn/error`；事件包含 Unix 毫秒时间戳、级别、消息、`fields` 和 `context.task_id/thread_id`。`set_context(task_id: str, thread_id: str)` 设置调用线程当前的上下文字段；空字符串表示尚无上下文，后续并发运行时再自动接通。

`event_redacted(level, message, fields, secret_keys: vector<str>)` 在序列化前将显式列出的键遮蔽为 `[REDACTED]`。两种事件入口还会递归遮蔽常见敏感键（`password/token/secret/api_key/authorization/credential/cookie`，不区分大小写）。遮蔽发生在 JSON 格式化前；普通 `str` 中主动放入秘密、非敏感键名下的秘密仍须由调用方标记。非法级别报 `invalid_level`；不支持 JSON 的字段值沿现有 JSON 错误语义失败，不输出半条事件。文件 sink、跨线程单条写入保证和自动任务上下文在 13.2 完成。

## 3.4 `debug.txh`

`stack_trace()` 返回当前 TX 调用栈，`last_error_stack()` 返回最近一次错误的 TX 调用栈，格式为 `函数 (文件:行:列)`，不包含参数值。`location()` 返回当前调用语句的 `文件:行:列`。`dump(value)` 为 `int/float/bool/str` 的静态重载，只向标准错误写类型、位置和值，不接受任意 `any` 或未来的秘密类型。调用者主动传入普通字符串时仍须自行避免打印秘密。编译器的 Release 产物保持 TX 语句位置更新；文件/函数栈映射不依赖 C++ 调试符号。

## 实施记录

- **代码：** `test/log/debug.txh` 公开静态接口，`src/stdlib/test.cpp` 管理临时目录，`src/stdlib/log.cpp` 实现日志策略，`src/backend/cpp/{test,log,debug}_abi.cpp` 提供断言/诊断运行时入口和日志 C ABI，`src/backend/llvm` 静态绑定重载和可恢复回调路径，`src/driver/test_runner.cpp` 负责发现、编译、子进程执行及 JSON 报告。没有增加第三方依赖。`txc test` 的每个文件在独立进程执行；具名 `run_case` 继续执行后续回调，失败数保证最终退出码非零。
- **构建：** 以 `scripts/build.ps1` 完整构建 Windows x64 Release 包，更新 `txc.exe`、`libtxstdlib.a`、公开接口及兼容清单。生成程序仍由现有 `clang -O3` 路径编译。
- **定向验证：** `txc test tests/diagnostics --case basic_test.tx --format json` 通过，覆盖四种类型断言、近似误差、预期错误及反例、具名项、临时目录、固定种子、当前/错误栈、Release 位置、结构化事件、显式和嵌套敏感键、非法日志级别。`assertion_failure.tx` 返回失败并指出断言源码位置；`registered_failure.tx` 返回失败且仍执行后续具名项；`compile_error.tx` 单独计入编译错误并给出源码位置。未运行与本节无关的大套件。
- **边界：** 本阶段以 Windows x64 的构建和运行结果为证据。运行器已按常见 Windows SEH 状态代码单独统计崩溃，尚未做真实原生崩溃注入；第 13.1 节继续验证超时、并行与隔离边界。日志只自动遮蔽常见键名并允许调用方显式补充；可替换 sink、轮转和并发写入保证在 13.2 完成。第 13 节终态矩阵及性能分析尚未验收。
