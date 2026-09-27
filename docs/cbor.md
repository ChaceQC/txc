# CBOR 编解码与分块流

`cbor.txh` 按 RFC 8949 的核心确定性编码处理 TX 的通用数据值。二进制输入、输出只使用 `bytes` 或 `binary_stream`；它不经 JSON 文本中转。结构体的版本化字段映射属于后续 8.5 的 `serde`，此模块不推断结构体布局。

## 值与规范化规则

| TX 值 | CBOR 表示 |
| --- | --- |
| `int` | 主类型 0/1；只接受 TX 有符号 64 位范围 |
| `float` | IEEE 754 最短无损的 half、single 或 double；NaN 固定为 `f9 7e 00`，保留有符号零 |
| `str` / `bytes` | 严格 UTF-8 文本 / 原始字节串，互不隐式转换 |
| `array` / `dict` | 确定长度数组 / 映射 |
| `bool` / `none` | `false`、`true` / `null` |

映射键支持 TX `dict` 可表示的 `int`、有限 `float`、`str`、`bool` 和 `none`。字节串键及复合键无法放入 TX `dict`，解码时拒绝。普通浮点值允许 ±Inf 和 NaN，并使用最短确定性表示。映射按编码后的键长度、再按无符号字节序排列；同一 TX 键或同一编码键重复均报 `duplicate_key`。编码拒绝循环引用和不能表示的运行时对象，包括句柄、函数与结构体。解码要求最短整数及长度头、最短无损浮点、上述键顺序；拒绝无限长项、标签、未定义值、未分配简单值和多余根数据，不悄悄规范化非法输入。

## 接口与限额

`limits` 含 `max_bytes`（整个编解码输入或输出）、`max_value_bytes`（完整值或流中单个元素）、`max_depth`（根值从 0 起算）、`max_items`（单个数组或映射的元素数）。`default_limits()` 返回 1 GiB、16 MiB、128、100 万；配置上限为 1 TiB、64 MiB、128、100 万，均须为正数。长度头在分配前校验；输出边生成边检查。大数据可通过降低单值上限来限制驻留内存。

| API | 行为 |
| --- | --- |
| `encode(value: any, limits) -> bytes` / `decode(data: bytes, limits) -> any` | 一个完整值；解码必须消费全部输入。 |
| `read(source: binary_stream, limits) -> any` / `write(target: binary_stream, value: any, limits) -> void` | 从流读一个完整值并检查 EOF，或分块写一个值。 |
| `reader(source: binary_stream, limits) -> cbor_reader` / `next(source: cbor_reader) -> option<any>` | 根值必须是确定长度数组，逐项解码。读取最后一项后继续调用 `next` 以确认 EOF；合法 `null` 是有值的 option。 |
| `writer(target: binary_stream, count: int, limits) -> cbor_writer` / `write_value(target: cbor_writer, value: any)` / `finish(target: cbor_writer)` | 先写确定长度根数组头，再逐项生成；元素数量不符时 `finish` 报错。成功 `finish` flush，重复调用无操作。 |
| `close(reader/writer)` | 重复关闭无操作；writer 关闭不补写、不提交。 |

流操作使用固定大小读写缓冲，不持有已消费的文件前缀；仅当前元素和当前映射的键排序索引占用与其大小相应的内存。游标共享底层流，存活期间调用方不得交错读写或 seek。错误使游标失效，之后操作报 `runtime_error/invalid_state`；关闭或最后引用释放资源。文件流被外部关闭时保留原 `io_error`。输出失败可能留下前缀，需要原子保存时先写临时文件再替换。`write` 不负责 flush。

非法输入抛 `parse_error`，信息含 CBOR 字节偏移；稳定代码为 `empty_input`、`invalid_syntax`、`non_canonical`、`invalid_utf8`、`duplicate_key`、`type_mismatch`、`out_of_range`、`depth_limit`、`size_limit`。非法选项与不可编码值分别使用 `runtime_error/invalid_argument` 和 `runtime_error/unsupported_type`，循环引用为 `runtime_error/cyclic_value`。无需外部依赖：该受限核心格式使用仓库内实现，不引入 QCBOR 静态库。

## 8.4 实施记录

2026-09-27：新增公开 `.txh`、标准库编解码与流游标、直接 C ABI/LLVM 调用和不透明句柄的类型恢复/清理规则。构建使用 `pwsh -NoProfile -File scripts/build.ps1 -Incremental`，Windows x64 `txc.exe`、标准库静态库和兼容指纹已更新，`build/` 保留供本轮增量构建。

定向验证使用 `tests/cbor/behavior.tx` 和 `tests/cbor/behavior_native.cpp`：固定规范编码向量、TX 类型恢复与完整值/流读写、1 字节分块的 2 万元素数组、重复键/乱序键/非最短编码/无效 UTF-8/截断/超范围整数、深度/长度上限、失败后状态及源码位置类型诊断均通过。未运行全库测试、跨平台或模糊测试；schema 版本迁移和跨格式联动属于 8.5～8.6。
