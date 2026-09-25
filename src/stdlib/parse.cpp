#include "stdlib/parse.hpp"

#include <charconv>
#include <cmath>
#include <limits>
#include <system_error>

namespace tx_generated
{
namespace
{

bool ascii_space(char value)
{
    return value == ' ' || value == '\t' || value == '\r' ||
           value == '\n' || value == '\v' || value == '\f';
}

std::string_view trimmed(std::string_view text)
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

template<class value_type>
operation_result<value_type> failed(const char* code, const char* message)
{
    return {false, {}, {tx::error_kind::parse, code, message}};
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

} // namespace

operation_result<std::int64_t> try_parse_int(std::string_view text,
                                            std::int64_t base)
{
    if (base < 2 || base > 36)
    {
        return failed<std::int64_t>("invalid_base", "整数解析进制必须在 2 到 36 之间");
    }
    text = trimmed(text);
    if (text.empty())
    {
        return failed<std::int64_t>("empty_input", "整数解析文本不能为空");
    }
    const bool negative = text.front() == '-';
    if (negative || text.front() == '+')
    {
        text.remove_prefix(1);
    }
    if (text.empty())
    {
        return failed<std::int64_t>("invalid_syntax", "整数符号后需要数字");
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
            return failed<std::int64_t>("invalid_syntax", "整数文本含有不属于指定进制的字符");
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
        return failed<std::int64_t>("out_of_range", "整数文本超出 int 范围");
    }
    // 最小负值的绝对值不能先转换成有符号正数。
    const auto value = negative && magnitude == limit
        ? std::numeric_limits<std::int64_t>::min()
        : negative ? -static_cast<std::int64_t>(magnitude)
                   : static_cast<std::int64_t>(magnitude);
    return {true, value, {}};
}

operation_result<double> try_parse_float(std::string_view text)
{
    text = trimmed(text);
    if (text.empty())
    {
        return failed<double>("empty_input", "浮点解析文本不能为空");
    }
    if (text.front() == '+')
    {
        text.remove_prefix(1);
        if (text.empty() || text.front() == '-' || text.front() == '+')
        {
            return failed<double>("invalid_syntax", "浮点文本的符号或数字无效");
        }
    }
    double value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(),
                                             value, std::chars_format::general);
    if (error == std::errc::invalid_argument || end != text.data() + text.size())
    {
        return failed<double>("invalid_syntax", "字符串不能解析为完整的 float");
    }
    if (error == std::errc::result_out_of_range)
    {
        return failed<double>("out_of_range", "浮点文本超出 float 可表示范围");
    }
    if (!std::isfinite(value))
    {
        return failed<double>("non_finite", "浮点解析结果必须为有限值");
    }
    return {true, value, {}};
}

} // namespace tx_generated
