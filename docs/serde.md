# serde：有 schema 的结构体编解码

本模块对应[标准库实施顺序 8.5](standard_library_plan.md#8-jsoncsvxmlcbor-与结构化映射)。`serde` 的字段规则在编译期确定。现有 `json`、`cbor` 的动态 `any` 接口仍用于无固定形状的数据。

## 声明与接口

```tx
import "serde.txh" as serde

struct account serde(version=2, unknown="preserve", reserved=[4])
{
    id: int serde(1)
    name: str serde(2)
    nickname: str serde(3, default="")
    extras: dict serde(unknown)
}

auto text = serde.serialize_json(account(7, "林", "", {}))
account restored = serde.deserialize_json<account>(text)
```

`serialize_json<T>(value: T) -> str`、`serialize_cbor<T>(value: T) -> bytes` 从实参推断 `T`；`deserialize_json<T>(text: str) -> T`、`deserialize_cbor<T>(data: bytes) -> T` 必须显式写出 `T`。四个入口只接受声明 `serde(...)` 的结构体。编译器拒绝无法确定静态字段形状的类型，不通过 `any` 在运行时寻找结构体成员。

编译器为每个调用生成只读的具体 schema 描述，固定结构体类型、字段索引、编号、类型和默认值；运行时按索引读取该结构体的字段，仅在解码外部映射时按声明的名称或编号匹配输入。schema 描述没有通用的任意类构造入口。

结构体 `version` 从 1 开始。每个数据字段须有唯一的正整数字段编号；删除字段时把旧编号列在 `reserved` 中，不得复用。`default` 使字段缺失时采用该值；`option<T>` 缺失时为 `none`；其他字段必须出现。字段重命名只改变 JSON 名称，不改变 CBOR 编号；需要兼容旧 JSON 名称时保留旧 schema 定义并显式迁移。类型变更使用新编号或显式迁移。

## 线路格式与未知字段

JSON 根对象包含整数 `$schema`，其余键为声明的字段名。`bytes` 字段使用无填充 URL 安全 Base64 字符串。CBOR 根映射用整数键 `0` 保存 schema 版本、正整数字段编号保存值；编码复用确定性 CBOR 规则，`bytes` 保持二进制类型。嵌套结构体各自携带版本。

`unknown="reject"` 拒绝未声明字段；`unknown="ignore"` 丢弃它们；`unknown="preserve"` 将其存入标记 `serde(unknown)` 的 `dict` 字段，重新编码时写回相同格式的未知键和值。保留容器可能含动态类型；写出前仍须满足 JSON/CBOR 的类型和限额，不能把句柄或秘密对象绕过静态字段检查写出。已知字段、版本键和保留编号不能由未知字段容器覆盖。跨 JSON/CBOR 转码的未知值若目标格式不支持，会明确失败。

解码先执行格式解析和大小、深度限制，再校验版本、字段、类型及缺失项，最后构造完整对象；失败不交付半成品。错误归入 `parse_error`，稳定代码包括 `schema_version`、`missing_field`、`unknown_field`、`type_mismatch`、`duplicate_key`。输入和生成值上限为 16 MiB、向量与映射上限为 100 万项、嵌套深度上限为 128 层。值图有环时编码失败。

## 显式迁移

旧版与新版用不同结构体声明，各自固定版本与原有字段编号。应用程序先 `deserialize_json<account_v1>(old_text)` 或 `deserialize_cbor<account_v1>(old_bytes)`，再调用自己编写的 `upgrade(account_v1) -> account_v2`。新版解码器遇到旧版本会报告 `schema_version`；不会自动调用构造器或默默重映射字段。完整示例见 [serde_schema.tx](../examples/serde_schema.tx)。

## 边界

序列化可先生成完整 `str`/`bytes`，再交给现有流式 JSON/CBOR 或原子文件接口；本节不新增直接流式结构体入口。跨格式大数据、半写入和专项模糊测试属于 8.6 与总验收。
