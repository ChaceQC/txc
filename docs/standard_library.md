# 标准库

## 数学运算

导入 [tx/stdlib/math.txh](../tx/stdlib/math.txh)。int 版本和 float 版本的同名函数按实参类型精确匹配，不进行隐式数值转换。

| 函数 | 行为 |
| --- | --- |
| abs(value: int 或 float) | 绝对值 |
| min(left, right)、max(left, right) | 两个同类型数值中的较小值、较大值 |
| clamp(value, lower, upper) | 将数值限制在闭区间内，要求 lower <= upper |
| mod(left: int, right: int) -> int | 整数余数，符号与 left 相同 |
| sqrt(value: float) -> float | 平方根 |
| pow(base: float, exponent: float) -> float | 浮点幂 |
| floor(value: float)、ceil(value: float) -> int | 向下、向上取整 |

abs 对最小 int、mod 对零除数或最小 int 与 -1 的组合会报运行错误。float 参数及 float 结果必须是有限值；sqrt 的参数不得为负，pow 的结果必须可表示为有限 float。floor、ceil 的结果还必须落在 int 范围内。

## 数组操作

导入 [tx/stdlib/array.txh](../tx/stdlib/array.txh)：

| 函数 | 行为 |
| --- | --- |
| concat(left: array, right: array) -> array | 按顺序拼接两个数组 |
| slice(values: array, start: int, end: int) -> array | 取半开区间 [start, end) |
| reverse(values: array) -> array | 反转元素顺序 |

三者均返回新数组，不修改参数；元素保留原有类型和值。slice 要求 0 <= start <= end <= len(values)，否则报运行错误。现有数组按值传递，追加单个元素可写为 `values = concat(values, [value])`。

字符串库也定义了 slice；同时导入两个模块时，建议用 `as` 别名，例如 `text.slice(...)` 和 `arrays.slice(...)`。

## 文件系统与路径

导入 [tx/stdlib/fs.txh](../tx/stdlib/fs.txh) 和 [tx/stdlib/path.txh](../tx/stdlib/path.txh)，表中分别以 `fs` 和 `path` 为导入别名。路径参数使用 UTF-8 字符串，空路径或含 NUL 的路径会报运行错误。路径操作只处理路径文本，不访问磁盘。

| 函数 | 行为 |
| --- | --- |
| fs.exists(path) -> bool | 路径存在时返回 true；不存在时返回 false |
| fs.is_file(path) -> bool | 路径指向普通文件时返回 true |
| fs.is_directory(path) -> bool | 路径指向目录时返回 true |
| fs.create_directories(path) -> void | 创建路径及缺失的父目录；目录已存在则无操作 |
| fs.list_directory(path) -> array | 返回目录下直接子项的名称，按 UTF-8 字节序排序 |
| path.join(left, right) -> str | 拼接两个路径；right 为绝对路径时以 right 为准 |
| path.parent(path) -> str | 返回父路径；没有父路径时返回空字符串 |
| path.file_name(path) -> str | 返回末级名称 |
| path.extension(path) -> str | 返回末级扩展名，含开头的点；没有则返回空字符串 |

目录列表不递归，数组元素均为 str。文件系统操作遇到权限、非目录或其他 I/O 错误时会报运行错误；is_file 和 is_directory 对不存在的路径返回 false。路径函数的结果统一使用正斜杠作分隔符，便于直接写入 .tx 源码中的路径字符串。

## 时间

导入 [tx/stdlib/time.txh](../tx/stdlib/time.txh)：

| 函数 | 行为 |
| --- | --- |
| unix_millis() -> int | 返回当前 Unix 时间戳，单位毫秒 |
| monotonic_millis() -> int | 返回单调时钟读数，单位毫秒 |
| sleep_millis(duration: int) -> void | 至少等待指定的非负毫秒数 |

unix_millis 使用系统时钟，系统时间调整可能让相邻读数倒退。monotonic_millis 的起点没有日历含义，只适合用两次读数之差测量经过时间；毫秒精度下，短时间内的两次读数可以相同。sleep_millis(0) 无需等待；负数参数会报运行错误，实际等待时间可能比请求的更长。

## 伪随机数

导入 [tx/stdlib/random.txh](../tx/stdlib/random.txh)：

| 函数 | 行为 |
| --- | --- |
| seed(value: int) -> void | 用指定整数重新初始化当前随机序列 |
| random_int(lower: int, upper: int) -> int | 在含两端点的区间内均匀取整数 |
| random_float() -> float | 在半开区间 [0.0, 1.0) 内取浮点数 |

random_int 要求 lower <= upper，支持完整的 int 范围；区间无效时会报运行错误。未调用 seed 时，生成器由 C++ random_device 初始化，实际熵源取决于运行环境。用相同种子并按相同顺序调用，在同一构建环境中可重现结果。这些函数使用伪随机数生成器，不适合生成密钥、令牌或其他安全敏感值。

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
