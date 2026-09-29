#include "stdlib/serde_direct.hpp"

#include <algorithm>
#include <optional>
#include <unordered_set>

namespace tx_generated
{
namespace
{

const serde_field* find_field(const serde_schema& schema, const std::any& key, bool json)
{
    const auto* order = json ? schema.json_order : schema.cbor_order;
    const auto found = std::lower_bound(order, order + schema.field_size, key,
        [&](std::uint64_t index, const std::any& sought)
        {
            const auto& field = schema.field_data[index];
            return json ? std::string_view(field.name) < std::any_cast<const std::string&>(sought)
                : field.number < std::any_cast<std::int64_t>(sought);
        });
    if (found == order + schema.field_size)
    {
        return nullptr;
    }
    const auto& field = schema.field_data[*found];
    return (json ? field.name == std::any_cast<const std::string&>(key)
        : field.number == std::any_cast<std::int64_t>(key)) ? &field : nullptr;
}

void finish_fields(struct_fields& fields, const serde_schema& schema, serde_depth depth)
{
    for (const auto& field : schema.fields())
    {
        auto& slot = fields[field.index];
        if (slot.name)
        {
            continue;
        }
        if (field.default_value.kind >= 0)
        {
            slot = {field.name, serde_default_value(field.default_value)};
        }
        else if (field.type.kind == serde_kind::option)
        {
            // 缺省 option 无 wire 值，保留原来的逻辑深度检查与 none 表示。
            depth.child().check(false);
            struct_fields option(2);
            option[0] = {"present", false};
            option[1] = {"value", std::any{}};
            slot = {field.name, dynamic_struct(dynamic_struct_data{
                field.type.name, "option", std::move(option)})};
        }
        else
        {
            serde_decode_error("missing_field", "serde 缺少必需字段：" + std::string(field.name));
        }
    }
}

void read_unknown(serde_reader& reader, const serde_schema& schema, const std::any& key,
    std::optional<tx_dict>& unknown, std::unordered_set<std::string>& seen, std::size_t depth)
{
    const bool json = reader.format == serde_format::json;
    if (json && !seen.insert(std::any_cast<const std::string&>(key)).second)
    {
        serde_decode_error("duplicate_key", "serde JSON 对象包含重复字段名");
    }
    if (schema.unknown == serde_unknown::reject)
    {
        serde_decode_error("unknown_field", "serde 不允许未知字段：" + (json
            ? std::any_cast<const std::string&>(key) : std::to_string(std::any_cast<std::int64_t>(key))));
    }
    if (schema.unknown == serde_unknown::ignore)
    {
        reader.skip(depth);
        return;
    }
    if (!unknown)
    {
        unknown.emplace();
    }
    (void)unknown->emplace_back(key, reader.scalar(depth));
}

} // namespace

std::any serde_read_struct(serde_reader& reader, const serde_schema& schema, serde_depth depth)
{
    auto sequence = reader.begin(true, depth);
    struct_fields fields(schema.field_count);
    std::optional<tx_dict> unknown;
    std::unordered_set<std::string> unknown_seen;
    bool version_seen = false;
    const bool json = reader.format == serde_format::json;
    while (reader.next(sequence))
    {
        auto key = reader.key(sequence, depth.wire + 1);
        const bool version = json ? std::any_cast<const std::string&>(key) == "$schema"
            : std::any_cast<std::int64_t>(key) == 0;
        if (version)
        {
            if (version_seen)
            {
                serde_decode_error("duplicate_key", "serde 包含重复版本字段");
            }
            auto input = reader.scalar(depth.wire + 1);
            const auto* number = std::any_cast<std::int64_t>(&input);
            if (!number || *number != schema.version)
            {
                serde_decode_error("schema_version", "serde schema 版本不符或类型错误");
            }
            version_seen = true;
        }
        else if (const auto* field = find_field(schema, key, json))
        {
            auto& slot = fields[field->index];
            if (slot.name)
            {
                serde_decode_error("duplicate_key", "serde JSON 对象包含重复字段名");
            }
            slot = {field->name, field->type.codec->decode(reader, field->type, depth.child())};
        }
        else
        {
            read_unknown(reader, schema, key, unknown, unknown_seen, depth.wire + 1);
        }
    }
    if (!version_seen)
    {
        serde_decode_error("schema_version", "serde schema 版本缺失");
    }
    finish_fields(fields, schema, depth);
    if (schema.unknown_index >= 0)
    {
        fields[schema.unknown_index] = {schema.unknown_name, unknown ? std::move(*unknown) : tx_dict{}};
    }
    // 仅完整成功的槽位移入最终对象；此前所有资源均由局部 RAII 对象持有。
    return dynamic_struct(dynamic_struct_data{schema.type_name, schema.display_name, std::move(fields)});
}

} // namespace tx_generated
