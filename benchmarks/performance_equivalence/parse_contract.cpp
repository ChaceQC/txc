#include "parse_contract.hpp"

#include <charconv>
#include <cstdint>
#include <limits>

namespace performance_equivalence
{
namespace
{

bool ascii_space(char value)
{
    return value == ' ' || value == '\t' || value == '\r' ||
           value == '\n' || value == '\v' || value == '\f';
}

std::string_view trim(std::string_view text)
{
    while (!text.empty() && ascii_space(text.front()))
    {
        text.remove_prefix(1);
    }
    while (!text.empty() && ascii_space(text.back()))
    {
        text.remove_suffix(1);
    }
    return text;
}

int digit_value(char value)
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }
    if (value >= 'a' && value <= 'z')
    {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'Z')
    {
        return value - 'A' + 10;
    }
    return -1;
}

int_result fail(const char* code, const char* message)
{
    return {false, 0, {"parse", code, message}};
}

}

bool parse_int_core(std::string_view text)
{
    std::int64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(),
                                        value, 10);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

int_result try_parse_int(std::string_view text, std::int64_t base)
{
    if (base < 2 || base > 36)
    {
        return fail("invalid_base", "整数解析进制必须在 2 到 36 之间");
    }
    text = trim(text);
    if (text.empty())
    {
        return fail("empty_input", "整数解析文本不能为空");
    }
    const bool negative = text.front() == '-';
    if (negative || text.front() == '+')
    {
        text.remove_prefix(1);
    }
    if (text.empty())
    {
        return fail("invalid_syntax", "整数符号后需要数字");
    }
    const auto limit = static_cast<std::uint64_t>(
        std::numeric_limits<std::int64_t>::max()) + (negative ? 1U : 0U);
    std::uint64_t magnitude = 0;
    bool overflow = false;
    for (const char character : text)
    {
        const int digit = digit_value(character);
        if (digit < 0 || digit >= base)
        {
            return fail("invalid_syntax", "整数文本含有不属于指定进制的字符");
        }
        const auto value = static_cast<std::uint64_t>(digit);
        const auto radix = static_cast<std::uint64_t>(base);
        overflow |= magnitude > (limit - value) / radix;
        if (!overflow)
        {
            magnitude = magnitude * radix + value;
        }
    }
    if (overflow)
    {
        return fail("out_of_range", "整数文本超出 int 范围");
    }
    const auto value = negative && magnitude == limit
        ? std::numeric_limits<std::int64_t>::min()
        : negative ? -static_cast<std::int64_t>(magnitude)
                   : static_cast<std::int64_t>(magnitude);
    return {true, value, {}};
}

}
