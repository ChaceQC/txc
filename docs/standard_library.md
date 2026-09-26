# 标准库

本文说明当前已实现的标准库接口。后续能力缺口、建议优先级和实现前提见[标准库扩展规划](standard_library_plan.md)。

`httpx` 与 `websocket` 提供 HTTP/1.1、HTTP/2、WS/WSS 客户端和对应服务端的同步文本、二进制与文件流接口。HTTP/2 支持 HTTPS ALPN、明文 h2c prior knowledge，以及使用 PEM 证书的 TLS 服务端；二进制正文和消息直接使用 `bytes` 或 `binary_stream`。完整签名、边界及定向验证状态见[网络模块说明](network.md)。公开接口分别为 [httpx.txh](../tx/stdlib/httpx.txh) 和 [websocket.txh](../tx/stdlib/websocket.txh)。

`requests` 在 `httpx` 上提供接近 Python Requests 的同步客户端、命名参数、`session` 与 `response` 类、JSON、表单、Cookie 和重定向。公开接口为 [requests.txh](../tx/stdlib/requests.txh)，`.tx` 实现构建进同一个标准库静态库。具体签名、Python API 对照及限制见[requests 模块说明](requests.md)。

`bytes`、`encoding` 和 `file_stream` 的接口及语义见[字节值与文件流](bytes_file_stream.md)；代码已构建，少量定向场景已通过。

导入 [bytes.txh](../tx/stdlib/bytes.txh) 可构造、拼接、切片及进行 Hex/Base64 转换；[encoding.txh](../tx/stdlib/encoding.txh) 提供内存中的文本编码与解码；[file_stream.txh](../tx/stdlib/file_stream.txh) 提供二进制和文本文件流。`bytes` 也可作为 `vector<bytes>` 元素以及 `array`、`dict` 的值。

内置 `map<K, V>`、`set<T>`、`heap<T>`、`queue<T>` 与 `vector<T>` 一样无需导入，接口见[类型化容器](typed_containers.md)。它们的 C++23 实现随标准库静态库交付，普通模块可在 `.txh` 中使用这些类型。

## 解析与可恢复错误

解析及可恢复错误使用 `parse.txh`、`error.txh`，完整规则见[解析与可恢复错误](errors_and_parse.md)。`try_parse_int` 支持十进制和显式 2～36 进制，`try_parse_float` 支持有限十进制浮点数；返回具体的 `ok/value/error` 结果。严格 `parse_int/parse_float` 可由 `try { } exception 类型 as e { }` 捕获。文件库新增 `try_read_text/try_write_text/try_append_text`，复用相同的错误字段和结果结构。

## JSON

导入 [json.txh](../tx/stdlib/json.txh) 可解析和序列化 JSON；配置文件可通过 `file.read_text`、`json.parse_object` 得到普通 `dict`，直接索引和修改字段，再用 `json.stringify_pretty` 与 `file.write_text` 保存。`try_parse` 返回可区分合法 `null` 与失败的 `error.any_result`，`parse` 在错误时抛出含行、列和字节偏移的 `parse_error`。`contains`、`get` 及类型化 `get_*` 也可用于明确的字段读取。完整流程、类型对应和错误码见 [JSON 模块说明](json.md)。

## 容器算法

导入 [algorithm.txh](../tx/stdlib/algorithm.txh)，提供 `sort`、`sorted`、`find`、`count`、`lower_bound`、`upper_bound`、`reverse`，以及数值向量的 `sum`、`min_element`、`max_element`。排序与二分覆盖 `vector<int/float/str>`；查找、计数和原地反转还支持 `vector<bool>`。

`sort`、`reverse` 修改共享向量，`sorted` 返回独立向量。`find` 未命中返回 -1，二分边界未命中返回向量长度；最值函数返回元素值，空向量时报错。求和检查整数溢出与非有限浮点数。字符串按 UTF-8 字节序排序；浮点排序将 NaN 放在末尾。二分输入必须已经升序排列。完整签名、空容器规则、浮点边界和复杂度见[容器算法说明](algorithm.md)，与已有容器组合使用见[示例](../examples/algorithm.tx)。

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
| push_back(values: array, value: any) -> void | 原地追加一个值，摊还 O(1) |
| pop_back(values: array) -> void | 原地删除尾元素；空数组报运行错误 |
| insert(values: array, index: int, value: any) -> void | 在 0 到 len(values) 之间插入，O(n) |
| erase(values: array, index: int) -> void | 删除有效下标处的元素，O(n) |
| clear(values: array) -> void | 清空共享数组 |

`concat`、`slice`、`reverse` 返回新数组，不修改参数；其中的复合元素仍共享原对象。slice 要求 0 <= start <= end <= len(values)，否则报运行错误。数组默认按引用共享；需要递归复制时使用内置 `deep_copy(values)`。

