# JSON 模块

导入 `json.txh` 后，可以把 UTF-8 JSON 文本转换为 TX 的基础值，也可以把受支持的值序列化为 JSON。接口和示例分别见 [json.txh](../tx/stdlib/json.txh) 与 [json.tx](../examples/json.tx)。本模块使用现有 `any`、`array`、`dict` 和 [错误结果](errors_and_parse.md)，没有引入新的语言类型。

## 值的对应关系

| JSON | TX 中的值 |
| --- | --- |
| `null` | `none` |
| `true`、`false` | `bool` |
| 无小数点、无指数且在有符号 64 位范围内的数字 | `int` |
| 含小数点或指数的有限数字 | `float` |
| 字符串 | `str`，保存为 UTF-8 |
| 数组 | `array` |
| 对象 | `dict`，字段名为 `str` |

顶层可以是上述任意值。整数超出 `int` 范围或浮点数超出有限 `float` 范围时报解析错误，不静默转成另一种数值类型。`-0` 解析为整数零；浮点数序列化时会保留小数点或指数，使再次解析时仍是 `float`。同一对象中重复字段名采用最后一次出现的值。解析所得数组和字典遵循 TX 现有的共享引用规则。

## 解析

| 函数 | 行为 |
| --- | --- |
| `parse(text: str) -> any` | 解析完整 JSON，失败时抛出可由 `error.parse_error` 捕获的错误 |
| `parse_object(text: str) -> dict` | 解析根对象并直接返回 `dict`；根值不是对象时报 `parse_error` / `type_mismatch` |
| `try_parse(text: str) -> error.any_result` | 成功时 `ok=true`、`value` 为解析值；格式失败时 `ok=false`、`value=none`、`error` 记录原因 |

`error.any_result` 的字段依次为 `bool ok`、`any value`、`error.error_info error`。`try_parse("null")` 成功且值为 `none`，因此必须检查 `ok` 才能区分合法空值与失败。格式错误的 `error.kind` 为 `parse_error`，`code` 使用 `empty_input`、`invalid_syntax`、`invalid_escape`、`invalid_utf8`、`out_of_range` 或 `depth_limit`。错误消息含从 1 开始的行、列及从 0 开始的 UTF-8 字节偏移；列按 Unicode 标量计数。分配失败等运行时故障仍走运行时错误路径。

解析遵循严格 JSON 语法：只跳过空格、制表符、回车与换行；不接受注释、尾随逗号、前导零、`NaN`、`Infinity`、BOM 和未转义控制字符。字符串支持标准转义与成对的 UTF-16 代理项转义；原始文本必须为有效 UTF-8。嵌套最多 128 层。

## 序列化

| 函数 | 行为 |
| --- | --- |
| `stringify(value: any) -> str` | 返回紧凑 JSON |
| `stringify_pretty(value: any, indent: int) -> str` | 在数组和对象内换行，每层缩进 `indent` 个空格 |

缩进宽度可为 0～16。对象字段按 UTF-8 字节序输出，使同一对象的文本稳定；数组保持元素顺序。字符串按 JSON 规则转义，非 ASCII 的有效 UTF-8 原样保留。序列化只接受上表中的值，且 `dict` 的所有键必须为 `str`。无效 UTF-8、非有限 `float`、不支持的值、循环引用、超深嵌套或非法缩进会产生可捕获的 `runtime_error`，不会输出部分结果。共享但没有形成循环的数组或字典可以重复出现。

通常的配置文件流程直接使用 `parse_object`。嵌套值从字典或数组中读出时，可以直接赋给明确类型的 `dict` / `array` 变量，由语言在运行时检查并保留共享引用。数组作为根值时也可写 `array values = json.parse(text)`；实际类型不符时报可捕获的 `runtime_error`。

## 字段读取

以下函数的 `object` 参数为 `dict`，`key` 参数为 `str`：

| 函数 | 返回值与规则 |
| --- | --- |
| `contains(object, key) -> bool` | 检查字段是否存在，字段值为 `null` 时仍返回 `true` |
| `get(object, key) -> any` | 返回原值，可用 `is_none` 检查 `null` |
| `get_int`、`get_bool`、`get_str` | 只接受对应类型，返回相应静态类型 |
| `get_float` | 接受 `float` 或 `int`，后者转为 `float`，可能损失整数精度 |
| `get_array`、`get_object` | 只接受 `array` 或 `dict`，返回共享引用 |

