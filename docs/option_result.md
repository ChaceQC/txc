# option 与 result

`option<T>`、`result<T>` 和 `result<void>` 是编译器内置的参数化类型，不需要 import。`T` 必须是已知的可持有类型；`option<void>`、`option<none>`、`result<none>` 和函数类型参数无效。二者的运行时值带完整类型标记；通过 `any` 传递后只能显式 `as option<T>` / `as result<T>` 恢复，类型参数必须全部一致。

## 构造与状态检查

| 表达式 | 结果 |
| --- | --- |
| `option<T>()` | 无值 |
| `option<T>(value)` | 有值，`value` 静态类型恰为 T |
| `result<T>(value)` | 成功，保存 T |
| `result<T>(false, info)` | 失败，`info` 为 `error.error_info` |
| `result<void>()` | 成功，不保存值 |
| `result<void>(false, info)` | 失败，保存 `error.error_info` |

失败构造的第一个参数必须是字面量 `false`，以免把动态 bool 当作运行时状态选择。`result<T>(旧结果)` 可把 `error.int_result/float_result/str_result/bool_result/any_result` 转为对应的内置结果；旧结构和旧函数保持原样。转换复制成功值或错误信息，不丢失 `kind/code/message`。

`option<T>` 提供 `is_some()`、`is_none()`、`value()`、`value_or(fallback: T)`。`result<T>` 提供 `is_ok()`、`is_err()`、`value()`、`error()`；`result<void>` 的 `value()` 只检查状态，不返回值。所有方法只接受位置实参。`value()` 对空 option 报 `runtime_error/invalid_state`；对失败 result 则以保存的 `(kind, code, message)` 原样抛出，沿用现有 `try/exception` 匹配与清理路径。调用 `error()` 前需导入 `error.txh`，其静态返回类型为该模块的 `error_info`；成功 result 上调用会报 `runtime_error/invalid_state`。`value_or` 总会先求值 fallback，再根据 option 状态选择；这与普通实参求值顺序一致。

无值与失败均为独立标记，不能用 `none`、0、空字符串或默认构造的 T 代替。普通复制共享复合载荷；`deep_copy` 递归复制所含对象图并保留环。失败状态中的错误信息为独立值，`message` 仅供人阅读，程序按 `(kind, code)` 分支。当前不引入新的 `match` 语法，使用 `if value.is_some()` 或 `if result.is_ok()` 显式检查分支。

示例见[option/result 用法](../examples/option_result.tx)。

## 2.2 实施记录

- **代码：** 解析器、模块解析、语义分析和 LLVM 后端识别内置 `option/result`；运行时保存独立状态和值，失败结果保存 `error.error_info` 并在 `value()` 时按原 `(kind, code)` 传播。局部 `option<int/float/bool>` 使用栈上的“是否有值 + 标量值”，构造、状态检查和 `value()` 直接读写；跨函数传参或返回、进入 `any` 和复合值时按原类型标记生成运行时句柄，恢复为局部变量时再解包。空值仍报 `runtime_error/invalid_state`。`error()` 在导入 `error.txh` 后返回静态类型的 `error_info`。旧具体结果的公开结构和函数保持原样，`result<T>(旧结果)` 提供迁入桥。
- **构建：** `scripts/build.ps1` 在 Windows x64 成功生成编译器、标准库静态库和包兼容清单，确认后清理 `build/`。
- **定向验证：** `examples/option_result.tx` 编译运行，覆盖 option 空/有值、`value_or`、成功/失败的 `result<int>`、`result<void>`、旧结果成功/失败转换、异常类别与代码传播、`deep_copy` 和包含引用环的 option。`tests/stdlib/sum_module/main.tx` 跨 `.txh` 返回两种新类型并输出 `23 42`；`sum_invalid_type.tx` 在源码位置拒绝 `option<void>`，`sum_type_mismatch.tx` 在动态恢复到错误类型时失败。未运行全量套件。
- **边界与验收：** 现有 `error.*_result` 尚无反向生成桥；旧接口完全保留。`error()` 使用前须导入 `error.txh`。当前证据限于本机 Windows x64 的定向场景；2.6 的递归 Send/Sync 与唯一移动已完成，标准库终态矩阵仍单独验收。