新增的原地操作会通过所有共享引用生效。`push_back` 和 `insert` 的 `value` 接受任意非 void 值，包括基础值、none、数组、字典、结构体、类和 vector；编译期确定调用目标，在进入异构容器时保存该值。其他普通函数的参数匹配规则不变。实参从左到右各求值一次，插入位置按全部实参求值后的容器长度检查；复合元素保留共享关系。

`for` 仍记录进入循环时的长度，追加的元素不进入本次遍历；每次迭代按当前下标读取共享数组，插入和删除可能改变后续读取的元素，缩短导致待访问下标越界时报告运行错误。示例见[容器接口](../examples/container_interfaces.tx)。

字符串库也定义了 slice；同时导入两个模块时，建议用 `as` 别名，例如 `text.slice(...)` 和 `arrays.slice(...)`。

## 字典操作

导入 [tx/stdlib/dictionary.txh](../tx/stdlib/dictionary.txh)，建议使用 `as dictionary` 别名。带键的操作分别提供 `int`、`float`、`bool`、`str`、`none` 重载，按键的静态类型选用；字典仍保留原有的索引规则。

| 函数 | 行为 |
| --- | --- |
| `get(values: dict, key) -> any` | 返回对应值；缺键返回 `none`，不插入新键 |
| `contains(values: dict, key) -> bool` | 判断键是否存在，包括值为 `none` 的键 |
| `remove(values: dict, key) -> bool` | 删除键并返回 `true`；缺键返回 `false` |
| `keys(values: dict) -> array` | 返回键的快照，顺序不保证 |
| `values(values: dict) -> array` | 返回值的快照，顺序不保证 |
| `items(values: dict) -> array` | 返回条目快照，每个元素是 `[键, 值]` 数组，顺序不保证 |
| `clear(values: dict) -> void` | 清空共享字典 |

`get` 返回 `none` 时，可用 `contains` 区分缺键与已存储的 `none`。`keys` 和 `values` 返回独立数组，其中的复合值仍与原字典共享对象；两次独立调用的排列不能当作彼此配对的保证。`remove` 和 `clear` 会通过共享引用生效。普通键的查找、插入与删除平均为 O(1)，哈希碰撞极端情况下可能退化。完整用法见[字典操作示例](../examples/dictionary_operations.tx)。

`items` 的外层数组及每个键值对数组均独立于字典；替换快照中的键或值不会更新原字典，但快照内的复合值仍与原对象共享。它在一次遍历中取得成对条目，也保留无法再次按键查询的 NaN 条目。none 键可直接写为 `dictionary.get(values, none)`。

浮点 `NaN` 键保留现有语义：可以存入并计入长度与遍历，但由于它不等于自身，后续查找和删除都不会命中它。

## 文件系统与路径

导入 [tx/stdlib/fs.txh](../tx/stdlib/fs.txh) 和 [tx/stdlib/path.txh](../tx/stdlib/path.txh)，表中分别以 `fs` 和 `path` 为导入别名。路径参数使用 UTF-8 字符串，空路径或含 NUL 的路径会报运行错误。路径操作不查询目标文件；`absolute` 在处理相对路径时读取进程工作目录。

| 函数 | 行为 |
| --- | --- |
| fs.exists(path) -> bool | 路径存在时返回 true；不存在时返回 false |
| fs.is_file(path) -> bool | 路径指向普通文件时返回 true |
| fs.is_directory(path) -> bool | 路径指向目录时返回 true |
| fs.create_directories(path) -> void | 创建路径及缺失的父目录；目录已存在则无操作 |
| fs.list_directory(path) -> array | 返回目录下直接子项的名称，按 UTF-8 字节序排序 |
| fs.list_directory_vector(path) -> vector<str> | 与 list_directory 相同，直接返回字符串向量 |
| fs.walk_directory(path) -> vector<str> | 递归列举所有子项，返回相对根目录的路径，按 UTF-8 字节序排序 |
| fs.copy_file(source: str, destination: str, overwrite: bool) -> void | 复制普通文件；overwrite 为 false 时目标已存在即报错 |
| fs.rename(source: str, destination: str) -> void | 重命名或在同一文件系统内移动文件、目录；目标已存在时报错 |
| fs.remove(path) -> bool | 删除文件、符号链接或空目录；不存在返回 false |
| fs.remove_all(path) -> int | 递归删除并返回删除的文件、目录和链接数量；不存在返回 0 |
| fs.file_size(path) -> int | 返回普通文件的字节数 |
| fs.modified_millis(path) -> int | 返回最后修改时间的 Unix 毫秒时间戳 |
| path.join(left, right) -> str | 拼接两个路径；right 为绝对路径时以 right 为准 |
| path.parent(path) -> str | 返回父路径；没有父路径时返回空字符串 |
| path.file_name(path) -> str | 返回末级名称 |
| path.extension(path) -> str | 返回末级扩展名，含开头的点；没有则返回空字符串 |
| path.normalize(path) -> str | 词法整理分隔符、`.` 和可消去的 `..`，不解析符号链接 |
| path.is_absolute(path) -> bool | 判断是否为当前平台的绝对路径 |
| path.absolute(path) -> str | 基于当前工作目录生成规范化绝对路径，不要求目标存在 |
| path.relative(path: str, base: str) -> str | 规范化后计算词法相对路径；二者需同为相对路径或同根绝对路径 |
| path.replace_extension(path: str, extension: str) -> str | 替换扩展名；空字符串移除扩展名，缺少开头的点时自动补点 |

