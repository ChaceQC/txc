# 解析与可恢复错误

标准库保留具体结果结构体；语言提供 `try { } exception type as e { }`，两者使用相同的错误信息。内置 `result<T>` 可从旧结果转换，规则见[option 与 result](option_result.md)。暂不引入用户泛型、`throw` 或 `finally`。本轮先完成代码，随后经用户授权执行少量定向验证，记录见本文末尾。

## 统一结果

`error.txh` 公开 `error_info`，字段依次为 `str kind`、`str code`、`str message`。`kind` 是错误类别，`code` 是稳定的机器可读代码，`message` 是中文说明。

现有 `parse_error`、`io_error`、`runtime_error` 保持原编号与行为。公共契约还注册 `process_error`、`database_error`、`security_error`、`cancelled_error`，字段同样是 `kind/code/message`，可用于 `exception` 分支；目前尚无对应公开操作产生这些新类别。完整 `(kind, code)` 登记、严格接口与结果接口的选择见[标准库公共契约](standard_library_foundation.md#3-错误类别与稳定代码登记)。

具体结果为 `int_result`、`float_result`、`str_result`、`bool_result`，字段依次为 `bool ok`、对应静态类型的 `value`、`error_info error`。成功时 error 的三个字符串均为空；失败时 value 分别为 `0`、`0.0`、`""`、`false`，调用方应先检查 ok。成功的空文本与失败可以明确区分，不使用 none 代替失败。

结果及其错误对象仍是普通共享结构体，`ok/value/error` 和错误字符串均可按原规则修改；修改 `ok` 不会自动修改 `value` 或清除 `error`。例如 `auto alias = parsed` 后，`alias.value = 7` 必须能通过 `parsed.value` 观察。

编译器可将直接位置实参调用 `try_parse_int/try_parse_float` 初始化的局部结果暂存为标量值与错误描述。仅读取 `ok/value` 时不构造动态结果或错误对象；读取 `error`、写字段、建立别名、装箱、传参或返回时，先物化同一个普通结构体，后续访问和异常分支合流使用该对象。命名/展开实参和其他不能直接分析的初始化继续使用完整结果路径；这不改变公开返回类型及错误字符串。

`error.fail_io(code: str, message: str) -> void` 供 `.tx` 编写的标准库网络包装层报告可恢复的 I/O 错误；调用后进入 `io_error` 路径，调用方可按 `code` 分支处理。它不返回正常值，也不替代 `try`/`exception` 的捕获规则。

运行时首次记录错误时保存当前 TX 调用栈及最近执行的语句位置。`error.stack_trace() -> str` 返回按调用顺序排列的 `函数 (文件:行:列)` 文本；在 `exception` 分支取走错误后仍可读取该次快照，下一次错误会覆盖。无错误时返回空文本。栈记录不包含实参值、文件内容或秘密正文；未捕获错误会在原中文信息后附上该栈。重新抛出的清理错误不会覆盖正在传播的原错误与原栈。

## parse 接口

导入 `parse.txh` 后提供：

| 接口 | 返回类型 |
| --- | --- |
| `try_parse_int(str text)` | `error.int_result` |
| `try_parse_int(str text, int base)` | `error.int_result` |
| `try_parse_float(str text)` | `error.float_result` |
| `parse_int(str text)` / `parse_int(str text, int base)` | `int` |
| `parse_float(str text)` | `float` |

- 整数默认十进制；指定进制限 2～36，字母数字不区分大小写。允许一个前导 `+` 或 `-`，不自动识别或剥离 `0x` 等进制前缀；每个字符均作为当前进制的数字处理，不接受下划线、内部空白或多重符号。
- 两端仅忽略 ASCII 空白（空格、制表、换行、回车、垂直制表、换页）。必须消费完整文本，不能部分解析。
- 整数范围为有符号 64 位，包括最小负值；float 接受十进制小数和科学计数法，必须为有限 double，不接受 NaN、Infinity 或十六进制浮点数。
- 空文本、非法语法、越界、非法进制分别返回 `empty_input`、`invalid_syntax`、`out_of_range`、`invalid_base`；浮点非有限值返回 `non_finite`，类别均为 `parse_error`。
- 整数先检查进制，再检查空文本及数字；即使前面的数字已经溢出，后续非法字符仍优先报告 `invalid_syntax`。仅当全部字符有效且数值越界时报告 `out_of_range`。
- `try_parse_*` 对这些输入失败返回结果；`parse_*` 对相同失败进入可捕获错误路径。分配失败等运行时故障仍按运行时错误处理。
- 现有 `str as int/float` 保留转换与报错文本约定，转换失败归入 `parse_error`，可以捕获。

## 异常语法

```tx
import "parse.txh" as parse
import "error.txh" as errors

def main() -> int
{
    try
    {
        print(parse.parse_int("错误输入"))
    }
    exception errors.parse_error as e
    {
        print(e.code, e.message)
    }
    exception errors.runtime_error as e
    {
        print(e.kind, e.message)
    }
    return 0
}
```

`try` 必须跟至少一个 `exception 类型 as 变量名` 分支。大括号与头部可以同行或换行；分支可以紧接前一右括号或另起一行。分支按书写顺序匹配，第一项匹配后停止。

可捕获类型由标准库 `error.txh` 定义：`parse_error` 匹配解析失败，`io_error` 匹配文件、编码和网络 I/O 失败，`process_error`、`database_error`、`security_error`、`cancelled_error` 分别匹配对应的新错误类别，`runtime_error` 匹配所有运行时错误。fs/path 等其他现有模块的失败目前归入 runtime_error。每种类型均有 kind、code、message 字段；不按同名用户结构体或字符串名称进行匹配。重复分支、runtime_error 后的分支、非错误类型均为编译错误。

try 块与各 exception 块拥有独立作用域，e 只在所属分支内可见。支持嵌套、函数及方法间传播；处理分支再次失败时交给外层处理器。正常执行跳过所有 exception，处理后继续后续语句。try 和每个处理分支均返回时，满足函数完整返回路径要求。

错误跳转释放退出作用域的局部对象和尚未交付的临时值，保留外层变量及已经完成的修改；这不是事务回滚。析构清理期间原始错误优先，避免二次错误覆盖最初原因。没有匹配处理器时继续向外传播；到达程序入口仍未处理则输出中文运行错误、清理资源并返回非零退出码。

## 文件接口

`file.try_read_text(str path, str encoding)` 返回 `error.str_result`；`file.try_write_text(str path, str text, str encoding)`、`file.try_append_text(...)` 返回 `error.bool_result`，成功 value 为 true。文件或编码错误返回 `io_error` / `operation_failed`；原有 read_text/write_text/append_text 仍在失败时进入运行时错误路径，现在可用 `exception errors.io_error as e` 捕获。有效空文件返回 ok=true、value=""。

示例见 [parse_errors.tx](../examples/parse_errors.tx)，主示例也已加入 `parse_error_demo()`。

## 实现与验证

2026-09-26：`scripts/build.ps1` 构建通过，已更新 `tx/txc.exe` 和 `tx/libtxstdlib.a`，确认产物后清理 `build/`。`python -X utf8 scripts/check_parse_errors.py` 的 6 个定向场景通过：

- 综合行为：int 最大/最小边界、2/16/36 进制、空文本与非法符号、溢出、有限浮点、命名和展开调用；文件成功空文本、读取失败、写入和追加及编码失败。
- 同一综合场景验证跨模块返回、嵌套匹配、处理分支再次出错、vector 越界与随机数快速调用；临时参数、拥有实参、循环内局部对象、析构期间原始错误保留，以及替换变量时旧对象析构失败后新值仍可使用。
- 未捕获错误：中文诊断、退出码 1，仍执行析构。
- 独立 parse 示例及根目录主示例分别编译运行，关键输出和自建文本文件清理符合预期。
- 非错误类型捕获和重复捕获各一个编译诊断场景，均包含源码位置。

以上合计 3 个正常运行、1 个预期失败运行、2 个静态诊断；未执行全量回归或性能基准。

## 2.7 实施记录

- **代码：** 编译器在 TX 函数入口、返回、错误退出和语句位置更新运行时栈；首次失败保存函数与源码位置快照。`error.stack_trace()` 在捕获后返回该快照，未处理错误在原中文信息后输出栈。错误清理守卫保留原栈，避免析构期间的新错误覆盖初始原因；现有具体异常类别匹配保持原规则。
- **构建：** `scripts/build.ps1` 在 Windows x64 成功生成编译器、静态库与兼容清单。
- **定向验证：** `tests/stdlib/stack_trace.tx` 的三层调用在捕获后输出 `main/middle/inner` 与各自源码位置；未捕获的 `hash_key_error.tx` 在原整数除零信息后输出 `main/hash_key`。`examples/option_result.tx`、`examples/closures.tx`、`examples/parse_errors.tx` 及 `tests/bytes_file_stream/behavior.tx` 重新编译运行，覆盖结果值、闭包引用环、异常分类、文件流关闭和失败路径。未运行全量套件。
- **边界与验收：** 映射精度为 TX 函数和最近执行的语句位置；同一语句内的子表达式目前共享该语句位置。栈只包含 TX 帧，不含原生 C++ 内部帧或实参值。当前证据限 Windows x64，后续 `debug.txh` 可以复用此栈来源。
