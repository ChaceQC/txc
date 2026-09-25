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

## 实现与验证

2026-09-26：接口与实现已接入；`pwsh -NoProfile -File scripts/build.ps1` 构建通过并清理临时 `build/`。本轮只运行了两个定向场景：

- [文件配置示例](../examples/json.tx)编译运行成功，从 `examples/json_config.json` 读取含嵌套对象与对象数组的 `dict`，直接修改字段后写入 `tx_build/json_config_updated.json`；核对输入文件未变，输出包含 `retries=3`、`enabled=true`、`server.port=8081` 和首个任务 `enabled=false`。
- [边界用例](../tests/json/behavior.tx)编译运行并输出 `JSON_OK`，覆盖合法 `null`、Unicode 转义、整数边界、浮点数、重复字段、数组根值、紧凑与缩进序列化、带位置的格式错误、缺失字段、循环引用、非字符串键和非法缩进；还验证了 `any` 到 `dict` / `array` 的声明、赋值、返回、共享修改及类型不符错误。

未运行全量回归或性能测试。
