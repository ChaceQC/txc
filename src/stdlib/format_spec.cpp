#include "stdlib/format_internal.hpp"

#include "backend/cpp/value_format.hpp"
#include "stdlib/stdlib.hpp"

#include <algorithm>
#include <charconv>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <type_traits>
#include <utility>

namespace tx_generated
{
namespace
{

using tx::format_spec;

bool integer_type(char type)
{
    return type == 'd' || type == 'b' || type == 'o' ||
           type == 'x' || type == 'X';
}

bool floating_type(char type)
{
    return type == 'f' || type == 'F' || type == 'e' ||
           type == 'E' || type == 'g' || type == 'G';
}

std::string integer_text(tx_int value, char type)
{
    const int base = type == 'b' ? 2 : type == 'o' ? 8
                   : type == 'x' || type == 'X' ? 16 : 10;
    char buffer[70];
    const auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer),
                                           value, base);
    if (error != std::errc{})
    {
        throw std::runtime_error("format 整数格式化失败");
    }
    std::string result(buffer, end);
    if (type == 'X')
    {
        std::transform(result.begin(), result.end(), result.begin(),
                       [](char item) { return item >= 'a' && item <= 'f'
                           ? static_cast<char>(item - 'a' + 'A') : item; });
    }
    return result;
}

std::string floating_text(double value, char type, std::size_t precision)
{
    const auto style = type == 'f' || type == 'F' ? std::chars_format::fixed
        : type == 'e' || type == 'E' ? std::chars_format::scientific
                                    : std::chars_format::general;
    const auto digits = style == std::chars_format::general
        ? std::max<std::size_t>(1, precision) : precision;
    std::string result(digits + 400, '\0');
    const auto [end, error] = std::to_chars(result.data(),
        result.data() + result.size(), value, style,
        static_cast<int>(digits));
    if (error != std::errc{})
    {
        throw std::runtime_error("format 浮点数格式化失败");
    }
    result.resize(static_cast<std::size_t>(end - result.data()));
    if (type == 'F' || type == 'E' || type == 'G')
    {
        std::transform(result.begin(), result.end(), result.begin(),
                       [](char item) { return item >= 'a' && item <= 'z'
                           ? static_cast<char>(item - 'a' + 'A') : item; });
    }
    return result;
}

std::string apply_width(std::string text, const format_spec& spec,
                        bool numeric)
{
    const auto align = spec.align == 0 ? (numeric ? '>' : '<') : spec.align;
    if (spec.zero && align != '>')
    {
        throw std::runtime_error("format 数字零填充只支持右对齐");
    }
    const auto length = static_cast<std::size_t>(tx_len(text));
    if (length >= spec.width)
    {
        return text;
    }
    const auto padding = spec.width - length;
    const auto fill = spec.zero ? '0' : spec.fill;
    if (align == '<')
    {
        text.append(padding, fill);
    }
    else if (align == '^')
    {
        const auto left = padding / 2;
        text.insert(0, left, fill);
        text.append(padding - left, fill);
    }
    else
    {
        const auto after_sign = spec.zero && numeric && !text.empty() &&
            (text[0] == '-' || text[0] == '+' || text[0] == ' ');
        text.insert(after_sign ? 1 : 0, padding, fill);
    }
    return text;
}

template<class expected_type, class actual_type>
bool format_has_type(const actual_type& value)
{
    if constexpr (std::is_same_v<actual_type, std::any>)
    {
        return value.type() == typeid(expected_type);
    }
    return std::is_same_v<expected_type, actual_type>;
}

template<class expected_type, class actual_type>
const expected_type& format_value_as(const actual_type& value)
{
    if constexpr (std::is_same_v<actual_type, std::any>)
    {
        return std::any_cast<const expected_type&>(value);
    }
    else if constexpr (std::is_same_v<expected_type, actual_type>)
    {
        return value;
    }
    else
    {
        throw std::runtime_error("format 参数类型不匹配");
    }
}

template<class value_type>
std::string format_text(const value_type& value, bool repr)
{
    if constexpr (std::is_same_v<value_type, tx_int>)
    {
        return std::to_string(value);
    }
    else if constexpr (std::is_same_v<value_type, double>)
    {
        return tx_float_to_string(value);
    }
    else if constexpr (std::is_same_v<value_type, bool>)
    {
        return value ? "true" : "false";
    }
    else if constexpr (std::is_same_v<value_type, std::string>)
    {
        return repr ? format_repr_text(value) : value;
    }
    else
    {
        return repr ? format_repr_value(value) : format_print_value(value);
    }
}

} // namespace

std::string format_field_value(const std::any& value, std::string_view raw_spec,
                               char conversion)
{
    return format_field_value(value, tx::parse_format_spec(raw_spec), conversion);
}

template<class value_type>
std::string format_typed_field(const value_type& value, const format_spec& spec,
                               char conversion)
{
    const bool converted = conversion != 0;
    const bool integer = !converted && format_has_type<tx_int>(value);
    const bool floating = !converted && format_has_type<double>(value);
    const bool numeric = integer || floating;
    if ((spec.sign != 0 || spec.zero) && !numeric)
    {
        throw std::runtime_error("format 符号和零填充只用于数字");
    }
    std::string result;
    if (integer_type(spec.type))
    {
        if (!integer || spec.precision >= 0)
        {
            throw std::runtime_error("format 整数格式需要 int，且不支持精度");
        }
        result = integer_text(format_value_as<tx_int>(value), spec.type);
    }
    else if (floating_type(spec.type))
    {
        if (!floating)
        {
            throw std::runtime_error("format 浮点格式需要 float");
        }
        result = floating_text(format_value_as<double>(value), spec.type,
                               (spec.precision < 0 ? 6 : spec.precision));
    }
    else
    {
        if (spec.type == 's' && !converted &&
            !format_has_type<std::string>(value))
        {
            throw std::runtime_error("format s 格式需要 str");
        }
        result = format_text(value, conversion == 'r');
        if (spec.precision >= 0)
        {
            if (floating && spec.type == 0)
            {
                result = floating_text(format_value_as<double>(value), 'g',
                                       static_cast<std::size_t>(spec.precision));
            }
            else if (converted || format_has_type<std::string>(value))
            {
                const auto length = static_cast<std::size_t>(tx_len(result));
                result = tx_fn_slice(result, 0, static_cast<tx_int>(
                    std::min(length, static_cast<std::size_t>(spec.precision))));
            }
            else
            {
                throw std::runtime_error("format 此类型不支持精度");
            }
        }
    }
    if (numeric && spec.sign != 0 &&
        (result.empty() || result.front() != '-'))
    {
        result.insert(result.begin(), spec.sign);
    }
    return apply_width(std::move(result), spec, numeric);
}

std::string format_field_value(const std::any& value, const format_spec& spec, char conversion)
{
    return format_typed_field(value, spec, conversion);
}

std::string format_field_value(std::int64_t value, const format_spec& spec, char conversion)
{
    return format_typed_field(value, spec, conversion);
}

std::string format_field_value(double value, const format_spec& spec, char conversion)
{
    return format_typed_field(value, spec, conversion);
}

std::string format_field_value(bool value, const format_spec& spec, char conversion)
{
    return format_typed_field(value, spec, conversion);
}

std::string format_field_value(const std::string& value, const format_spec& spec, char conversion)
{
    return format_typed_field(value, spec, conversion);
}

} // namespace tx_generated
