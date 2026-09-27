#include "frontend/sema/sema.hpp"

#include <charconv>
#include <cmath>
#include <set>
#include <string>
#include <string_view>

namespace tx
{

void semantic_analyzer::validate_serde_type(
    const value_type& type, source_pos position) const
{
    if (type == value_type::int_type || type == value_type::float_type ||
        type == value_type::bool_type || type == value_type::str_type ||
        type == value_type::bytes_type)
    {
        return;
    }
    if (type.is_vector() || type.is_option())
    {
        validate_serde_type(type.parameters.front(), position);
        return;
    }
    const auto found = structs_.find(type.name);
    if (found == structs_.end() || !found->second->serde)
    {
        throw compile_error(position,
            "serde 字段类型必须是受支持的值类型或已标记的结构体：" +
            type.name);
    }
}

void semantic_analyzer::validate_serde_definition(
    const struct_decl& definition) const
{
    if (!definition.serde)
    {
        for (const auto& field : definition.fields)
        {
            if (field.serde)
            {
                throw compile_error(field.position,
                    "字段 serde 标记只能用于已声明 serde 版本的结构体");
            }
        }
        return;
    }
    std::set<std::int64_t> numbers;
    for (const auto number : definition.serde->reserved)
    {
        if (!numbers.insert(number).second)
        {
            throw compile_error(definition.position,
                "serde reserved 编号重复：" + std::to_string(number));
        }
    }
    std::size_t unknown_fields = 0;
    for (const auto& field : definition.fields)
    {
        if (!field.serde)
        {
            throw compile_error(field.position,
                "serde 结构体的每个字段都需要 serde 编号或 unknown 标记");
        }
        const auto& metadata = *field.serde;
        if (metadata.unknown_capture)
        {
            if (field.type != value_type::dict_type ||
                definition.serde->unknown != serde_unknown_policy::preserve ||
                ++unknown_fields > 1)
            {
                throw compile_error(field.position,
                    "serde(unknown) 仅可作为 preserve 策略下唯一的 dict 字段");
            }
            continue;
        }
        if (!numbers.insert(metadata.number).second)
        {
            throw compile_error(field.position,
                "serde 字段编号重复或已保留：" +
                std::to_string(metadata.number));
        }
        validate_serde_type(field.type, field.position);
        if (!metadata.default_literal)
        {
            continue;
        }
        const auto& literal = *metadata.default_literal;
        const bool is_string = literal.starts_with('"');
        const bool is_bool = literal == "true" || literal == "false";
        const bool is_float = !is_string && !is_bool &&
            literal.find_first_of(".eE") != std::string::npos;
        if ((is_string && field.type != value_type::str_type) ||
            (is_bool && field.type != value_type::bool_type) ||
            (is_float && field.type != value_type::float_type) ||
            (!is_string && !is_bool && !is_float &&
             field.type != value_type::int_type))
        {
            throw compile_error(field.position,
                "serde default 字面量与字段类型不一致：" + field.name);
        }
        if (field.type == value_type::int_type)
        {
            std::int64_t parsed = 0;
            const auto [end, error] = std::from_chars(literal.data(),
                literal.data() + literal.size(), parsed);
            if (error != std::errc{} ||
                end != literal.data() + literal.size())
            {
                throw compile_error(field.position,
                    "serde default 整数超出 int 范围");
            }
        }
        if (field.type == value_type::float_type)
        {
            double parsed = 0;
            const auto [end, error] = std::from_chars(literal.data(),
                literal.data() + literal.size(), parsed);
            if (error != std::errc{} ||
                end != literal.data() + literal.size() ||
                !std::isfinite(parsed))
            {
                throw compile_error(field.position,
                    "serde default 浮点数超出 float 范围");
            }
        }
    }
    if (definition.serde->unknown == serde_unknown_policy::preserve &&
        unknown_fields != 1)
    {
        throw compile_error(definition.position,
            "serde preserve 策略需要一个 dict 字段标记为 serde(unknown)");
    }
}

value_type semantic_analyzer::check_serde_intrinsic(
    expression& item, call_expression& call, std::string_view name)
{
    if (call.arguments.size() != 1 ||
        call.arguments.front().kind != argument_kind::positional)
    {
        throw compile_error(item.position, "serde 编解码只接受一个位置实参");
    }
    const auto actual = check_expression(*call.arguments.front().value);
    const bool decoding = name == "deserialize_json" ||
        name == "deserialize_cbor";
    value_type target;
    if (decoding)
    {
        if (!call.explicit_type)
        {
            throw compile_error(item.position,
                "serde 反序列化需要显式写出 <结构体类型>");
        }
        target = *call.explicit_type;
        const auto expected = name == "deserialize_json"
            ? value_type::str_type : value_type::bytes_type;
        if (actual != expected)
        {
            throw compile_error(call.arguments.front().position,
                "serde 反序列化输入类型需要 " + expected.name);
        }
    }
    else
    {
        if (call.explicit_type)
        {
            throw compile_error(item.position,
                "serde 序列化从值参数推断结构体类型");
        }
        target = actual;
    }
    const auto found = structs_.find(target.name);
    if (found == structs_.end() || !found->second->serde)
    {
        throw compile_error(item.position,
            "serde 需要已标记 schema 的结构体：" + target.name);
    }
    call.overload_index = 0;
    if (decoding)
    {
        return target;
    }
    return name == "serialize_json"
        ? value_type::str_type : value_type::bytes_type;
}

} // namespace tx
