#include "stdlib/serde.hpp"

#include "stdlib/cbor.hpp"
#include "stdlib/dictionary.hpp"
#include "stdlib/error.hpp"
#include "stdlib/json.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <string_view>

namespace tx_generated
{
namespace
{

constexpr std::size_t serde_max_bytes = 16 * 1024 * 1024;

std::any wire_key(const serde_field& field, serde_format format)
{
    return format == serde_format::json
        ? std::any(std::string(field.name)) : std::any(field.number);
}

std::any version_key(serde_format format)
{
    return format == serde_format::json
        ? std::any(std::string("$schema")) : std::any(std::int64_t{0});
}

bool known_key(const std::any& key, const serde_schema& schema,
               serde_format format)
{
    for (const auto& field : schema.fields())
    {
        if (format == serde_format::json)
        {
            const auto* name = std::any_cast<std::string>(&key);
            if (name && *name == field.name)
            {
                return true;
            }
        }
        else
        {
            const auto* number = std::any_cast<std::int64_t>(&key);
            if (number && *number == field.number)
            {
                return true;
            }
        }
    }
    return false;
}

void check_wire_key(const std::any& key, serde_format format)
{
    if (format == serde_format::json)
    {
        if (!std::any_cast<std::string>(&key))
        {
            serde_decode_error("type_mismatch", "serde JSON 字段名必须是字符串");
        }
    }
    else
    {
        const auto* number = std::any_cast<std::int64_t>(&key);
        if (!number || *number <= 0)
        {
            serde_decode_error("type_mismatch",
                "serde CBOR 字段编号必须是正整数");
        }
    }
}

std::string key_label(const std::any& key, serde_format format)
{
    if (format == serde_format::json)
    {
        return std::any_cast<const std::string&>(key);
    }
    return std::to_string(std::any_cast<std::int64_t>(key));
}

void merge_unknown(tx_dict& output, const dynamic_struct& input,
                   const serde_schema& schema, serde_format format)
{
    if (schema.unknown_index < 0)
    {
        return;
    }
    const auto* bag = std::any_cast<tx_dict>(
        &input->fields[schema.unknown_index].value);
    if (!bag)
    {
        serde_encode_error("type_mismatch", "serde 未知字段容器必须是 dict");
    }
    bag->for_each([&](const std::any& key, const std::any& value)
    {
        if (format == serde_format::json && !std::any_cast<std::string>(&key))
        {
            serde_encode_error("type_mismatch", "serde JSON 未知字段键必须是 str");
        }
        if (format == serde_format::cbor)
        {
            const auto* number = std::any_cast<std::int64_t>(&key);
            if (!number || *number <= 0)
            {
                serde_encode_error("type_mismatch",
                    "serde CBOR 未知字段键必须是正整数");
            }
        }
        if (output.find_value(key) != nullptr)
        {
            serde_encode_error("unknown_collision",
                "serde 未知字段与已知字段或版本键冲突");
        }
        (void)output.emplace_back(key, value);
    });
}

} // namespace

[[noreturn]] void serde_decode_error(const char* code, std::string message)
{
    throw runtime_failure({tx::error_kind::parse, code, std::move(message)});
}

[[noreturn]] void serde_encode_error(const char* code, std::string message)
{
    throw runtime_failure({tx::error_kind::runtime, code, std::move(message)});
}

std::any serde_encode_struct(const std::any& value,
    const serde_schema* schema, serde_format format,
    serde_active& active, std::size_t depth)
{
    if (depth > 128)
    {
        serde_encode_error("depth_limit", "serde 结构体嵌套超过 128 层");
    }
    const auto* input = std::any_cast<dynamic_struct>(&value);
    if (!input || (*input)->type_name != schema->type_name ||
        (*input)->fields.size() != schema->field_count)
    {
        serde_encode_error("type_mismatch", "serde 结构体实际类型与 schema 不一致");
    }
    serde_cycle_guard guard(active, input->identity());
    tx_dict result;
    (void)result.emplace_back(version_key(format), schema->version);
    for (const auto& field : schema->fields())
    {
        auto encoded = serde_encode_value((*input)->fields[field.index].value,
            field.type, format, active, depth + 1);
        (void)result.emplace_back(wire_key(field, format), std::move(encoded));
    }
    merge_unknown(result, *input, *schema, format);
    return result;
}

std::any serde_decode_struct(const std::any& value,
    const serde_schema* schema, serde_format format,
    std::size_t depth)
{
    if (depth > 128)
    {
        serde_decode_error("depth_limit", "serde 结构体嵌套超过 128 层");
    }
    const auto* object = std::any_cast<tx_dict>(&value);
    if (!object)
    {
        serde_decode_error("type_mismatch", "serde 根值或嵌套结构体必须是映射");
    }
    if (object->size() > 1000000)
    {
        serde_decode_error("size_limit", "serde 映射超过字段数上限");
    }
    const auto key = version_key(format);
    const auto* version = object->find_value(key);
    if (!version || !std::any_cast<std::int64_t>(version) ||
        std::any_cast<std::int64_t>(*version) != schema->version)
    {
        serde_decode_error("schema_version",
            "serde schema 版本缺失、不符或类型错误：" + std::string(schema->display_name));
    }
    // 所有字段先解码到局部槽，任何失败都不会交付部分结构体。
    struct_fields fields(schema->field_count);
    for (const auto& field : schema->fields())
    {
        const auto field_key = wire_key(field, format);
        const auto* input = object->find_value(field_key);
        std::any decoded;
        if (input)
        {
            decoded = serde_decode_value(*input, field.type, format, depth + 1);
        }
        else if (field.default_value.kind >= 0)
        {
            decoded = serde_default_value(field.default_value);
        }
        else if (field.type.kind == serde_kind::option)
        {
            decoded = serde_decode_value(std::any{}, field.type, format,
                depth + 1);
        }
        else
        {
            serde_decode_error("missing_field", "serde 缺少必需字段：" +
                std::string(field.name));
        }
        fields[field.index] = {field.name, std::move(decoded)};
    }
    tx_dict unknown;
    object->for_each([&](const std::any& item_key, const std::any& item)
    {
        if (item_key.type() == key.type() &&
            key_label(item_key, format) == key_label(key, format))
        {
            return;
        }
        check_wire_key(item_key, format);
        if (known_key(item_key, *schema, format))
        {
            return;
        }
        if (schema->unknown == serde_unknown::reject)
        {
            serde_decode_error("unknown_field", "serde 不允许未知字段：" +
                key_label(item_key, format));
        }
        if (schema->unknown == serde_unknown::preserve)
        {
            (void)unknown.emplace_back(item_key, item);
        }
    });
    if (schema->unknown_index >= 0)
    {
        fields[schema->unknown_index] = {schema->unknown_name,
            std::move(unknown)};
    }
    return dynamic_struct(dynamic_struct_data{schema->type_name,
        schema->display_name, std::move(fields)});
}

std::string serde_serialize_json(const serde_schema* schema,
                                 const std::any& value)
{
    serde_active active;
    auto output = json_stringify(serde_encode_struct(value,
        schema, serde_format::json, active, 0));
    if (output.size() > serde_max_bytes)
    {
        serde_encode_error("size_limit", "serde JSON 输出超过 16 MiB");
    }
    return output;
}

std::any serde_deserialize_json(const serde_schema* schema, std::string_view text)
{
    if (text.size() > serde_max_bytes)
    {
        serde_decode_error("size_limit", "serde JSON 输入超过 16 MiB");
    }
    return serde_decode_struct(json_parse_unique(text), schema,
        serde_format::json, 0);
}

byte_value serde_serialize_cbor(const serde_schema* schema,
                                const std::any& value)
{
    serde_active active;
    const cbor_limits limits{serde_max_bytes, serde_max_bytes, 128, 1000000};
    return cbor_encode(serde_encode_struct(value, schema,
        serde_format::cbor, active, 0), limits);
}

std::any serde_deserialize_cbor(const serde_schema* schema,
                                const byte_value& data)
{
    const cbor_limits limits{serde_max_bytes, serde_max_bytes, 128, 1000000};
    return serde_decode_struct(cbor_decode(data, limits),
        schema, serde_format::cbor, 0);
}

} // namespace tx_generated