除 `contains` 外，缺少字段时报 `parse_error` / `missing_field`；类型不符时报 `parse_error` / `type_mismatch`。错误消息包含字段名。`get` 返回 `none` 时，该字段确定存在；缺失字段与合法 `null` 不混淆。字段读取不会更改对象。

## 使用示例

下面是常见的文件配置流程。`parse_object` 得到普通 `dict` 后，可以直接索引、修改和新增字段；读取字段所得静态类型为 `any`，赋给 `str`、`int` 等明确类型时会进行运行时检查。

```tx
import "file.txh" as file
import "json.txh" as json

def main() -> int
{
    str text = file.read_text("config.json", "utf-8")
    dict config = json.parse_object(text)

    str name = config["name"]
    int retries = config["retries"]
    dict server = config["server"]
    array tasks = config["tasks"]
    dict first_task = tasks[0]
    print(name, retries, server["host"], first_task["name"])

    config["retries"] = retries + 1
    config["enabled"] = true
    server["port"] = 8081
    first_task["enabled"] = false
    file.write_text("config.updated.json", json.stringify_pretty(config, 2), "utf-8")
    return 0
}
```

这里的 `config.json` 需要存在，且内容是 JSON 对象，例如：

```json
{
  "name": "TX",
  "retries": 2,
  "enabled": false,
  "server": {"host": "127.0.0.1", "port": 8080},
  "tasks": [
    {"name": "compile", "enabled": true},
    {"name": "format", "enabled": false}
  ]
}
```

`server` 和 `first_task` 是共享引用，直接修改它们后，`config` 序列化时会包含这些更新。输出写到 `config.updated.json`，输入文件不会被覆盖。缺少字段仍遵循字典索引的运行错误规则；若需要判断字段是否存在，使用 `json.contains`。文件读写错误属于 `io_error`，JSON 解析错误属于 `parse_error`。

## 增量读写与 schema

`limits` 的三个字段为 `max_bytes`（整个输入/输出）、`max_value_bytes`（单个根值或根数组元素）和 `max_depth`。`default_limits()` 返回 1 GiB、16 MiB、128；可配置硬上限分别为 1 TiB、64 MiB、128，均须为正数。文本流的字节数和偏移指解码后的 UTF-8，二进制流按原始 UTF-8 计数。旧 `parse/stringify` 接口保持原有契约。

| 接口 | 规则 |
| --- | --- |
| `read(source: binary_stream/text_stream, limits) -> any` | 分块读取一个完整根值并检查 EOF；返回值占用与该值大小相应的内存，不另存整个输入文本。 |
| `reader(source: binary_stream/text_stream, limits) -> json_reader` | 打开根数组游标，按需读取，不预读整个数组。 |
| `next(source: json_reader) -> option<any>` | 返回一个完整元素；合法 `null` 是有值的 option，其 value 为 none；数组闭合并确认 EOF 后才返回无值。 |
| `write(target: binary_stream/text_stream, value: any, limits) -> void` | 以有界 UTF-8 块生成一个完整根值，不生成完整中间 JSON 字符串。 |
| `writer(target: binary_stream/text_stream, limits) -> json_writer` | 开始写根数组。 |
| `write_value(target: json_writer, value: any) -> void` | 追加一个元素，对象仍按 UTF-8 键排序。 |
| `finish(target: json_writer) -> void` | 写入 `]` 并 flush；成功后重复 finish 无操作。 |
| `close(target: json_reader/json_writer) -> void` | 释放游标状态，重复关闭无操作；writer 的 close 不补写、不提交。 |
| `validate(value: any, schema: dict) -> void` | 显式执行下述 schema 校验，失败抛出 parse_error。 |

游标持有流的共享引用，其存活期调用方不得交错读取、写入或 seek。解析/写入失败后游标进入失败状态，后续操作报 `invalid_state`；显式 close 或最后一个引用销毁释放内部缓冲和流引用。底层流若被调用方关闭，操作报原始 `io_error`，之后游标失败。已返回的元素和已写出的前缀不会回滚；最后一项已返回也不代表文件完整，须继续 next 直到无值。需要原子保存时先写临时文件，再原子替换。直接 write 不负责 flush，调用方使用 `file_stream.flush/sync/close`。

