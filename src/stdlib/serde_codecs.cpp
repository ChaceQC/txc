#include "stdlib/serde_direct.hpp"
#include "stdlib/serde_scalar.hpp"

namespace tx_generated
{
namespace
{

template<class value_type, auto write>
void encode_scalar_slot(serde_writer& writer, const typed_slot& value,
    const serde_type&, serde_depth depth)
{
    depth.check(true);
    if constexpr (std::is_same_v<value_type, std::int64_t>)
    {
        (writer.*write)(value.integer);
    }
    else if constexpr (std::is_same_v<value_type, double>)
    {
        (writer.*write)(value.floating);
    }
    else
    {
        (writer.*write)(value.boolean);
    }
}

template<class value_type, auto write>
void encode_scalar(serde_writer& writer, const std::any& value,
    const serde_type&, serde_depth depth)
{
    depth.check(true);
    const auto* input = std::any_cast<value_type>(&value);
    if (!input)
    {
        serde_encode_error("type_mismatch", "serde 字段实际类型与静态 schema 不一致");
    }
    (writer.*write)(*input);
}

template<class value_type>
std::any decode_scalar(serde_reader& reader, const serde_type&, serde_depth depth)
{
    depth.check(false);
    auto result = reader.scalar(depth.wire);
    if constexpr (std::is_same_v<value_type, byte_value>)
    {
        if (reader.format == serde_format::json)
        {
            if (const auto* text = std::any_cast<std::string>(&result))
            {
                return bytes_from_base64_url(*text);
            }
        }
    }
    if (!std::any_cast<value_type>(&result))
    {
        serde_decode_error("type_mismatch", "serde 字段值与声明类型不匹配");
    }
    return result;
}

void encode_option(serde_writer& writer, const std::any& value,
    const serde_type& type, serde_depth depth)
{
    depth.check(true);
    const auto* input = std::any_cast<dynamic_struct>(&value);
    if (!input || (*input)->type_name != type.name || (*input)->fields.size() != 2)
    {
        serde_encode_error("type_mismatch", "serde option 的实际类型不匹配");
    }
    serde_cycle_guard guard(writer.active, input->identity());
    const auto* present = std::any_cast<bool>(&(*input)->fields[0].value);
    if (!present)
    {
        serde_encode_error("type_mismatch", "serde option 状态无效");
    }
    if (*present)
    {
        type.element->codec->encode(writer, (*input)->fields[1].value,
            *type.element, depth.child(1, 0));
    }
    else
    {
        writer.null();
    }
}

std::any decode_option(serde_reader& reader, const serde_type& type, serde_depth depth)
{
    depth.check(false);
    const bool present = !reader.take_null();
    struct_fields fields(2);
    fields[0] = {"present", present};
    fields[1] = {"value", present ? type.element->codec->decode(reader,
        *type.element, depth.child(1, 0)) : std::any{}};
    return dynamic_struct(dynamic_struct_data{type.name, "option", std::move(fields)});
}

void encode_structure(serde_writer& writer, const std::any& value,
    const serde_type& type, serde_depth depth)
{
    serde_write_struct(writer, value, *type.structure, depth.child(1, 0));
}

std::any decode_structure(serde_reader& reader, const serde_type& type, serde_depth depth)
{
    return serde_read_struct(reader, *type.structure, depth.child(1, 0));
}

} // namespace
} // namespace tx_generated

using namespace tx_generated;

extern "C" const serde_codec tx_serde_integer{
    encode_scalar<std::int64_t, &serde_writer::integer>, decode_scalar<std::int64_t>,
    encode_scalar_slot<std::int64_t, &serde_writer::integer>, serde_decode_scalar_slot<std::int64_t>};
extern "C" const serde_codec tx_serde_floating{
    encode_scalar<double, &serde_writer::floating>, decode_scalar<double>,
    encode_scalar_slot<double, &serde_writer::floating>, serde_decode_scalar_slot<double>};
extern "C" const serde_codec tx_serde_boolean{
    encode_scalar<bool, &serde_writer::boolean>, decode_scalar<bool>,
    encode_scalar_slot<bool, &serde_writer::boolean>, serde_decode_scalar_slot<bool>};
extern "C" const serde_codec tx_serde_text{
    encode_scalar<std::string, &serde_writer::text>, decode_scalar<std::string>, nullptr,
    serde_decode_scalar_slot<std::string>};
extern "C" const serde_codec tx_serde_bytes{
    encode_scalar<byte_value, &serde_writer::bytes>, decode_scalar<byte_value>};
extern "C" const serde_codec tx_serde_option{encode_option, decode_option};
extern "C" const serde_codec tx_serde_structure{encode_structure, decode_structure};
