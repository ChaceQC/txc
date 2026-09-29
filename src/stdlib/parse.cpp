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

scalar_parse_result<std::int64_t> parse_int_scalar(std::string_view text,
                                                  std::int64_t base) noexcept
{
    if (base < 2 || base > 36)
    {
        return {0, parse_error::invalid_base};
    }
    text = trimmed(text);
    if (text.empty())
    {
        return {0, parse_error::empty_int};
    }
    const bool negative = text.front() == '-';
    if (negative || text.front() == '+')
    {
        text.remove_prefix(1);
    }
    if (text.empty())
    {
        return {0, parse_error::int_sign};
    }
    const auto limit = static_cast<std::uint64_t>(
        std::numeric_limits<std::int64_t>::max()) + (negative ? 1U : 0U);
    std::uint64_t magnitude = 0;
    if (base == 10)
    {
        // 标准库的十进制专用扫描不做逐位进制分派；越界也会消费所有数字。
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), magnitude, 10);
        if (error == std::errc::invalid_argument || end != text.data() + text.size())
        {
            return {0, parse_error::int_syntax};
        }
        if (error == std::errc::result_out_of_range || magnitude > limit)
        {
            return {0, parse_error::int_range};
        }
        return {negative ? static_cast<std::int64_t>(-magnitude)
                         : static_cast<std::int64_t>(magnitude), parse_error::none};
    }
    bool overflow = false;
    const auto radix = static_cast<std::uint64_t>(base);
    const auto cutoff = limit / radix;
    const auto cutlim = limit % radix;
    for (const char character : text)
    {
        const int digit = digit_value(character);
        if (digit < 0 || digit >= base)
        {
            return {0, parse_error::int_syntax};
        }
        const auto value = static_cast<std::uint64_t>(digit);
        // 溢出后仍检查余下字符，保留非法字符高于越界的错误优先级。
        overflow |= magnitude > cutoff || (magnitude == cutoff && value > cutlim);
        if (!overflow)
        {
            magnitude = magnitude * radix + value;
        }
    }
    if (overflow)
    {
        return {0, parse_error::int_range};
    }
    // 最小负值的绝对值不能先转换成有符号正数。
    const auto value = negative && magnitude == limit
        ? std::numeric_limits<std::int64_t>::min()
        : negative ? -static_cast<std::int64_t>(magnitude)
                   : static_cast<std::int64_t>(magnitude);
    return {value, parse_error::none};
}

scalar_parse_result<double> parse_float_scalar(std::string_view text) noexcept
{
    text = trimmed(text);
    if (text.empty())
    {
        return {0, parse_error::empty_float};
    }
    if (text.front() == '+')
    {
        text.remove_prefix(1);
        if (text.empty() || text.front() == '-' || text.front() == '+')
        {
            return {0, parse_error::float_sign};
        }
    }
    double value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(),
                                             value, std::chars_format::general);
    if (error == std::errc::invalid_argument || end != text.data() + text.size())
    {
        return {0, parse_error::float_syntax};
    }
    if (error == std::errc::result_out_of_range)
    {
        return {0, parse_error::float_range};
    }
    if (!std::isfinite(value))
    {
        return {0, parse_error::non_finite};
    }
    return {value, parse_error::none};
}

parse_error_text parse_error_text_of(parse_error error) noexcept
{
    const char* code = "";
    const char* message = "";
    switch (error)
    {
    case parse_error::none: return {};
    case parse_error::invalid_base:
        code = "invalid_base"; message = "整数解析进制必须在 2 到 36 之间"; break;
    case parse_error::empty_int:
        code = "empty_input"; message = "整数解析文本不能为空"; break;
    case parse_error::int_sign:
        code = "invalid_syntax"; message = "整数符号后需要数字"; break;
    case parse_error::int_syntax:
        code = "invalid_syntax"; message = "整数文本含有不属于指定进制的字符"; break;
    case parse_error::int_range:
        code = "out_of_range"; message = "整数文本超出 int 范围"; break;
    case parse_error::empty_float:
        code = "empty_input"; message = "浮点解析文本不能为空"; break;
    case parse_error::float_sign:
        code = "invalid_syntax"; message = "浮点文本的符号或数字无效"; break;
    case parse_error::float_syntax:
        code = "invalid_syntax"; message = "字符串不能解析为完整的 float"; break;
    case parse_error::float_range:
        code = "out_of_range"; message = "浮点文本超出 float 可表示范围"; break;
    case parse_error::non_finite:
        code = "non_finite"; message = "浮点解析结果必须为有限值"; break;
    }
    return {error_kind_name(tx::error_kind::parse), code, message};
}

error_info materialize_parse_error(parse_error error)
{
    const auto text = parse_error_text_of(error);
    return {error == parse_error::none ? tx::error_kind::none : tx::error_kind::parse,
        std::string(text.code), std::string(text.message)};
}

operation_result<std::int64_t> try_parse_int(std::string_view text, std::int64_t base)
{
    const auto parsed = parse_int_scalar(text, base);
    return {parsed.error == parse_error::none, parsed.value,
            materialize_parse_error(parsed.error)};
}

operation_result<double> try_parse_float(std::string_view text)
{
    const auto parsed = parse_float_scalar(text);
    return {parsed.error == parse_error::none, parsed.value,
            materialize_parse_error(parsed.error)};
}

} // namespace tx_generated
