#include "common/format_spec.hpp"

#include <stdexcept>

namespace tx
{
namespace
{

constexpr std::size_t max_format_size = 1000000;

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

} // namespace

format_spec parse_format_spec(std::string_view spec)
{
    format_spec result;
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

} // namespace tx