`list_directory` 及其 vector 版本不递归，元素均为 str。文件系统操作遇到权限、非目录或其他 I/O 错误时会报运行错误；is_file 和 is_directory 对不存在的路径返回 false。路径函数的结果统一使用正斜杠作分隔符，便于直接写入 .tx 源码中的路径字符串。

`walk_directory` 包含文件、子目录和链接本身，不进入子目录符号链接；结果不含根目录本身。读取中遇到错误即报错，不静默跳过。`remove_all` 删除符号链接本身，不递归其目标。复制、移动和写文件均不自动创建父目录；移动跨文件系统时报告错误。修改时间精度由文件系统决定，文件大小和删除数量超出 int 范围时报错。文件系统变更失败可能已经产生部分效果，接口不提供事务或并发隔离保证。

`relative` 不访问磁盘、不消解符号链接；相同路径返回 `.`，不同盘符等无法表达相对关系的情况报错。`replace_extension` 的扩展名不得含路径分隔符或 NUL。完整操作见[文件系统接口示例](../examples/filesystem_interfaces.tx)。

## 系统与环境变量

导入 [system.txh](../tx/stdlib/system.txh) 获取程序参数、当前工作目录、可执行文件路径、临时目录和用户主目录，或切换进程工作目录。`args()` 返回不含可执行文件名的独立 `vector<str>` 快照；入口仍为 `def main() -> int`。

导入 [env.txh](../tx/stdlib/env.txh) 使用 `contains/get/set/remove` 查询和修改当前进程的环境变量。`get(name)` 在缺失时报错，`get(name, default_value)` 仅在缺失时使用默认值，已设置的空字符串不会被默认值替换。所有文本采用 UTF-8，Windows 上通过宽字符 API 访问；完整签名、失败和进程范围规则见[系统与环境变量](system_env.md)，组合用法见[示例](../examples/system_env.tx)。

## 时间

导入 [tx/stdlib/time.txh](../tx/stdlib/time.txh)：

| 函数 | 行为 |
| --- | --- |
| unix_millis() -> int | 返回当前 Unix 时间戳，单位毫秒 |
| monotonic_millis() -> int | 返回单调时钟读数，单位毫秒 |
| monotonic_micros() -> int | 返回单调时钟读数，单位微秒 |
| sleep_millis(duration: int) -> void | 至少等待指定的非负毫秒数 |

unix_millis 使用系统时钟，系统时间调整可能让相邻读数倒退。monotonic_millis 和 monotonic_micros 的起点没有日历含义，只适合用两次读数之差测量经过时间；微秒单位不保证实际时钟具有微秒分辨率。sleep_millis(0) 无需等待；负数参数会报运行错误，实际等待时间可能比请求的更长。

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

`input_or_none()` 和 `input_or_none("提示")` 使用同样的读取与提示规则，但在没有更多输入时返回 `none`；读到一行时返回 `str`，静态返回类型为 `any`。可先用 `is_none` 判断，再用 `as str` 取得文本。真正的读取失败和无效 UTF-8 仍报运行错误。

`print()` 输出空行；`print(value, ...)` 输出零个或多个值，默认用空格分隔、末尾换行。可以在值后指定 `sep` 和 `end` 字符串，如 `print("a", 2, sep=", ", end="!\n")`。不支持在 `print` 中使用 `*` 或 `**` 展开。数值和布尔值的文本格式与 `as str` 一致；顶层字符串原样输出，`none` 输出为 `none`。数组显示为 `[元素, ...]`，字典显示为 `{键: 值, ...}`，结构体显示为 `类型名(字段=值, ...)`；嵌套字符串以双引号括起，并转义反斜杠、引号及 ASCII 控制字符。字典不保证输出顺序，结构体保持字段声明顺序。复合值嵌套超过 64 层时报运行错误。`as str` 对复合值的现有转换规则不变。

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

多值输出与 EOF 输入的完整示例见 [print_input.tx](../examples/print_input.tx)。

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
| any 中实际存放 bool | bool | 严格取出布尔值；其他实际类型报运行时类型错误 |

