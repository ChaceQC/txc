#include "stdlib/serde_scalar.hpp"

namespace tx_generated
{
namespace
{

template<class first, class second>
void write_pair(serde_writer& writer, const dynamic_struct& value,
    const serde_schema& schema, serde_depth depth)
{
    writer.begin(true, 3, depth);
    writer.key("$schema", 0);
    depth.child(0, 1).check(true);
    writer.integer(schema.version);
    const auto* order = writer.format == serde_format::json ? schema.json_order : schema.cbor_order;
    for (std::size_t index = 0; index < 2; ++index)
    {
        const auto field_index = order[index];
        const auto& field = schema.field_data[field_index];
        writer.separator(index + 1);
        writer.key(field.name, field.number);
        if (field_index == 0)
        {
            serde_write_scalar_slot<first>(writer, value->view.data[field.index], depth.child());
        }
        else
        {
            serde_write_scalar_slot<second>(writer, value->view.data[field.index], depth.child());
        }
    }
    writer.end(true);
}

template<class first, class second>
std::any read_pair(serde_reader& reader, const serde_schema& schema, serde_depth depth)
{
    auto sequence = reader.begin(true, depth);
    dynamic_struct value{dynamic_struct_data(schema.layout)};
    unsigned seen = 0;
    const bool json = reader.format == serde_format::json;
    const auto& a = schema.field_data[0];
    const auto& b = schema.field_data[1];
    while (reader.next(sequence))
    {
        const auto key = reader.key(sequence, depth.wire + 1);
        const unsigned field = (json ? key.name == "$schema" : key.number == 0) ? 4 :
            (json ? key.name == a.name : key.number == a.number) ? 1 :
            (json ? key.name == b.name : key.number == b.number) ? 2 : 0;
        if (!field)
        {
            serde_decode_error("unknown_field", "serde 不允许未知字段");
        }
        if (seen & field)
        {
            serde_decode_error("duplicate_key", "serde 包含重复字段");
        }
        if (field == 4)
        {
            depth.child(0, 1).check(false);
            if (reader.integer(depth.wire + 1) != schema.version)
            {
                serde_decode_error("schema_version", "serde schema 版本不符或类型错误");
            }
        }
        else if (field == 1)
        {
            serde_decode_scalar_slot<first>(reader, value->view.data[a.index], a.type, depth.child());
        }
        else
        {
            serde_decode_scalar_slot<second>(reader, value->view.data[b.index], b.type, depth.child());
        }
        seen |= field;
    }
    if (seen != 7)
    {
        // 缺省值、缺失版本及错误优先级由现有失败回退路径精确裁定。
        serde_decode_error("missing_field", "serde 缺少必需字段");
    }
    return value;
}

} // namespace
} // namespace tx_generated

#define tx_pair_codec(a, b, first, second) \
    extern "C" const tx_generated::serde_schema_codec tx_serde_pair_##a##_##b \
        {tx_generated::write_pair<first, second>, tx_generated::read_pair<first, second>};
#define tx_pair_row(a, first) \
    tx_pair_codec(a, i64, first, std::int64_t) \
    tx_pair_codec(a, f64, first, double) \
    tx_pair_codec(a, bool, first, bool) \
    tx_pair_codec(a, str, first, std::string)
tx_pair_row(i64, std::int64_t)
tx_pair_row(f64, double)
tx_pair_row(bool, bool)
tx_pair_row(str, std::string)
#undef tx_pair_row
#undef tx_pair_codec
