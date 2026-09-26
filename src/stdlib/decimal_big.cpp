#include "stdlib/decimal_big.hpp"

#include <algorithm>
#include <stdexcept>

namespace tx_generated::decimal_math
{

natural::natural() : digits_{0}
{
}

natural::natural(std::string_view text)
{
    digits_.reserve(text.size());
    for (auto index = text.size(); index != 0; --index)
    {
        digits_.push_back(static_cast<std::uint8_t>(text[index - 1] - '0'));
    }
    if (digits_.empty())
    {
        digits_.push_back(0);
    }
    normalize();
}

void natural::normalize() noexcept
{
    while (digits_.size() > 1 && digits_.back() == 0)
    {
        digits_.pop_back();
    }
}

bool natural::is_zero() const noexcept
{
    return digits_.size() == 1 && digits_[0] == 0;
}

bool natural::is_odd() const noexcept
{
    return (digits_[0] & 1) != 0;
}

std::size_t natural::size() const noexcept
{
    return digits_.size();
}

std::string natural::text() const
{
    std::string result;
    result.reserve(digits_.size());
    for (auto index = digits_.size(); index != 0; --index)
    {
        result.push_back(static_cast<char>('0' + digits_[index - 1]));
    }
    return result;
}

void natural::multiply_pow10(std::size_t places)
{
    if (!is_zero())
    {
        digits_.insert(digits_.begin(), places, 0);
    }
}

void natural::shift_digit(std::uint8_t digit)
{
    multiply_pow10(1);
    digits_[0] = digit;
    normalize();
}

int natural::compare(const natural& left, const natural& right) noexcept
{
    if (left.digits_.size() != right.digits_.size())
    {
        return left.digits_.size() < right.digits_.size() ? -1 : 1;
    }
    for (auto index = left.digits_.size(); index != 0; --index)
    {
        if (left.digits_[index - 1] != right.digits_[index - 1])
        {
            return left.digits_[index - 1] < right.digits_[index - 1]
                ? -1 : 1;
        }
    }
    return 0;
}

natural natural::add(const natural& left, const natural& right)
{
    natural result;
    result.digits_.assign(std::max(left.size(), right.size()) + 1, 0);
    unsigned carry = 0;
    for (std::size_t index = 0; index < result.digits_.size(); ++index)
    {
        unsigned sum = carry;
        if (index < left.size())
        {
            sum += left.digits_[index];
        }
        if (index < right.size())
        {
            sum += right.digits_[index];
        }
        result.digits_[index] = static_cast<std::uint8_t>(sum % 10);
        carry = sum / 10;
    }
    result.normalize();
    return result;
}

natural natural::subtract(const natural& left, const natural& right)
{
    if (compare(left, right) < 0)
    {
        throw std::logic_error("十进制整数减法前置条件不成立");
    }
    natural result;
    result.digits_.assign(left.size(), 0);
    int borrow = 0;
    for (std::size_t index = 0; index < left.size(); ++index)
    {
        int digit = left.digits_[index] - borrow;
        if (index < right.size())
        {
            digit -= right.digits_[index];
        }
        borrow = digit < 0 ? 1 : 0;
        result.digits_[index] = static_cast<std::uint8_t>(digit + borrow * 10);
    }
    result.normalize();
    return result;
}

natural natural::multiply(const natural& left, const natural& right)
{
    natural result;
    result.digits_.assign(left.size() + right.size(), 0);
    for (std::size_t first = 0; first < left.size(); ++first)
    {
        unsigned carry = 0;
        for (std::size_t second = 0; second < right.size(); ++second)
        {
            const auto index = first + second;
            const unsigned product = result.digits_[index] + carry +
                left.digits_[first] * right.digits_[second];
            result.digits_[index] = static_cast<std::uint8_t>(product % 10);
            carry = product / 10;
        }
        result.digits_[first + right.size()] = static_cast<std::uint8_t>(carry);
    }
    result.normalize();
    return result;
}

std::pair<natural, natural> natural::divide(
    const natural& numerator, const natural& denominator)
{
    if (denominator.is_zero())
    {
        throw std::logic_error("十进制整数除数不能为零");
    }
    natural quotient;
    quotient.digits_.assign(numerator.size(), 0);
    natural remainder;
    // 每一步的商位在 0～9，最多做九次减法，避免平台大整数依赖。
    for (auto index = numerator.size(); index != 0; --index)
    {
        remainder.shift_digit(numerator.digits_[index - 1]);
        while (compare(remainder, denominator) >= 0)
        {
            remainder = subtract(remainder, denominator);
            ++quotient.digits_[index - 1];
        }
    }
    quotient.normalize();
    return {std::move(quotient), std::move(remainder)};
}

} // namespace tx_generated::decimal_math
