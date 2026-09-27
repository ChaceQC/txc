#include "backend/cpp/runtime_abi_internal.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace
{

[[noreturn]] void fail(const char* code, const char* message)
{
    throw tx_generated::runtime_failure({tx::error_kind::runtime, code, message});
}

std::uint64_t magnitude(std::int64_t value)
{
    return value < 0 ? static_cast<std::uint64_t>(-(value + 1)) + 1
                     : static_cast<std::uint64_t>(value);
}

std::int64_t signed_product(std::int64_t left, std::int64_t right)
{
    const bool negative = (left < 0) != (right < 0);
    const auto first = magnitude(left);
    const auto second = magnitude(right);
    const auto limit = static_cast<std::uint64_t>(
        std::numeric_limits<std::int64_t>::max()) + (negative ? 1 : 0);
    if (second != 0 && first > limit / second)
    {
        fail("out_of_range", "整数乘法结果超出 int 范围");
    }
    const auto product = first * second;
    if (negative && product == limit)
    {
        return std::numeric_limits<std::int64_t>::min();
    }
    const auto positive = static_cast<std::int64_t>(product);
    return negative ? -positive : positive;
}

std::int64_t checked_integer(double value)
{
    if (!std::isfinite(value) || value < -0x1p63 || value >= 0x1p63)
    {
        fail("out_of_range", "舍入结果超出 int 范围");
    }
    return static_cast<std::int64_t>(value);
}

double round_half_even(double value)
{
    double integral = 0;
    const double fraction = std::modf(value, &integral);
    if (std::fabs(fraction) < 0.5)
    {
        return integral;
    }
    if (std::fabs(fraction) > 0.5 || std::fmod(std::fabs(integral), 2.0) != 0)
    {
        return integral + std::copysign(1.0, value);
    }
    return integral;
}

std::int64_t round_to_integer(double value, std::string_view mode)
{
    if (mode == "toward_zero")
    {
        return checked_integer(std::trunc(value));
    }
    if (mode == "floor")
    {
        return checked_integer(std::floor(value));
    }
    if (mode == "ceiling")
    {
        return checked_integer(std::ceil(value));
    }
    if (mode == "half_even")
    {
        return checked_integer(round_half_even(value));
    }
    if (mode == "half_away")
    {
        return checked_integer(std::round(value));
    }
    fail("invalid_argument", "未知舍入模式");
}

} // namespace

#define TX_MATH_UNARY(name, operation)                                      \
extern "C" int txrt_math_##name(double value, double* result) noexcept       \
{                                                                          \
    return tx_generated::detail::invoke_leaf([&]                        \
    {                                                                      \
        *result = operation(value);                                        \
    });                                                                    \
}

TX_MATH_UNARY(sin, std::sin)
TX_MATH_UNARY(cos, std::cos)
TX_MATH_UNARY(tan, std::tan)
TX_MATH_UNARY(asin, std::asin)
TX_MATH_UNARY(acos, std::acos)
TX_MATH_UNARY(atan, std::atan)
TX_MATH_UNARY(sinh, std::sinh)
TX_MATH_UNARY(cosh, std::cosh)
TX_MATH_UNARY(tanh, std::tanh)
TX_MATH_UNARY(exp, std::exp)
TX_MATH_UNARY(log, std::log)
TX_MATH_UNARY(log10, std::log10)

#undef TX_MATH_UNARY

extern "C" int txrt_math_atan2(double y, double x, double* result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        *result = std::atan2(y, x);
    });
}

extern "C" int txrt_math_is_finite(double value, bool* result) noexcept
{
    *result = std::isfinite(value);
    return 0;
}

extern "C" int txrt_math_is_nan(double value, bool* result) noexcept
{
    *result = std::isnan(value);
    return 0;
}

extern "C" int txrt_math_is_infinite(double value, bool* result) noexcept
{
    *result = std::isinf(value);
    return 0;
}

extern "C" int txrt_math_gcd(std::int64_t left, std::int64_t right,
                              std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        auto first = magnitude(left);
        auto second = magnitude(right);
        while (second != 0)
        {
            const auto remainder = first % second;
            first = second;
            second = remainder;
        }
        if (first > static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max()))
        {
            fail("out_of_range", "gcd 结果超出 int 范围");
        }
        *result = static_cast<std::int64_t>(first);
    });
}

extern "C" int txrt_math_lcm(std::int64_t left, std::int64_t right,
                              std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        if (left == 0 || right == 0)
        {
            *result = 0;
            return;
        }
        auto first = magnitude(left);
        auto second = magnitude(right);
        auto divisor_left = first;
        auto divisor_right = second;
        while (divisor_right != 0)
        {
            const auto remainder = divisor_left % divisor_right;
            divisor_left = divisor_right;
            divisor_right = remainder;
        }
        first /= divisor_left;
        const auto limit = static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max());
        if (first > limit / second)
        {
            fail("out_of_range", "lcm 结果超出 int 范围");
        }
        *result = static_cast<std::int64_t>(first * second);
    });
}

extern "C" int txrt_math_pow_int(std::int64_t base, std::int64_t exponent,
                                  std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        if (exponent < 0)
        {
            fail("invalid_argument", "整数幂的指数不能为负");
        }
        std::int64_t product = 1;
        auto power = base;
        auto remaining = static_cast<std::uint64_t>(exponent);
        while (remaining != 0)
        {
            if ((remaining & 1) != 0)
            {
                product = signed_product(product, power);
            }
            remaining >>= 1;
            if (remaining != 0)
            {
                power = signed_product(power, power);
            }
        }
        *result = product;
    });
}

extern "C" int txrt_math_round_to_int(double value, const void* mode,
                                        std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        *result = round_to_integer(value, *static_cast<const std::string*>(mode));
    });
}
