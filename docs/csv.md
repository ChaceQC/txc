# CSV 流式读写

`csv.txh` 提供 `reader`、`next_row`、`header`、`writer`、`write_row`、`finish`、`close`，以及 `parse/stringify` 整文本接口。输入源与输出目标可为 `binary_stream` 或 `text_stream`；二进制流严格按 UTF-8 处理，其他编码须显式使用文本流转换。所有字段均为 `str`，不自动猜测数字、布尔或公式类型。

## Dialect 与限额

`default_dialect()` 返回以下设置；在创建游标时复制配置，之后修改配置不影响已有游标。

| 字段 | 默认值与规则 |
| --- | --- |
| `delimiter` | `","`；一个 ASCII 字符，不得为引号、CR、LF 或控制字符（允许 TAB 分隔）。 |
| `quote` | `"\""`；一个可打印 ASCII 字符，与分隔符不同。字段中的引号使用双写转义。 |
| `newline` | `"\r\n"`，遵循 RFC 4180；也可显式选择 `"\n"` 或 `"auto"`。auto 读取接受 CRLF/LF，写入使用 CRLF。裸 CR 在引号外始终拒绝。 |
| `has_header` | `false`。true 时 reader 消费第一条记录作为表头；writer 的第一次 write_row 写表头。表头必须存在、列名非空且不重复；后续行须与其等宽。 |
| `strict_width` | `true`。无表头时第一行固定后续行宽；false 允许不等宽的无表头行。 |
| `allow_bom` | `false`。true 时仅消费文件起始的可选 UTF-8 BOM；其他位置的 U+FEFF 是字段内容。 |
| `write_bom` | `false`。true 时 writer 在首行前写 UTF-8 BOM。配合文本流时应使用不自行写 BOM 的编码，避免双 BOM。 |
| `max_field_bytes` | 1 MiB，最大 16 MiB，按解引号后的 UTF-8 字节计数。 |
| `max_row_bytes` | 16 MiB，最大 64 MiB，包含分隔符、转义引号和行结束符的编码字节数。 |
| `max_columns` | 4096，最大 65536。 |
| `max_bytes` | 1 GiB，最大 1 TiB，包含 BOM 与所有记录。 |
| `max_rows` | 1000000，最大 1000000000，包含表头。 |

所有限额须为正数；不支持的配置报 `runtime_error/invalid_argument`。文本流的计数与错误偏移均对应解码后的 UTF-8。

## 接口与数据规则

| 接口 | 行为 |
| --- | --- |
| `reader(stream, dialect) -> csv_reader` | 创建逐行游标；有表头时立即读取并验证表头。 |
| `next_row(reader) -> option<vector<str>>` | 逐条返回数据行，EOF 为无值 option。引号内可以含 CR/LF，不会提前截断行。 |
| `header(reader) -> vector<str>` | 返回独立的表头快照；无表头返回空向量。 |
| `writer(stream, dialect) -> csv_writer` | 创建写入游标。 |
| `write_row(writer, row: vector<str>) -> void` | 必要时自动加引号；每行先检查字段、行宽、字节限额，再交付该行。底层写入仍可能部分成功。 |
| `finish(writer) -> void` | 验证必需表头已写入并 flush；重复成功 finish 无操作，之后不能再追加。 |
| `close(reader/writer) -> void` | 重复关闭无操作；释放游标和其流引用，不关闭调用方的流，也不代替 finish。 |
| `parse(text, dialect) -> vector<vector<str>>` | 使用同一解析器返回全部记录，包含配置为表头的首行；限额与流式接口相同。 |
| `stringify(rows: vector<vector<str>>, dialect) -> str` | 使用同一 writer 生成全部记录；首行在 has_header=true 时为表头。 |

空文件表示零条记录；要求表头时空文件报 `missing_header`。空物理行表示一个空字段，逗号表示两个空字段，行尾逗号保留末尾空字段。最后一行允许不带结束换行；记录结尾换行不额外制造空记录。writer 不接受零字段向量，单个空字段输出为 `""` 加换行。字段内前后空白原样保留，闭引号与分隔符之间的额外字符一律拒绝；字段中的 NUL 也拒绝。

游标赋值与从 `any` 显式恢复共享状态；禁止手工构造、deep_copy、跨线程传递。读取只保留预读块、表头和当前记录，内存不随记录数增长。游标存活期间不得交错操作底层流。发生错误后游标失效，后续操作报 `invalid_state`；已返回/写出的记录不回滚，close 或最后引用销毁释放资源。

格式错误使用 `parse_error/invalid_syntax`，非法 UTF-8 使用 `invalid_utf8`，BOM 拒绝使用 `unexpected_bom`，字段/行/列/总量/行数超限使用 `size_limit`，行宽不符使用 `row_width`，无效表头使用 `invalid_header`。解析消息含 CSV 格式名、1 起始行/Unicode 标量列、0 起始 UTF-8 字节偏移，不回显原始字段。writer 的数据错误使用同名 `runtime_error` 码；流的外部错误仍为 `io_error`。

小数据可用 `parse/stringify`，大文件用 [csv_stream.tx](../examples/csv_stream.tx) 中的逐行循环。需要原子保存时写临时文件再替换；这里不承诺多行事务或故障回滚。

## 8.2 实施记录

2026-09-27：`.txh`、reader/writer 原生状态机、直接 C ABI/LLVM 调用、类型规则、示例和错误码登记已交付；`vector<str>` 字段沿用类型化字符串载荷，不通过 JSON 文本或异构 array 解析中转。配置为快照，表头和数据行均不保留游标反向引用。未新增第三方依赖。

`pwsh -NoProfile -File scripts/build.ps1` 完整构建 Windows x64 工具链和标准库成功；兼容指纹包含 `csv.txh`，产物留在 `tx/`，`build/` 已清理。`python scripts/check_data_formats.py csv` 通过：

- `tests/csv/stream.tx` 与示例：二进制/UTF-16 文本流、表头快照、类型化行、BOM、多行字段、双写引号、空字段、分号/单引号/LF dialect、嵌套向量整文本往返、游标共享与 any 恢复、关闭后失效及 deep_copy 拒绝。
- `tests/csv/stream_native.cpp`：1/2/7/4096 字节分块、跨块 Unicode/BOM/CRLF/双引号、空文件和空行、最后无换行、尾部空字段、重复/缺失表头、非法编码/语法、字段/行/列/总量/行数上限、全局位置、持续生成 20000 条记录（单行限额 16 字节）、半写入、失败状态和引用释放。
- 错误实参在 TX 源码位置报静态诊断；生成文件由 Python `csv.reader(strict=True)` 独立解析并核对字段。

8.2 的接口及上述定向验收已完成；未运行全库测试、跨平台验收、模糊测试或性能基准。XML/CBOR/serde 与第 8.6、13 节的终态交叉验收仍独立推进。
