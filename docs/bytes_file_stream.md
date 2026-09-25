# 字节值、编码与文件流设计

本文是后续实现契约草案，**`bytes`、内存编码模块和文件流目前均未实现**。当前可用的是[整文件文本接口](standard_library.md#文本文件输入输出)及其 UTF-8、UTF-16、GBK、GB18030 转换。此设计先确定静态类型、复制和共享、EOF、定位、资源关闭与错误语义，再修改编译器及标准库。

## 设计目标

- `bytes` 表示连续原始字节，不借用 `str` 存放未验证的二进制，也不把 `vector<int>` 当作主要字节存储。`str` 仍表示 UTF-8 文本；二者之间必须显式编码或解码。
- 二进制流提供有界分块读取、整流读取、完整写入和随机定位。文本流在已有字符集上提供按字符读取和逐行处理，不要求先把整个文件放入内存。
- 原有 `file.read_text/write_text/append_text` 继续作为整文件便捷接口，现有编码、BOM、错误类别和写入规则保持兼容。文件流属于新增模块，不替换原接口。
- 接口按静态类型区分二进制流与文本流；不会在运行时根据某个字符串参数把 `bytes` 与 `str` 混用。

## `bytes` 的语言类型与值语义

新增内置类型 `bytes`，由运行时的连续 `uint8_t` 存储承载。它是不可变的拥有型值：赋值、传参和返回可以共享同一只读载荷；`concat`、`slice` 等生成新值，不修改原值。不可变性使共享安全，`deep_copy(bytes)` 复用同一只读载荷。切片第一版复制所选字节，避免小切片长期固定整个大文件缓冲。

| 表达式或操作 | 规则 |
| --- | --- |
| `len(data: bytes) -> int` | 返回**字节**数量，范围为非负有符号 64 位整数 |
| `data[index] -> int` | 返回 0～255；下标从 0 开始，越界报运行错误 |
| `data[index] = value` | 不支持；需要修改时先转为 `vector<int>` 再构造新 `bytes` |
| `left == right`、`left != right` | 按全部字节比较；不定义大小排序 |
| `print(data)` | 只显示 `bytes(长度)`，不擅自把内容当作文本输出 |

`bytes` 可作为普通函数参数、返回值、局部变量和结构体/类字段，也可放入 `any`、`array` 和 `dict` 的值位置。未显式初始化的类字段默认空 `bytes`。第一版不把 `bytes` 作为字典键，不提供字节字面量或隐式 `bytes as str`。实现时需同步更新类型解析、语义检查、LLVM 类型和 C ABI、运行时装箱、打印、析构及循环回收边界；只增加 `.txh` 函数并不足以形成这一类型。

建议 `bytes.txh` 提供：

| 函数 | 行为 |
| --- | --- |
| `empty() -> bytes` | 返回空字节值 |
| `from_vector(values: vector<int>) -> bytes` | 复制元素，每个值必须在 0～255 |
| `to_vector(data: bytes) -> vector<int>` | 生成独立整数向量 |
| `concat(left: bytes, right: bytes) -> bytes` | 按顺序拼接 |
| `slice(data: bytes, start: int, end: int) -> bytes` | 半开区间 `[start, end)`，按字节定位 |
| `to_hex(data: bytes) -> str`、`from_hex(text: str) -> bytes` | 小写十六进制输出；解析接受大小写字母，不接受空白，长度必须为偶数 |
| `to_base64(data: bytes) -> str`、`from_base64(text: str) -> bytes` | 严格 RFC 4648 标准字母表和填充，不接受空白或 URL 安全变体 |

非法下标、范围和 0～255 约束报 `runtime_error` / `invalid_argument`；非法 hex/Base64 文本报 `parse_error`，分别使用 `invalid_hex`、`invalid_base64`。空输入可合法产生空 `bytes`。长度、拼接或转换若超出 `int` 可表示范围，报 `runtime_error` / `size_limit`；分配失败沿用运行时错误。

## 内存编码模块

新增 `encoding.txh`，复用现有 [`encoding.cpp`](../src/stdlib/encoding.cpp) 的编码名称及严格转换规则：

```tx
def encode(text: str, encoding: str) -> bytes
def decode(data: bytes, encoding: str) -> str
```

支持 `utf-8`、`utf-8-sig`、`utf-16`、`utf-16le`、`utf-16be`、`gbk`、`gb18030` 及现有别名。`encode` 先校验输入是合法 UTF-8，再编码；`decode` 必须完整消费字节，不能用替代字符静默修复无效序列。`utf-8-sig` 写入 BOM，读取时去掉可选 BOM；`utf-16` 写入小端 BOM，读取时要求有大小端 BOM；显式端序的 UTF-16 行为与当前文件接口一致。未知编码、无效字节或无法表示的字符报告 `parse_error` 与可区分的 `unknown_encoding`、`invalid_encoding`、`unrepresentable_character`。文件流遇到同类数据时对外归入 `io_error`，以维持文件操作的错误约定。

## 文件流的公开类型

建议新增 `file_stream.txh`，用引用语义的 `binary_stream`、`text_stream` 类包装原生文件句柄；两者不能互换。普通赋值和传参共享同一流对象与游标。`close` 使所有别名同时失效；再次 `close` 无害，关闭后读写、定位或刷新报告 `io_error` / `closed_stream`。类的 `deinit` 在最后一个引用消失时兜底关闭；需要确认写入或刷新是否成功时，程序仍应显式调用 `close`。`deep_copy(stream)` 不得复制原生句柄，须在编译期或运行时报明确错误。

```tx
struct byte_chunk
{
    data: bytes
    eof: bool
}

struct text_chunk
{
    text: str
    eof: bool
}

struct line_result
{
    has_line: bool
    line: str
}
```

`byte_chunk.eof` 和 `text_chunk.eof` 只在本次读取**没有取得内容且底层已到文件末尾**时为 `true`；读到最后一段非空内容时先返回 `eof=false`，下一次读取返回空内容且 `eof=true`。这样空内容不会与 EOF 混淆。`line_result.has_line=false` 只表示 EOF；实际空行返回 `has_line=true`、`line=""`。读取故障永不伪装成 EOF。

## 二进制流接口

| 建议签名 | 行为 |
| --- | --- |
| `open_binary(path: str, mode: str) -> binary_stream` | 打开二进制文件 |
| `read_bytes(source: binary_stream, size: int) -> byte_chunk` | 最多读 `size` 字节，`size` 须在 1～8 MiB 之间 |
| `read_all_bytes(source: binary_stream) -> bytes` | 从当前游标读取到 EOF |
| `write_bytes(target: binary_stream, data: bytes) -> void` | 尝试完整写入；失败抛错，文件可能已有部分写入 |
| `tell(source: binary_stream) -> int` | 返回当前字节偏移 |
| `seek(source: binary_stream, offset: int, origin: str) -> int` | 以 `start`、`current` 或 `end` 为基准定位，返回新偏移 |
| `flush(target: binary_stream) -> void` | 把语言层缓冲交给操作系统；不承诺持久化到物理介质 |
| `close(target: binary_stream) -> void` | 刷新并关闭，报告首次关闭时的写入错误 |

`mode` 固定为 `read`、`write`、`append`、`update`：`read` 要求文件存在；`write` 创建或清空；`append` 创建或追加，写入始终在文件尾；`update` 要求文件存在，可读写且不清空，初始游标为 0。`read`/`write`/`append` 分别只允许对应操作，`update` 同时允许读写和定位。`seek` 的结果不得为负，也不得超出 `int` 范围；在追加模式下不允许定位。写入不创建父目录。路径使用严格 UTF-8，Windows 上先转宽字符路径，规则与现有 `file` 模块相同。

`read_bytes` 以独立 `bytes` 返回，后续读取或关闭不改变已返回的数据；允许读取少于请求大小的字节。`read_all_bytes` 适用于确定大小可承受的文件；大文件应循环调用 `read_bytes`。切换读写方向时应明确使用 `seek` 或 `flush` 同步游标与缓冲，避免依赖 C 标准流的未定义组合行为。

## 文本流接口

| 建议签名 | 行为 |
| --- | --- |
| `open_text(path: str, mode: str, encoding: str) -> text_stream` | 按指定编码打开文件；模式限 `read`、`write`、`append` |
| `read_chars(source: text_stream, count: int) -> text_chunk` | 最多读 `count` 个 Unicode 标量值，`count` 须在 1～1,048,576 之间 |
| `read_line(source: text_stream, max_bytes: int) -> line_result` | 读一行，返回不含行尾符号的 UTF-8 文本；`max_bytes` 限 1～8 MiB |
| `write_text(target: text_stream, text: str) -> void` | 按指定编码写入，不改变换行符 |
| `write_line(target: text_stream, text: str) -> void` | 写入文本后追加单个 `\n` |
| `flush(target: text_stream) -> void`、`close(target: text_stream) -> void` | 刷新或关闭 |

`read_chars` 的计数单位与现有 `str` 长度一致，为 Unicode 标量值，不是字节或字素簇。增量解码器须保存跨读取边界的 UTF-8/UTF-16/GBK/GB18030 部分序列；不能因为一个分块恰好截断字符就报错。`read_line` 的上限按解码后的 UTF-8 字节数计算，不含行尾符号；超过上限时关闭该文本流并报告 `io_error` / `size_limit`，避免把残余半行误当作新行。`read_line` 识别 `\n`、`\r\n` 和 `\r`，返回时移除行尾符号；文件末尾尚有内容但没有行尾时仍返回最后一行，下一次才返回 `has_line=false`。可交替调用 `read_chars` 与 `read_line`，二者共享同一解码游标。

文本流不提供按字节 `seek/tell`，避免在多字节字符和增量解码器状态中产生含糊定位。`utf-8-sig` 与 `utf-16` 的 BOM 规则沿用整文件接口；追加到非空文件时检查现有 BOM 并沿用其端序，不能在中间写入第二个 BOM。`open_text(..., "write", ...)` 在打开时清空目标文件，这是流式写入与现有整文件 `file.write_text` 先完成编码再打开文件的区别；后续编码或磁盘错误可能留下空文件或部分内容。`read_line` 的显式上限与 `read_chars` 的单次计数上限防止无界累积不可信输入。

## 错误、关闭与并发边界

- 文件缺失、权限、路径、编码、短写入、关闭和刷新错误统一为 `error.io_error`；建议区分 `not_found`、`permission_denied`、`invalid_path`、`invalid_mode`、`invalid_encoding`、`closed_stream`、`invalid_seek`、`size_limit`、`operation_failed`。`message` 为中文说明，程序逻辑依据 `code`。
- 对一个流的读写和关闭不提供跨线程并发保证。若后续语言增加线程，需要定义共享游标锁或要求调用方同步。外部进程同时修改文件时，不保证读取快照或写入事务。
- 异常离开作用域时，`deinit` 负责释放资源，但清理错误不应覆盖原始错误；需要可靠获知写入失败应显式 `flush`、`close`。关闭失败仍应标记流不可继续使用，避免重复写入。
- 无论手动关闭、正常析构、循环回收或错误退出，原生句柄最多关闭一次。循环引用可能推迟 `deinit`，因此长时间运行的程序要在使用后及时 `close`。

## 与网络模块的关系和实施顺序

[网络模块设计](network.md)的第一阶段正文和 WS 消息是 UTF-8 文本。`bytes` 落地后可增加 `httpx.send_bytes`、响应二进制正文和 `ws.send_binary/receive_binary`；这些是后续新签名，不改变已有文本接口的含义。文件流可为网络上传下载提供分块来源与目标，但真正的流式背压、取消和连接关闭规则需在网络实施时单独明确。

建议先实现 `bytes` 的语言类型、运行时表示与编码转换，再实现二进制流，最后加入文本增量解码与逐行处理。每阶段同步 `.txh`、文档和示例，并仅做对应的定向验证：字节边界、非法编码、EOF/空行、读写定位、BOM、关闭与错误清理。**本轮只交付文档，尚无这些接口的代码或测试结果。**
