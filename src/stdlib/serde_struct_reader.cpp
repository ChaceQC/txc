#include "stdlib/serde_direct.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <unordered_set>

namespace tx_generated
{
namespace
{

const serde_field* find_field(const serde_schema& schema, const serde_key& key, bool json)
{
    const auto* order = json ? schema.json_order : schema.cbor_order;
    const auto found = std::lower_bound(order, order + schema.field_size, key,
        [&](std::uint64_t index, const serde_key& sought)
        {
            const auto& field = schema.field_data[index];
            return json ? std::string_view(field.name) < sought.name : field.number < sought.number;
        });
    if (found == order + schema.field_size)
    {
        return nullptr;
    }
    const auto& field = schema.field_data[*found];
    return (json ? field.name == key.name : field.number == key.number) ? &field : nullptr;
}

void store_field(struct_fields& fields, const std::optional<dynamic_struct>& fixed,
    std::size_t index, const char* name, std::any value)
{
    if (fixed)
    {
        (*fixed)->write_field(index, std::move(value));
    }
    else
    {
        fields[index] = {name, std::move(value)};
    }
}

void finish_fields(struct_fields& fields, const std::optional<dynamic_struct>& fixed,
    std::span<const std::uint8_t> seen, const serde_schema& schema, serde_depth depth)
{
    for (const auto& field : schema.fields())
    {
        if (seen[field.index])
        {
            continue;
        }
        if (field.default_value.kind >= 0)
        {
            store_field(fields, fixed, field.index, field.name, serde_default_value(field.default_value));
        }
        else if (field.type.kind == serde_kind::option)
        {
            // 缺省 option 无 wire 值，保留原来的逻辑深度检查与 none 表示。
            depth.child().check(false);
            struct_fields option(2);
            option[0] = {"present", false};
            option[1] = {"value", std::any{}};
            store_field(fields, fixed, field.index, field.name, dynamic_struct(dynamic_struct_data{
                field.type.name, "option", std::move(option)}));
        }
        else
        {
            serde_decode_error("missing_field", "serde 缺少必需字段：" + std::string(field.name));
        }
    }
}

void read_unknown(serde_reader& reader, const serde_schema& schema, const serde_key& key,
    std::optional<tx_dict>& unknown, std::unordered_set<std::string>& seen, std::size_t depth)
{
    const bool json = reader.format == serde_format::json;
    if (json && !seen.insert(key.name).second)
    {
        serde_decode_error("duplicate_key", "serde JSON 对象包含重复字段名");
    }
    if (schema.unknown == serde_unknown::reject)
    {
        serde_decode_error("unknown_field", "serde 不允许未知字段：" + (json
            ? key.name : std::to_string(key.number)));
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
    (void)unknown->emplace_back(json ? std::any(key.name) : std::any(key.number), reader.scalar(depth));
}

} // namespace

std::any serde_read_struct(serde_reader& reader, const serde_schema& schema, serde_depth depth)
{
    auto sequence = reader.begin(true, depth);
    struct_fields fields(schema.layout ? 0 : schema.field_count);
    std::optional<dynamic_struct> fixed;
    if (schema.layout)
    {
        fixed.emplace(dynamic_struct_data(schema.layout));
    }
    std::array<std::uint8_t, 64> local_seen{};
    std::vector<std::uint8_t> large_seen(schema.field_count > local_seen.size() ? schema.field_count : 0);
    const std::span<std::uint8_t> seen = large_seen.empty()
        ? std::span<std::uint8_t>(local_seen).first(schema.field_count) : std::span<std::uint8_t>(large_seen);
    std::optional<tx_dict> unknown;
    std::unordered_set<std::string> unknown_seen;
    bool version_seen = false;
    const bool json = reader.format == serde_format::json;
    while (reader.next(sequence))
    {
        auto key = reader.key(sequence, depth.wire + 1);
        const bool version = json ? key.name == "$schema" : key.number == 0;
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
            if (seen[field->index])
            {
                serde_decode_error("duplicate_key", "serde JSON 对象包含重复字段名");
            }
            store_field(fields, fixed, field->index, field->name,
                field->type.codec->decode(reader, field->type, depth.child()));
            seen[field->index] = 1;
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
    finish_fields(fields, fixed, seen, schema, depth);
    if (schema.unknown_index >= 0)
    {
        store_field(fields, fixed, schema.unknown_index, schema.unknown_name,
            unknown ? std::move(*unknown) : tx_dict{});
    }
    // 已知布局直接填入最终对象；失败时尚未发布的对象和已写字段由 RAII 一起清理。
    if (fixed)
    {
        return std::move(*fixed);
    }
    return dynamic_struct(dynamic_struct_data{schema.type_name, schema.display_name, std::move(fields)});
}

} // namespace tx_generated