例如 2 as float 输出 2.0，"42" as int 得到 42，3.5 as str 得到 "3.5"。对 none、数组或结构体做这些转换会报运行错误。

静态类型为 `any` 的表达式赋给明确的 `int`、`float`、`str` 目标时，可省略 `as`；显式变量声明、普通赋值和返回值采用相同的转换及错误规则。例如 `array values = [1, 2, 3.2, "666"]` 后，`int number = values[1]`、`float decimal = values[2]`、`str text = values[3]` 分别得到 `2`、`3.2`、`"666"`。静态类型已知的 `int`、`float`、`str` 之间仍需显式使用 `as`。

## 字符串基础操作

先导入 [tx/stdlib/string.txh](../tx/stdlib/string.txh)：

```tx
import "string.txh"
```

| 函数 | 返回值 | 说明 |
| --- | --- | --- |
| len(text) | int | UTF-8 字符数量；len(array) 返回数组长度，len(dict) 返回键数 |
| contains(text, part) | bool | 是否包含子串 |
| starts_with(text, prefix) | bool | 是否以指定文本开头 |
| ends_with(text, suffix) | bool | 是否以指定文本结尾 |
| find(text, part) | int | 首次出现的字符位置，找不到返回 -1 |
| slice(text, start, end) | str | 按字符位置取半开区间 [start, end) |
| replace(text, old, new) | str | 替换所有 old；old 不得为空 |
| split(text, separator) | array | 按非空分隔符拆分，保留空段 |
| split_vector(text, separator) | vector<str> | 直接生成字符串向量，保留空段，分隔符不得为空 |
| join(parts, separator) | str | 接受 array 或 vector<str>；array 中其他元素类型报错 |
| trim(text) | str | 去除首尾 ASCII 空白 |
| lower(text) / upper(text) | str | 转换 ASCII 字母大小写，其他字符保持原样 |

字符位置从 0 开始；slice 要求 0 <= start <= end <= len(text)。字符串使用 UTF-8 保存，以上按字符计数的操作会检查编码有效性。这些函数的实现位于编译得到的标准库静态库中，.txh 只提供接口。完整用法见 [导入与输入示例](../examples/import_io.tx)。

## 字符串格式化

导入 [tx/stdlib/format.txh](../tx/stdlib/format.txh)，调用 `format(template: str, *args: array, **kwargs: dict) -> str`：

```tx
import "format.txh" as fmt

def main() -> int {
    print(fmt.format("你好，{}！", "世界"))
    print(fmt.format("{name}: {score:.2f}", name="Ada", score=9.875))
    print(fmt.format("{name}: {score:.2f}", "Ada", 9.875))
    print(fmt.format("{0:>6} | {1:04d} | {{完成}}", "TX", 7))
    return 0
}
```

- `{}` 按顺序引用位置实参；`{0}`、`{1}` 按下标引用；`{name}` 优先引用同名命名实参。没有同名实参时，`{name}` 按该名称首次出现的顺序消耗下一个位置实参，后续相同名称复用该值。因此 `fmt.format("{name}: {score:.2f}", "Ada", 9.875)` 也得到 `Ada: 9.88`。`{}` 与这种名称回退可混用并共用顺序；它们不能与显式数字编号在同一模板中混用。同一实参可引用多次；多余的实参允许保留不用。名称回退是 TX 相对 Python `str.format` 的扩展。
- `{{` 和 `}}` 分别产生字面量 `{` 和 `}`。孤立的大括号、缺失的实参和不支持的字段语法会报运行错误。
- 字段可写 `!s`（默认）或 `!r`；`!r` 会给顶层字符串加双引号并转义，复合值仍按 `print` 的表示方式展开。
- `:` 后可写格式说明：可选单个 ASCII 填充字符与 `<`、`>`、`^` 对齐符，可选 `+` 或空格数值符号，可选 `0` 零填充、十进制宽度、`.精度` 和类型。宽度按 UTF-8 字符数计算，精度对字符串表示保留的字符数，对浮点类型表示小数位或有效位。宽度与精度的上限各为 1000000。
- 类型 `s` 用于字符串；`d`、`b`、`o`、`x`、`X` 用于整数；`f`、`F`、`e`、`E`、`g`、`G` 用于浮点数。省略类型时使用值的默认文本表示；对字符串可配合精度截断。浮点数指定类型但省略精度时默认 6 位。`0` 零填充用于数字的右对齐，负号和显式正号保持在零之前。

格式化只返回字符串，不输出。类型不匹配、无效格式说明、字段属性或下标访问、嵌套替换字段、无效 UTF-8 都会报可理解的运行错误。目前只覆盖上述 Python 风格常用语法；完整示例见 [format.tx](../examples/format.tx)。
