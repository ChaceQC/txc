# 输入输出、转换与字符串库

## 终端输入输出

`input()` 从标准输入读取一行 UTF-8 文本，返回不含行尾换行的 `str`。`input("提示")` 先将提示写到标准输出并刷新。输入到文件末尾时，若已有字符但没有结尾换行，仍返回这最后一行；之后再次调用才报运行错误。读取失败或输入不是有效 UTF-8 时也报运行错误。

`print(value)` 将 `int`、`float`、`bool`、`str`、`none` 或 `any` 中的这些值输出到标准输出并换行。数值和布尔值的文本格式与 `as str` 一致；数组和结构体不能直接打印。

需要控制换行、标准错误或刷新时，导入 [tx/stdlib/io.txh](../tx/stdlib/io.txh)：

```tx
import "io.txh" as io

def main() -> int {
    io.write("姓名> ")
    io.flush()
    auto name = input()
    io.write_line("你好，" + name)
    io.write_error("这条消息写入标准错误\n")
    return 0
}
```

| 函数 | 行为 |
| --- | --- |
| `write(text: str)` | 写入标准输出，不自动换行 |
| `write_line(text: str)` | 写入标准输出并追加 `\n` |
| `write_error(text: str)` | 写入标准错误，不自动换行 |
| `flush()` | 刷新标准输出 |

上述函数均返回 `void`；输出失败会报运行错误。标准输入、标准输出和标准错误使用 UTF-8；重定向时直接读写 UTF-8 字节。

## 文本文件输入输出

导入 [tx/stdlib/file.txh](../tx/stdlib/file.txh)。所有文件路径均是 UTF-8 `str`；Windows 上先严格解码为宽字符路径再打开文件，因此中文路径不依赖系统活动代码页。

```tx
import "file.txh" as file

def main() -> int {
    file.write_text("记录.txt", "你好\n", "utf-8")
    file.append_text("记录.txt", "世界\n", "utf-8")
    print(file.read_text("记录.txt", "utf-8"))
    return 0
}
```

| 函数 | 行为 |
| --- | --- |
| `read_text(path: str, encoding: str) -> str` | 读完整个文件，解码为 UTF-8 字符串 |
| `write_text(path: str, text: str, encoding: str) -> void` | 转换编码后创建或覆盖文件 |
| `append_text(path: str, text: str, encoding: str) -> void` | 转换编码后追加到文件；不存在则创建 |

`encoding` 不区分大小写，支持 `utf-8`、`utf-8-sig`、`utf-16`、`utf-16le`、`utf-16be`、`gbk` 和 `gb18030`；`utf8`、`utf8-sig`、`utf16`、`utf16le`、`utf16be` 是对应别名。`utf-8-sig` 写入 BOM，读取时去掉可选 BOM；`utf-16` 写入小端 BOM，读取时要求有小端或大端 BOM；显式 `utf-16le`、`utf-16be` 写入时不加 BOM，读取时允许去掉匹配的 BOM。追加到非空 `utf-16` 文件时沿用文件 BOM 指定的字节序；追加到空文件时写入小端 BOM。追加到非空 `utf-8-sig` 文件时不重复写 BOM。

文件函数不改变换行符，不自动建立父目录。未知字符集、无效编码、目标字符集无法表示文本、路径错误及读写失败都会报运行错误。编码转换会先完成，再打开待写文件，避免因编码错误覆盖现有文件。

## 显式转换

`as` 用于数值与字符串之间的转换，转换失败会报运行错误：

| 源类型 | 目标类型 | 规则 |
| --- | --- | --- |
| int | float | 转成双精度浮点数，可能损失精度 |
| float | int | 向零截断，检查是否超出 int 范围 |
| str | int | 解析完整的十进制整数，允许首尾 ASCII 空白和正负号 |
| str | float | 解析完整的有限浮点数，允许首尾 ASCII 空白 |
| int、float、bool | str | 转为对应的文本表示 |
| any 中实际存放上述值 | int、float、str | 按实际类型执行对应转换 |

例如 2 as float 输出 2.0，"42" as int 得到 42，3.5 as str 得到 "3.5"。对 none、数组或结构体做这些转换会报运行错误。

## 字符串基础操作

先导入 [tx/stdlib/string.txh](../tx/stdlib/string.txh)：

```tx
import "string.txh"
```

| 函数 | 返回值 | 说明 |
| --- | --- | --- |
| len(text) | int | UTF-8 字符数量；len(array) 仍返回数组长度 |
| contains(text, part) | bool | 是否包含子串 |
| starts_with(text, prefix) | bool | 是否以指定文本开头 |
| ends_with(text, suffix) | bool | 是否以指定文本结尾 |
| find(text, part) | int | 首次出现的字符位置，找不到返回 -1 |
| slice(text, start, end) | str | 按字符位置取半开区间 [start, end) |
| replace(text, old, new) | str | 替换所有 old；old 不得为空 |
| split(text, separator) | array | 按非空分隔符拆分，保留空段 |
| join(parts, separator) | str | 连接数组中的字符串；其他元素类型报错 |
| trim(text) | str | 去除首尾 ASCII 空白 |
| lower(text) / upper(text) | str | 转换 ASCII 字母大小写，其他字符保持原样 |

字符位置从 0 开始；slice 要求 0 <= start <= end <= len(text)。字符串使用 UTF-8 保存，以上按字符计数的操作会检查编码有效性。这些函数的实现位于编译得到的标准库静态库中，.txh 只提供接口。完整用法见 [导入与输入示例](../examples/import_io.tx)。
