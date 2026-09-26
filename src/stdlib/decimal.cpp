#include "stdlib/decimal.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace tx_generated::decimal_math
{
namespace
{

enum class rounding
{
    toward_zero,
    floor,
    ceiling,
    half_even,
    half_away
};

[[noreturn]] void fail(tx::error_kind kind, const char* code,
                       const char* message)
{
    throw runtime_failure({kind, code, message});
}

rounding parse_rounding(std::string_view mode)
{
    if (mode == "toward_zero")
    {
        return rounding::toward_zero;
    }
    if (mode == "floor")
    {
        return rounding::floor;
    }
    if (mode == "ceiling")
    {
        return rounding::ceiling;
    }
    if (mode == "half_even")
    {
        return rounding::half_even;
    }
    if (mode == "half_away")
    {
        return rounding::half_away;
    }
    fail(tx::error_kind::runtime, "invalid_argument", "未知十进制舍入模式");
}

void validate_scale(int scale)
{
    if (scale < 0 || scale > 18)
    {
        fail(tx::error_kind::runtime, "invalid_argument",
             "目标小数位须位于 0～18");
    }
}

void check_precision(const value& number)
{
    if (number.digits.size() > 38)
    {
        fail(tx::error_kind::runtime, "out_of_range",
             "十进制结果超过 38 位有效数字");
    }
}

void normalize_sign(value& number)
{
    if (number.digits.is_zero())
    {
        number.negative = false;
    }
}

natural rounded_quotient(natural quotient, const natural& remainder,
                         const natural& divisor, bool negative,
                         rounding mode)
{
    if (remainder.is_zero())
    {
        return quotient;
    }
    bool increment = false;
    if (mode == rounding::floor)
    {
        increment = negative;
    }
    else if (mode == rounding::ceiling)
    {
        increment = !negative;
    }
    else if (mode == rounding::half_even || mode == rounding::half_away)
    {
        const auto twice = natural::add(remainder, remainder);
        const auto relation = natural::compare(twice, divisor);
        increment = relation > 0 || (relation == 0 &&
            (mode == rounding::half_away || quotient.is_odd()));
    }
    if (increment)
    {
        quotient = natural::add(quotient, natural("1"));
    }
    return quotient;
}

value combine(const value& left, const value& right, bool subtract_right,
              int scale, std::string_view mode)
{
    const int aligned_scale = std::max(left.scale, right.scale);
    auto first = left.digits;
    auto second = right.digits;
    first.multiply_pow10(aligned_scale - left.scale);
    second.multiply_pow10(aligned_scale - right.scale);
    const bool right_negative = right.negative != subtract_right;
    value result;
    result.scale = aligned_scale;
    if (left.negative == right_negative)
    {
        result.negative = left.negative;
        result.digits = natural::add(first, second);
    }
    else if (natural::compare(first, second) >= 0)
    {
        result.negative = left.negative;
        result.digits = natural::subtract(first, second);
    }
    else
    {
        result.negative = right_negative;
        result.digits = natural::subtract(second, first);
    }
    return quantize(std::move(result), scale, mode);
}

} // namespace

value parse(std::string_view text)
{
    std::size_t index = 0;
    bool negative = false;
    if (index < text.size() && (text[index] == '-' || text[index] == '+'))
    {
        negative = text[index++] == '-';
    }
    const auto integer_start = index;
    while (index < text.size() && text[index] >= '0' && text[index] <= '9')
    {
        ++index;
    }
    if (index == integer_start)
    {
        fail(tx::error_kind::parse, "invalid_syntax",
             "十进制文本需要整数数字");
    }
    std::size_t fractional_start = index;
    if (index < text.size() && text[index] == '.')
    {
        fractional_start = ++index;
        while (index < text.size() && text[index] >= '0' && text[index] <= '9')
        {
            ++index;
        }
        if (index == fractional_start)
        {
            fail(tx::error_kind::parse, "invalid_syntax",
                 "小数点后需要数字");
        }
    }
    if (index != text.size())
    {
        fail(tx::error_kind::parse, "invalid_syntax", "十进制文本格式无效");
    }
    const auto scale = index - fractional_start;
    if (scale > 18)
    {
        fail(tx::error_kind::parse, "out_of_range",
             "十进制文本超过 18 位小数");
    }
    std::string digits;
    digits.reserve(text.size());
    for (char character : text)
    {
        if (character >= '0' && character <= '9')
        {
            digits.push_back(character);
        }
    }
    value result{negative, natural(digits), static_cast<int>(scale)};
    if (result.digits.size() > 38)
    {
        fail(tx::error_kind::parse, "out_of_range",
             "十进制文本超过 38 位有效数字");
    }
    normalize_sign(result);
    return result;
}

value from_integer(std::int64_t number)
{
    return parse(std::to_string(number));
}

std::string to_text(const value& number)
{
    auto digits = number.digits.text();
    if (number.scale > 0)
    {
        if (digits.size() <= static_cast<std::size_t>(number.scale))
        {
            digits.insert(0, static_cast<std::size_t>(number.scale) -
                               digits.size() + 1, '0');
        }
        digits.insert(digits.size() - number.scale, 1, '.');
    }
    if (number.negative)
    {
        digits.insert(digits.begin(), '-');
    }
    return digits;
}

value quantize(value number, int scale, std::string_view mode)
{
    validate_scale(scale);
    const auto rounding_mode = parse_rounding(mode);
    if (scale > number.scale)
    {
        number.digits.multiply_pow10(scale - number.scale);
    }
    else if (scale < number.scale)
    {
        natural divisor("1");
        divisor.multiply_pow10(number.scale - scale);
        auto [quotient, remainder] = natural::divide(number.digits, divisor);
        number.digits = rounded_quotient(std::move(quotient), remainder,
                                         divisor, number.negative, rounding_mode);
    }
    number.scale = scale;
    normalize_sign(number);
    check_precision(number);
    return number;
}

value add(const value& left, const value& right,
          int scale, std::string_view mode)
{
    return combine(left, right, false, scale, mode);
}

value subtract(const value& left, const value& right,
               int scale, std::string_view mode)
{
    return combine(left, right, true, scale, mode);
}

value multiply(const value& left, const value& right,
               int scale, std::string_view mode)
{
    value result{left.negative != right.negative,
                 natural::multiply(left.digits, right.digits),
                 left.scale + right.scale};
    return quantize(std::move(result), scale, mode);
}

value divide(const value& left, const value& right,
             int scale, std::string_view mode)
{
    validate_scale(scale);
    const auto rounding_mode = parse_rounding(mode);
    if (right.digits.is_zero())
    {
        fail(tx::error_kind::runtime, "division_by_zero",
             "十进制除数不能为零");
    }
    auto numerator = left.digits;
    auto denominator = right.digits;
    const int shift = right.scale - left.scale + scale;
    if (shift >= 0)
    {
        numerator.multiply_pow10(shift);
    }
    else
    {
        denominator.multiply_pow10(-shift);
    }
    auto [quotient, remainder] = natural::divide(numerator, denominator);
    value result{left.negative != right.negative,
                 rounded_quotient(std::move(quotient), remainder, denominator,
                                  left.negative != right.negative,
                                  rounding_mode), scale};
    normalize_sign(result);
    check_precision(result);
    return result;
}

int compare(const value& left, const value& right)
{
    if (left.negative != right.negative)
    {
        return left.negative ? -1 : 1;
    }
    auto first = left.digits;
    auto second = right.digits;
    first.multiply_pow10(std::max(left.scale, right.scale) - left.scale);
    second.multiply_pow10(std::max(left.scale, right.scale) - right.scale);
    const auto relation = natural::compare(first, second);
    return left.negative ? -relation : relation;
}

} // namespace tx_generated::decimal_math
