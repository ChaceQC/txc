# 字节值、编码与文件流

本文记录 `bytes`、内存编码模块与文件流的接口和语义。代码及 `.txh` 接口已接入编译器和标准库，C++ 构建与少量定向场景已通过。原有[整文件文本接口](standard_library.md#文本文件输入输出)继续可用。

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

`bytes` 可作为普通函数参数、返回值、局部变量和结构体/类字段，也可放入 `any`、`array` 和 `dict` 的值位置，或作为 `vector<bytes>` 的元素。`array`、`dict` 中的字节值按原有异构值规则装箱，可用 `as bytes` 或目标类型为 `bytes` 的赋值取回；`vector<bytes>` 保留完整静态元素类型，容器独立复制时继续共享不可变字节载荷。未显式初始化的类字段默认空 `bytes`。第一版不把 `bytes` 作为字典键，不提供字节字面量或隐式 `bytes as str`。

[bytes.txh](../tx/stdlib/bytes.txh) 提供：

| 函数 | 行为 |
| --- | --- |
| `empty() -> bytes` | 返回空字节值 |
| `from_vector(values: vector<int>) -> bytes` | 复制元素，每个值必须在 0～255 |
| `to_vector(data: bytes) -> vector<int>` | 生成独立整数向量 |
| `concat(left: bytes, right: bytes) -> bytes` | 按顺序拼接 |
| `slice(data: bytes, start: int, end: int) -> bytes` | 半开区间 `[start, end)`，按字节定位 |
| `to_hex(data: bytes) -> str`、`from_hex(text: str) -> bytes` | 小写十六进制输出；解析接受大小写字母，不接受空白，长度必须为偶数 |
| `to_base64(data: bytes) -> str`、`from_base64(text: str) -> bytes` | 严格 RFC 4648 标准字母表和填充，不接受空白或 URL 安全变体 |
| `to_base64_url(data: bytes) -> str`、`from_base64_url(text: str) -> bytes` | RFC 4648 URL 安全字母表，输出不带 `=`；解析只接受规范的无填充形式，不接受标准字母表或空白 |
| `to_hex_chunk(data: bytes, start: int, end: int, max_chars: int) -> str` | 按字节半开区间编码十六进制；输出长度不得超过 `max_chars` |
| `to_base64_chunk(data: bytes, start: int, end: int, max_chars: int) -> str` | 按字节半开区间编码标准 Base64；`start` 必须为 3 的倍数，`end` 必须为 3 的倍数或整个字节值的末尾，依次拼接结果等于整值编码 |

非法下标、范围和 0～255 约束报 `runtime_error` / `invalid_argument`；非法 hex/Base64 文本报 `parse_error`，分别使用 `invalid_hex`、`invalid_base64`。空输入可合法产生空 `bytes`。长度、拼接或转换若超出 `int` 可表示范围，报 `runtime_error` / `size_limit`；分配失败沿用运行时错误。

两个分块编码函数的 `max_chars` 以输出 ASCII 字符计，取值为 0～1,048,576；超出该值或所选分块的输出超过该限额时报告 `runtime_error` / `size_limit`。范围和 Base64 对齐错误报告 `runtime_error` / `invalid_argument`。空区间可与 `max_chars=0` 搭配，返回空串。编码函数始终显式接收 `bytes`；不会把任意字节隐式当作 `str`。

## 内存编码模块

新增 [encoding.txh](../tx/stdlib/encoding.txh)，复用现有 [`encoding.cpp`](../src/stdlib/encoding.cpp) 的编码名称及严格转换规则：

```tx
def encode(text: str, encoding: str) -> bytes
def decode(data: bytes, encoding: str) -> str
```

支持 `utf-8`、`utf-8-sig`、`utf-16`、`utf-16le`、`utf-16be`、`gbk`、`gb18030` 及现有别名。`encode` 先校验输入是合法 UTF-8，再编码；`decode` 必须完整消费字节，不能用替代字符静默修复无效序列。`utf-8-sig` 写入 BOM，读取时去掉可选 BOM；`utf-16` 写入小端 BOM，读取时要求有大小端 BOM；显式端序的 UTF-16 行为与当前文件接口一致。未知编码、无效字节或无法表示的字符报告 `parse_error` 与可区分的 `unknown_encoding`、`invalid_encoding`、`unrepresentable_character`。文件流遇到同类数据时对外归入 `io_error`，以维持文件操作的错误约定。

内存接口的一次转换受 Windows 字符集 API 的有符号 32 位长度参数限制；超出该上限报告 `runtime_error` / `size_limit`。大文件应使用下文的分块文件流。

第五部分扩展 `encoding` 的内存接口：`utf-32`、`utf-32le`、`utf-32be` 及去掉连字符的别名均受支持。`utf-32` 写入小端 BOM，读取时要求小端或大端 BOM；显式端序写入时不加 BOM，读取时可去掉匹配的 BOM，反向 BOM、代理项和超出 U+10FFFF 的码点报 `parse_error` / `invalid_encoding`。`file.read_text/write_text` 共享这一转换规则；`file_stream.open_text` 暂不接受 UTF-32，调用方可使用下面的增量解码器配合二进制流。

增量接口使用内置不透明类型 `encoding_decoder`、`encoding_encoder`，赋值与传参共享同一状态，`deep_copy` 不复制状态。`policy` 必须显式为 `strict` 或 `replace`；`replace` 解码时插入 U+FFFD，编码时采用目标编码的替代字节，并能通过计数接口查询替代次数。输入 `str` 自身必须是完整有效的 UTF-8，编码器不会接收残缺文本片段。

| 函数 | 行为 |
| --- | --- |
| `new_decoder(encoding: str, policy: str) -> encoding_decoder` | 创建增量解码器；支持现有编码及 UTF-32 |
| `decode_chunk(source: encoding_decoder, data: bytes, eof: bool) -> str` | 消费一个字节块；内部保存跨块的 UTF-8/UTF-16/UTF-32/GBK/GB18030 残缺序列；`eof=true` 才检查最后残缺序列并结束状态 |
| `decoder_replacements(source: encoding_decoder) -> int` | 自创建以来的替代次数 |
| `new_encoder(encoding: str, policy: str) -> encoding_encoder` | 创建增量编码器 |
| `encode_chunk(target: encoding_encoder, text: str, eof: bool) -> bytes` | 消费一段完整 UTF-8 文本；BOM 只写一次；`eof=true` 刷新并结束状态 |
| `encoder_replacements(target: encoding_encoder) -> int` | 自创建以来的替代次数 |

每次块输入最多 8 MiB，单次返回最多 32 MiB，超出时报 `runtime_error` / `size_limit`；非法策略报 `parse_error` / `invalid_policy`，严格模式下非法或 EOF 残缺序列报 `parse_error` / `invalid_encoding`，完成后的重复调用报 `runtime_error` / `closed_codec`。严格错误会使该编解码器失效，避免继续使用部分更新后的状态。`utf-8-sig` 的 BOM 可跨输入块识别；`utf-16` 与 `utf-32` 的 BOM 规则与内存接口一致。

## 文件流的公开类型

[file_stream.txh](../tx/stdlib/file_stream.txh) 使用内置不透明引用类型 `binary_stream`、`text_stream` 包装原生文件句柄；两者不能互换。普通赋值和传参共享同一流对象与游标。`close` 使所有别名同时失效；再次 `close` 无害，关闭后读写、定位或刷新报告 `io_error` / `closed_stream`。最后一个引用消失时运行时兜底关闭；需要确认写入或刷新是否成功时，程序仍应显式调用 `close`。`deep_copy(stream)` 报明确运行错误，不复制原生句柄。

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

内部读取使用 64 KiB 分块，结果按实际读到的字节增长；整流读取复用同一分块缓冲，短文件和 EOF 不再按 8 MiB 分配、清零临时字符串。读取上限、读写方向切换、关闭与 EOF 的公开语义保持。

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

内存 UTF-8 编解码使用严格的 Unicode 标量校验，不再为校验构造 UTF-16 副本；编码直接写入结果字节容器。编码名为已知字面量时，编译器提前解析编码枚举，动态编码名仍在调用时解析。BOM、非法序列与不可表示字符的原有错误分类保持。

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

## 7.3 整文件二进制、原子替换与文件锁

`file.txh` 增加 `read_bytes(path) -> bytes`、`write_bytes(path, data)`、`atomic_write_bytes(path, data)` 和 `atomic_write_text(path, text, encoding)`。整文件读取适合可放入内存的小文件；大文件继续使用分块流。普通 `write_bytes` 创建或清空文件，写入失败可能留下前缀。文本原子写入先按现有编码规则完整编码，编码失败不改动目标。

两个原子写入接口在目标目录内用不可预测名称、排他创建临时文件，完整写入并刷新该文件，再用同卷重命名替换目标。提交前失败会尝试删除临时文件，目标保持原样；失败时绝不报告成功。替换成功后，新打开的读者看到完整新文件，已打开的句柄仍可能看到旧内容。目录、共享模式、权限、链接或文件系统不支持替换时返回 `io_error`，不回退为复制覆盖；替换可能改变目标的原有权限、属性和文件标识。临时文件与目标同目录，因而正常路径不跨卷；符号链接目标被替换的是目录项本身，不能把路径检查当作安全边界。文件内容在替换前显式刷新，但 Windows 不提供本接口可确认的目录元数据持久化承诺；断电后不能保证替换记录已落盘。

`file_stream.txh` 增加 `sync(binary_stream/text_stream)`、`open_binary_shared(path, mode) -> binary_stream`、`lock(binary_stream, mode)`、`try_lock(binary_stream, mode) -> bool` 和 `unlock(binary_stream)`。原 `open_binary` 的系统共享打开行为保持不变；协作进程或多个句柄要参与文件锁时使用 `open_binary_shared`。共享打开不改变 TX 流对象的别名和游标语义，同一流状态的操作仍不提供跨线程并发保证。`flush` 只刷新 C 运行时缓冲；`sync` 再调用系统的文件缓冲刷新，要求可写流，成功表示系统已接受持久化请求，不保证磁盘硬件或远端存储在断电后仍保留数据。`mode` 只接受 `shared` 或 `exclusive`：共享锁要求可读流，独占锁要求可写流；锁住当前文件的整个字节范围。`lock` 等待其他协作方释放，`try_lock` 遇到锁冲突返回 `false`，其他故障抛错。一个流状态最多持有一把锁，别名共享锁状态；重复加锁和无锁解锁报 `io_error/invalid_state`。关闭流自动释放锁，关闭后锁操作报 `io_error/closed_stream`。锁只约束遵守同机锁协议的访问者，不提供事务、跨机器互斥或线程同步；关闭、崩溃与进程退出均由系统释放锁。

新接口使用 `io_error`，并按情况给出 `not_found`、`permission_denied`、`invalid_path`、`invalid_encoding`、`invalid_mode`、`closed_stream`、`invalid_state`、`size_limit`、`operation_failed`。普通写入和 `sync` 失败时内容可能已部分写入；原子写入提交失败时极端外部故障下应重新读取目标确认结果。`sync` 不替代原子替换，原子替换也不替代应用层锁和事务。

原来 `file.write_text("配置.txt", content, "utf-8")` 适合允许目标在失败时出现部分更新的场景；需要完整替换时改用 `file.atomic_write_text("配置.txt", content, "utf-8")`。已有流写入的 `flush` 只交出进程内缓冲；需要请求操作系统持久化时，在写入后调用 `file_stream.sync(writer)`，再显式 `close(writer)`。锁的典型流程是先用 `file_stream.open_binary_shared(path, "update")` 打开，再在 `file_stream.try_lock(writer, "exclusive")` 成功后写入，最后调用 `unlock(writer)` 或 `close(writer)`；整个流程的异常路径仍须确保关闭流。

## 与网络模块的关系和实施顺序

[网络模块](network.md)提供文本、`bytes` 和文件流三套接口：`httpx.send_bytes`、二进制请求/响应，`websocket.send_binary/receive_binary`，以及 HTTP/1.1、HTTP/2 和 WebSocket 的 `binary_stream` 分块接口。原有文本接口仍要求有效 UTF-8，整条正文或消息的内存接口仍限 8 MiB。流式接口从源流当前位置按长度读入、向目标流逐块写出，调用方设置接收上限；网络同步读写提供背压。发生错误后目标文件可能保留已写入的前缀，网络调用不替调用方关闭或回滚文件流。完整签名与限制见网络模块说明。

代码已按 `bytes` 与编码、二进制流、文本流的顺序接入。[组合示例](../examples/bytes_file_stream.tx) 展示 `vector<bytes>`、`array`/`dict` 值、编码与两种文件流。

2026-09-26 定向验证：`scripts/build.ps1` 构建通过并更新 `tx/txc.exe`、`tx/libtxstdlib.a`，成功后清理 `build/`。组合示例通过；[边界场景](../tests/bytes_file_stream/behavior.tx)通过非法 Hex/Base64、越界字节、无效 UTF-8、关闭别名、EOF、空行及字符与逐行混合读取；[编码场景](../tests/bytes_file_stream/encodings.tx)通过七种编码的内存转换与增量文本流。未运行全量回归或性能测试。
