# XML 拉取读取、文档树与流式写入

`xml.txh` 提供三种互补入口：`reader/next` 逐事件读取大文件，`parse/read` 将小文档构造成可遍历的树，`writer/start_element/write_attribute/write_text/end_element/finish` 逐步生成文档。二进制流按 XML 声明识别 UTF-8、UTF-16LE 和 UTF-16BE；文本流须由调用方先按指定编码解码，传入 XML 模块的是 UTF-8 文本。`stringify` 与写入二进制流输出 UTF-8；写入文本流由流负责转码，模块不写编码声明。

## 名称、节点与安全边界

元素和属性名称分别暴露 `local_name`、`namespace_uri`、`prefix`。URI 是身份，前缀只是文档中的写法；空前缀的元素可属于默认命名空间，空前缀的属性始终不属于默认命名空间。`xml` 前缀仅绑定标准 URI，`xmlns` 前缀保留。写入时由库声明所需命名空间，调用方不得写原始标签、DOCTYPE 或实体定义。属性按 `(namespace_uri, local_name)` 唯一。

`next` 返回 `option<event>`，正常 EOF 为无值。事件有 `kind`、`depth`、名称三元组、`text`、`empty_element` 和 1 起始的 `line/column`。`kind` 为 `start_element/end_element/text/cdata/comment/pi`；XML 声明和命名空间声明不作为独立事件。属性只在 `start_element` 事件上可查询，`attribute_count/attribute_*` 按索引读取当前事件属性，跳过 `xmlns` 声明。调用下一次 `next` 后，先前事件结构仍有效，但 reader 的属性查询转向新事件。树提供 `root/first_child/next_sibling`，节点的名称、内容和属性访问；文本、CDATA、注释和 PI 节点按原顺序保留。

默认拒绝任何 DTD，包括内部子集；不装载或替换外部实体，不访问本地或网络实体，不处理 XInclude。预定义 XML 实体和数字字符引用由 XML 解析器正常解码。树解析和 reader 均不启用恢复模式；遇到格式错误停止，并报告 `parse_error` 的格式名、行、列与可取得的字节位置。由于实体定义被拒绝，实体扩展次数上限为零。

`default_limits()` 返回总输入/输出 1 GiB、元素深度 128、节点 1,000,000、单个文本节点或属性值 8 MiB、单个元素属性 4096 的上限。调用方可调低；上限不得超过 1 TiB、256、10,000,000、8 MiB、65,536。8 MiB 低于固定版本 libxml2 的单文本节点 10,000,000 字节内部限制。超限使用 `parse_error/size_limit` 或 `parse_error/depth_limit`；写入数据超限对应 `runtime_error`。整树解析还要求 `max_bytes <= 64 MiB`，防止无意中将大文件全部载入内存。需要处理大文档时使用 reader。格式错误为 `invalid_syntax`，非法编码为 `invalid_encoding`，禁止的 DTD 为 `forbidden_dtd`，重复属性为 `duplicate_attribute`，无效名称和命名空间组合为 `runtime_error/invalid_name`。底层流错误保留 `io_error`。

`xml_reader/xml_writer/xml_document/xml_node` 是不透明、不可手工构造的句柄。赋值和从 `any` 显式恢复共享状态；`deep_copy` 不复制这些句柄。`close(reader/writer)` 可重复调用，不关闭调用方流。reader 的错误使其失效；writer 的错误使其失效，先前已写字节不回滚。`finish` 验证唯一根元素与标签闭合并刷新流，成功后可重复调用。树节点保留所属文档，即使文档变量离开作用域仍有效；树变更不删除节点，因此已取得的节点引用保持有效。

| 接口 | 行为 |
| --- | --- |
| `reader(stream, limits)`、`next(reader)` | 从二进制或文本流分块读取事件；EOF 返回无值 `option<event>`。reader 存活期间不得交错操作底层流。 |
| `attribute_count/attribute_*(reader, index)` | 查询最近一次 `start_element` 的属性；索引从 0 开始，越界报 `runtime_error/out_of_range`。 |
| `parse(str, limits)`、`read(stream, limits)` | 解析完整树；要求 `max_bytes` 不超过 64 MiB。空文档和格式错误报 `parse_error`。 |
| `root`、`first_child`、`next_sibling` | 遍历文档树；无子节点或下一个兄弟节点返回无值 option。`text(node)` 对元素拼接其后代文本，对文本类节点返回自身内容。 |
| `new_document`、`append_element`、`set_attribute`、`append_text` | 构造可序列化的树；属性同名再次设置会替换原值。命名空间必须通过 URI 和前缀实参提供。 |
| `stringify`、`write(stream, document, limits)` | 校验树的节点、深度、属性和输出字节上限，再生成文档。二进制流使用 UTF-8 声明；文本流不写编码声明，由流编码决定实际字节。 |
| `writer`、`start_element`、`write_attribute`、`write_text`、`end_element`、`finish` | 逐步写入，自动转义文本/属性。属性只能写在当前起始标签内，重复的展开名报 `runtime_error/duplicate_attribute`；失败后 writer 不可继续写入。 |

## 使用方式

小文档用 `parse(text, limits)`、`root` 和 `stringify`；大文件用 `reader(binary_stream, limits)` 逐事件处理，或用 `writer(binary_stream, limits)` 增量输出。UTF-16 可以直接交给二进制 reader，也可以先通过 `text_stream` 解码。示例见 [xml_stream.tx](../examples/xml_stream.tx)。生成到文件可能部分写入；需要原子替换时由调用方使用临时文件和 `file.atomic_replace`。

## 8.3 实施记录

2026-09-27：新增公开接口、libxml2 2.13.8 静态后端、直接 C ABI/LLVM 调用与四类不透明句柄；源码归档 SHA-256 为 `277294cb33119ab71b2bc81f2f445e9bc9435b893ad15bb2cd2b0e859a0ee84a`，许可证随 `tx/LIBXML2-LICENSE` 交付。Windows x64 执行 `pwsh -NoProfile -File scripts/build.ps1 -Incremental` 成功，确认 `txc.exe`、合并的 `libtxstdlib.a`、`xml.txh`、许可证和兼容清单；`build/` 按增量构建模式保留，重复运行时 Ninja 报告无须重编译。

`python scripts/check_xml.py` 通过：TX reader/writer 示例、树访问与构建往返、展开名称和属性、UTF-16 二进制读取、DTD/外部子集拒绝、深度边界、错误后资源状态、`any` 恢复与 `deep_copy` 拒绝、错误实参静态诊断，并由 Python ElementTree 独立读取生成的 XML。上述是本机 Windows x64 的定向验证；未执行跨平台、模糊测试、长时性能测试或第 8.6/13 节总验收。