严格语法、重复键最后值、数字类型与深度规则沿用旧接口；增量解析错误仍给出行、Unicode 标量列与 UTF-8 字节偏移。超限使用 `size_limit/depth_limit`，错误选项使用 `invalid_argument`。读取缓冲为固定块，根数组仅保留当前元素；生成缓冲为固定块，对象排序另需与对象字段数成正比的空间。

schema 是明确受限的 JSON Schema 风格契约，不宣称支持完整 JSON Schema 草案：支持 `type`（单个 `null/boolean/integer/number/string/array/object`）、`properties`、`required`、布尔 `additionalProperties`、`items`、`minimum/maximum`、`minLength/maxLength`、`minItems/maxItems`。未声明的约束不限制值；`number` 接受 int 和有限 float，`integer` 只接受 TX int；长度按 Unicode 标量计数。所有未知关键字（包括 `$ref`、`format`）和类型错误均报 `invalid_schema`，不会默默忽略。schema 最大 128 层、10000 个节点；校验值须为无环可序列化 JSON 值，最大 64 MiB，校验最多访问 1000000 个节点。缺失必需字段使用 `missing_field`，类型错误使用 `type_mismatch`，其他约束使用 `schema_mismatch`；消息含实例路径，不输出字段值。所有 schema 分支均先检查有效性，即使实例没有对应字段。

迁移时，小数据继续调用 `parse/stringify`；受限完整值使用 `read/write`；大数组使用 `reader/next` 和 `writer/write_value/finish`。需要 schema 时，对完整根值或逐项读取的元素显式调用 `validate`，校验通过后再使用或写出。

## 实现与验证

2026-09-26：接口与实现已接入；`pwsh -NoProfile -File scripts/build.ps1` 构建通过并清理临时 `build/`。本轮只运行了两个定向场景：

- [文件配置示例](../examples/json.tx)编译运行成功，从 `examples/json_config.json` 读取含嵌套对象与对象数组的 `dict`，直接修改字段后写入 `tx_build/json_config_updated.json`；核对输入文件未变，输出包含 `retries=3`、`enabled=true`、`server.port=8081` 和首个任务 `enabled=false`。
- [边界用例](../tests/json/behavior.tx)编译运行并输出 `JSON_OK`，覆盖合法 `null`、Unicode 转义、整数边界、浮点数、重复字段、数组根值、紧凑与缩进序列化、带位置的格式错误、缺失字段、循环引用、非字符串键和非法缩进；还验证了 `any` 到 `dict` / `array` 的声明、赋值、返回、共享修改及类型不符错误。

未运行全量回归或性能测试。

## 8.1 实施记录

2026-09-27：公开 `.txh`、不透明类型、标准库实现、直接 ABI/LLVM 调用、语言文档和 [增量示例](../examples/json_stream.tx) 已交付。解析器共用有界预读和位置追踪，生成器支持有界输出块；游标不保存 TX 回调或对象，关闭和最后引用释放原生资源，已返回的值沿用原有 GC 规则。未新增第三方依赖。

`pwsh -NoProfile -File scripts/build.ps1` 完整构建 Windows x64 `tx/` 工具链和标准库成功，兼容指纹已更新，临时 `build/` 已清理。`python scripts/check_data_formats.py json` 通过：

- 既有 `tests/json/behavior.tx` 的严格语法、重复键、数字、中文位置、稳定输出和循环拒绝回归。
- `tests/json/stream.tx` 与示例的二进制/UTF-16 文本流、逐项 null/EOF、共享别名、any 恢复、deep_copy 拒绝、重复 finish/close、schema 与失败状态。
- `tests/json/stream_native.cpp` 的 1/2/7/4096 字节分块、跨块 UTF-8/代理项、20000 元素持续生成（单值限额 16 字节）、大小/深度/尾部语法、全局行列/偏移、I/O 失败注入、半写入和关闭释放引用。
- 错误实参在 TX 源码位置报类型诊断；生成文件由 Python `json` 独立读取并核对内容。

8.1 的接口及上述定向验收已完成。schema 是本页列明的受限校验契约，未知关键字会拒绝，并非完整 JSON Schema 草案实现。未运行全库测试、跨平台验收、解析器模糊测试或性能基准；8.3～8.6 的其他格式、结构映射与终态交叉验收仍独立推进。
