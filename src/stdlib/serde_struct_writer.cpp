#include "stdlib/serde_direct.hpp"

#include <algorithm>

namespace tx_generated
{
namespace
{

struct output_field
{
    std::string_view name;
    std::int64_t number;
    const std::any* value;
    const serde_type* type;
};

const tx_dict* unknown_fields(const dynamic_struct& value, const serde_schema& schema)
{
    if (schema.unknown_index < 0)
    {
        return nullptr;
    }
    const auto* result = std::any_cast<tx_dict>(&value->reference_field(schema.unknown_index));
    if (!result)
    {
        serde_encode_error("type_mismatch", "serde 未知字段容器必须是 dict");
    }
    return result;
}

void append_unknown_fields(std::vector<output_field>& fields,
    const tx_dict& unknown, bool json)
{
    unknown.for_each([&](const std::any& key, const std::any& value)
    {
        output_field field{{}, 0, &value, nullptr};
        if (json)
        {
            const auto* name = std::any_cast<std::string>(&key);
            if (!name)
            {
                serde_encode_error("type_mismatch", "serde JSON 未知字段键必须是 str");
            }
            field.name = *name;
        }
        else
        {
            const auto* number = std::any_cast<std::int64_t>(&key);
            if (!number || *number <= 0)
            {
                serde_encode_error("type_mismatch", "serde CBOR 未知字段键必须是正整数");
            }
            field.number = *number;
        }
        fields.push_back(field);
    });
}

void write_preserved(serde_writer& writer, const dynamic_struct& input,
    const serde_schema& schema, const tx_dict& unknown, serde_depth depth)
{
    // 只在实际存在未知字段时合并排序，条目借用原值，不构建映射或值副本。
    std::vector<output_field> fields;
    fields.reserve(schema.field_size + unknown.size() + 1);
    fields.push_back({"$schema", 0, nullptr, nullptr});
    std::vector<std::any> known;
    known.reserve(schema.field_size);
    for (const auto& field : schema.fields())
    {
        known.push_back(input->read_field(field.index));
        fields.push_back({field.name, field.number, &known.back(), &field.type});
    }
    const bool json = writer.format == serde_format::json;
    append_unknown_fields(fields, unknown, json);
    std::sort(fields.begin(), fields.end(), [json](const output_field& left, const output_field& right)
    {
        return json ? left.name < right.name : left.number < right.number;
    });
    writer.begin(true, fields.size(), depth);
    for (std::size_t index = 0; index < fields.size(); ++index)
    {
        const auto& field = fields[index];
        if (index && (json ? fields[index - 1].name == field.name
            : fields[index - 1].number == field.number))
        {
            serde_encode_error("unknown_collision", "serde 未知字段与已知字段或版本键冲突");
        }
        writer.separator(index);
        writer.key(field.name, field.number);
        if (!field.value)
        {
            depth.child(0, 1).check(true);
            writer.integer(schema.version);
        }
        else if (field.type)
        {
            field.type->codec->encode(writer, *field.value, *field.type, depth.child());
        }
        else
        {
            writer.dynamic(*field.value, depth.wire + 1);
        }
    }
    writer.end(true);
}

} // namespace

void serde_write_struct(serde_writer& writer, const std::any& value,
    const serde_schema& schema, serde_depth depth)
{
    depth.check(true);
    const auto* input = std::any_cast<dynamic_struct>(&value);
    if (!input || (*input)->type_name != schema.type_name ||
        (*input)->field_count() != schema.field_count)
    {
        serde_encode_error("type_mismatch", "serde 结构体实际类型与 schema 不一致");
    }
    serde_cycle_guard guard(writer.active, input->identity());
    const auto* unknown = unknown_fields(*input, schema);
    if (unknown && unknown->size() != 0)
    {
        write_preserved(writer, *input, schema, *unknown, depth);
        return;
    }
    writer.begin(true, schema.field_size + 1, depth);
    writer.key("$schema", 0);
    depth.child(0, 1).check(true);
    writer.integer(schema.version);
    const auto* order = writer.format == serde_format::json ? schema.json_order : schema.cbor_order;
    for (std::size_t index = 0; index < schema.field_size; ++index)
    {
        const auto& field = schema.field_data[order[index]];
        writer.separator(index + 1);
        writer.key(field.name, field.number);
        field.type.codec->encode(writer, (*input)->read_field(field.index), field.type, depth.child());
    }
    writer.end(true);
}

} // namespace tx_generated
