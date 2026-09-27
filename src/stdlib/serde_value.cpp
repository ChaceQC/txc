#include "stdlib/serde.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/array.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <string>

namespace tx_generated
{
namespace
{

template<class vector_type, class value_type>
void append_vector(vector_type& result, value_type value)
{
    result.data().values.push_back(std::move(value));
}

std::any encode_vector(const std::any& value, const serde_type& type,
                       serde_format format, serde_active& active,
                       std::size_t depth)
{
    tx_array result;
    const auto append = [&](const std::any& element)
    {
        if (result.size() >= 1000000)
        {
            serde_encode_error("size_limit", "serde vector 超过元素数上限");
        }
        result.push_back(serde_encode_value(element, *type.element,
            format, active, depth + 1));
    };
    if (const auto* input = std::any_cast<int_vector>(&value))
    {
        serde_cycle_guard guard(active, input->identity());
        for (const auto item : input->data().values) append(item);
    }
    else if (const auto* input = std::any_cast<float_vector>(&value))
    {
        serde_cycle_guard guard(active, input->identity());
        for (const auto item : input->data().values) append(item);
    }
    else if (const auto* input = std::any_cast<bool_vector>(&value))
    {
        serde_cycle_guard guard(active, input->identity());
        for (const auto item : input->data().values) append(static_cast<bool>(item));
    }
    else if (const auto* input = std::any_cast<string_vector>(&value))
    {
        serde_cycle_guard guard(active, input->identity());
        for (const auto& item : input->data().values) append(item.get());
    }
    else if (const auto* input = std::any_cast<bytes_vector>(&value))
    {
        serde_cycle_guard guard(active, input->identity());
        for (const auto& item : input->data().values) append(item);
    }
    else if (const auto* input = std::any_cast<object_vector>(&value))
    {
        serde_cycle_guard guard(active, input->identity());
        for (const auto& item : input->data().values) append(item);
    }
    else
    {
        serde_encode_error("type_mismatch", "serde vector 的实际元素类型不匹配");
    }
    return result;
}

std::any decode_vector(const std::any& value, const serde_type& type,
                       serde_format format, std::size_t depth)
{
    const auto* input = std::any_cast<tx_array>(&value);
    if (input == nullptr)
    {
        serde_decode_error("type_mismatch", "serde 字段需要数组");
    }
    if (input->size() > 1000000)
    {
        serde_decode_error("size_limit", "serde vector 超过元素数上限");
    }
    const auto kind = type.element->kind;
    if (kind == serde_kind::integer)
    {
        int_vector result(type.name);
        for (const auto& item : *input)
        {
            append_vector(result, std::any_cast<std::int64_t>(
                serde_decode_value(item, *type.element, format, depth + 1)));
        }
        result.data().refresh();
        return result;
    }
    if (kind == serde_kind::floating)
    {
        float_vector result(type.name);
        for (const auto& item : *input)
        {
            append_vector(result, std::any_cast<double>(
                serde_decode_value(item, *type.element, format, depth + 1)));
        }
        result.data().refresh();
        return result;
    }
    if (kind == serde_kind::boolean)
    {
        bool_vector result(type.name);
        for (const auto& item : *input)
        {
            append_vector(result, static_cast<std::uint8_t>(std::any_cast<bool>(
                serde_decode_value(item, *type.element, format, depth + 1))));
        }
        result.data().refresh();
        return result;
    }
    if (kind == serde_kind::text)
    {
        string_vector result(type.name);
        for (const auto& item : *input)
        {
            auto* handle = detail::make_handle<std::string>(std::any_cast<std::string>(
                serde_decode_value(item, *type.element, format, depth + 1)));
            text_reference reference(handle);
            detail::destroy_handle(handle);
            append_vector(result, std::move(reference));
        }
        result.data().refresh();
        return result;
    }
    if (kind == serde_kind::bytes)
    {
        bytes_vector result(type.name);
        for (const auto& item : *input)
        {
            append_vector(result, std::any_cast<byte_value>(
                serde_decode_value(item, *type.element, format, depth + 1)));
        }
        result.data().refresh();
        return result;
    }
    object_vector result(type.name);
    for (const auto& item : *input)
    {
        append_vector(result, serde_decode_value(item, *type.element,
            format, depth + 1));
    }
    result.data().refresh();
    return result;
}

std::any option_value(const std::any& value, const serde_type& type,
                      serde_format format, serde_active& active,
                      std::size_t depth)
{
    const auto* input = std::any_cast<dynamic_struct>(&value);
    if (input == nullptr || (*input)->type_name != type.name ||
        (*input)->fields.size() != 2)
    {
        serde_encode_error("type_mismatch", "serde option 的实际类型不匹配");
    }
    serde_cycle_guard guard(active, input->identity());
    const auto* present = std::any_cast<bool>(&(*input)->fields[0].value);
    if (present == nullptr)
    {
        serde_encode_error("type_mismatch", "serde option 状态无效");
    }
    return *present ? serde_encode_value((*input)->fields[1].value,
        *type.element, format, active, depth + 1) : std::any{};
}

std::any decode_option(const std::any& value, const serde_type& type,
                       serde_format format, std::size_t depth)
{
    struct_fields fields(2);
    fields[0] = {"present", value.has_value()};
    fields[1] = {"value", value.has_value()
        ? serde_decode_value(value, *type.element, format, depth + 1)
        : std::any{}};
    return dynamic_struct(dynamic_struct_data{type.name, "option",
        std::move(fields)});
}

} // namespace

std::any serde_encode_value(const std::any& value, const serde_type& type,
    serde_format format, serde_active& active, std::size_t depth)
{
    if (depth > 128)
    {
        serde_encode_error("depth_limit", "serde 值嵌套超过 128 层");
    }
    switch (type.kind)
    {
    case serde_kind::integer:
        if (std::any_cast<std::int64_t>(&value))
        {
            return value;
        }
        break;
    case serde_kind::floating:
        if (std::any_cast<double>(&value))
        {
            return value;
        }
        break;
    case serde_kind::boolean:
        if (std::any_cast<bool>(&value))
        {
            return value;
        }
        break;
    case serde_kind::text:
        if (std::any_cast<std::string>(&value))
        {
            return value;
        }
        break;
    case serde_kind::bytes:
        if (const auto* bytes = std::any_cast<byte_value>(&value))
        {
            return format == serde_format::json
                ? std::any(bytes_to_base64_url(*bytes)) : value;
        }
        break;
    case serde_kind::vector:
        return encode_vector(value, type, format, active, depth);
    case serde_kind::option:
        return option_value(value, type, format, active, depth);
    case serde_kind::structure:
        return serde_encode_struct(value, type.structure, format, active,
            depth + 1);
    }
    serde_encode_error("type_mismatch", "serde 字段实际类型与静态 schema 不一致");
}

std::any serde_decode_value(const std::any& value, const serde_type& type,
    serde_format format, std::size_t depth)
{
    if (depth > 128)
    {
        serde_decode_error("depth_limit", "serde 值嵌套超过 128 层");
    }
    switch (type.kind)
    {
    case serde_kind::integer:
        if (std::any_cast<std::int64_t>(&value))
        {
            return value;
        }
        break;
    case serde_kind::floating:
        if (std::any_cast<double>(&value))
        {
            return value;
        }
        break;
    case serde_kind::boolean:
        if (std::any_cast<bool>(&value))
        {
            return value;
        }
        break;
    case serde_kind::text:
        if (std::any_cast<std::string>(&value))
        {
            return value;
        }
        break;
    case serde_kind::bytes:
        if (format == serde_format::json)
        {
            if (const auto* text = std::any_cast<std::string>(&value))
            {
                return bytes_from_base64_url(*text);
            }
        }
        else if (std::any_cast<byte_value>(&value))
        {
            return value;
        }
        break;
    case serde_kind::vector:
        return decode_vector(value, type, format, depth);
    case serde_kind::option:
        return decode_option(value, type, format, depth);
    case serde_kind::structure:
        return serde_decode_struct(value, type.structure, format, depth + 1);
    }
    serde_decode_error("type_mismatch", "serde 字段值与声明类型不匹配");
}

} // namespace tx_generated
