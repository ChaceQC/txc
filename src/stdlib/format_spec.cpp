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
#include <utility>

namespace tx_generated
{
namespace
{

constexpr std::size_t max_format_size = 1000000;

struct parsed_spec
{
    char fill = ' ';
    char align = 0;
    char sign = 0;
    char type = 0;
    bool zero = false;
    std::size_t width = 0;
    std::optional<std::size_t> precision;
};

bool is_digit(char value)
{
    return value >= '0' && value <= '9';
}

bool is_align(char value)
{
    return value == '<' || value == '>' || value == '^';
}

std::size_t parse_size(std::string_view spec, std::size_t& offset)
{
    if (offset == spec.size() || !is_digit(spec[offset]))
    {
        throw std::runtime_error("format 宽度或精度需要十进制数字");
    }
    std::size_t result = 0;
    while (offset < spec.size() && is_digit(spec[offset]))
    {
        const auto digit = static_cast<std::size_t>(spec[offset++] - '0');
        if (result > (max_format_size - digit) / 10)
        {
            throw std::runtime_error("format 宽度或精度过大");
        }
        result = result * 10 + digit;
    }
    return result;
}

parsed_spec parse_spec(std::string_view spec)
{
    parsed_spec result;
    std::size_t offset = 0;
    if (spec.size() >= 2 && is_align(spec[1]))
    {
        const auto fill = static_cast<unsigned char>(spec[0]);
        if (fill < 0x20 || fill > 0x7e)
        {
            throw std::runtime_error("format 填充字符必须是单个 ASCII 字符");
        }
        result.fill = spec[0];
        result.align = spec[1];
        offset = 2;
    }
    else if (!spec.empty() && is_align(spec[0]))
    {
        result.align = spec[0];
        offset = 1;
    }
    if (offset < spec.size() && (spec[offset] == '+' || spec[offset] == ' '))
    {
        result.sign = spec[offset++];
    }
    if (offset < spec.size() && spec[offset] == '0')
    {
        result.zero = true;
        ++offset;
    }
    if (offset < spec.size() && is_digit(spec[offset]))
    {
        result.width = parse_size(spec, offset);
    }
    if (offset < spec.size() && spec[offset] == '.')
    {
        ++offset;
        result.precision = parse_size(spec, offset);
    }
    if (offset < spec.size())
    {
        result.type = spec[offset++];
    }
    if (offset != spec.size() ||
        (result.type != 0 &&
         std::string_view("sdboxXfFeEgG").find(result.type) ==
             std::string_view::npos))
    {
        throw std::runtime_error("format 不支持此格式说明");
    }
    return result;
}

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

std::string apply_width(std::string text, const parsed_spec& spec,
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

} // namespace

std::string format_field_value(const std::any& value, std::string_view raw_spec,
                               char conversion)
{
    const auto spec = parse_spec(raw_spec);
    const bool converted = conversion != 0;
    const bool integer = !converted && value.type() == typeid(tx_int);
    const bool floating = !converted && value.type() == typeid(double);
    const bool numeric = integer || floating;
    if ((spec.sign != 0 || spec.zero) && !numeric)
    {
        throw std::runtime_error("format 符号和零填充只用于数字");
    }
    std::string result;
    if (integer_type(spec.type))
    {
        if (!integer || spec.precision)
        {
            throw std::runtime_error("format 整数格式需要 int，且不支持精度");
        }
        result = integer_text(std::any_cast<tx_int>(value), spec.type);
    }
    else if (floating_type(spec.type))
    {
        if (!floating)
        {
            throw std::runtime_error("format 浮点格式需要 float");
        }
        result = floating_text(std::any_cast<double>(value), spec.type,
                               spec.precision.value_or(6));
    }
    else
    {
        if (spec.type == 's' && !converted &&
            value.type() != typeid(std::string))
        {
            throw std::runtime_error("format s 格式需要 str");
        }
        result = conversion == 'r' ? format_repr_value(value)
                                   : format_print_value(value);
        if (spec.precision)
        {
            if (floating && spec.type == 0)
            {
                result = floating_text(std::any_cast<double>(value), 'g',
                                       *spec.precision);
            }
            else if (converted || value.type() == typeid(std::string))
            {
                const auto length = static_cast<std::size_t>(tx_len(result));
                result = tx_fn_slice(result, 0, static_cast<tx_int>(
                    std::min(length, *spec.precision)));
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

} // namespace tx_generated
