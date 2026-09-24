#include "backend/cpp/numeric_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace tx_generated
{
namespace
{

void require_finite(double value, const char* operation)
{
    if (!std::isfinite(value))
    {
        throw std::runtime_error(std::string(operation) + " 需要有限 float");
    }
}

double finite_result(double value, const char* operation)
{
    if (!std::isfinite(value))
    {
        throw std::runtime_error(std::string(operation) + " 的结果不是有限 float");
    }
    return value;
}

tx_int checked_integer(double value, const char* operation)
{
    require_finite(value, operation);
    constexpr double upper = 0x1p63;
    constexpr double lower = -0x1p63;
    if (value < lower || value >= upper)
    {
        throw std::runtime_error(std::string(operation) + " 的结果超出 int 范围");
    }
    return static_cast<tx_int>(value);
}

} // namespace

tx_int tx_fn_abs(tx_int value)
{
    if (value == std::numeric_limits<tx_int>::min())
    {
        throw std::runtime_error("abs 的结果超出 int 范围");
    }
    return value < 0 ? -value : value;
}

double tx_fn_abs(double value)
{
    require_finite(value, "abs");
    return std::fabs(value);
}

tx_int tx_fn_min(tx_int left, tx_int right)
{
    return std::min(left, right);
}

double tx_fn_min(double left, double right)
{
    require_finite(left, "min");
    require_finite(right, "min");
    return std::min(left, right);
}

tx_int tx_fn_max(tx_int left, tx_int right)
{
    return std::max(left, right);
}

double tx_fn_max(double left, double right)
{
    require_finite(left, "max");
    require_finite(right, "max");
    return std::max(left, right);
}

tx_int tx_fn_clamp(tx_int value, tx_int lower, tx_int upper)
{
    if (lower > upper)
    {
        throw std::runtime_error("clamp 的下界不能大于上界");
    }
    return std::clamp(value, lower, upper);
}

double tx_fn_clamp(double value, double lower, double upper)
{
    require_finite(value, "clamp");
    require_finite(lower, "clamp");
    require_finite(upper, "clamp");
    if (lower > upper)
    {
        throw std::runtime_error("clamp 的下界不能大于上界");
    }
    return std::clamp(value, lower, upper);
}

tx_int tx_fn_mod(tx_int left, tx_int right)
{
    if (right == 0)
    {
        throw std::runtime_error("mod 的除数不能为零");
    }
    if (left == std::numeric_limits<tx_int>::min() && right == -1)
    {
        throw std::runtime_error("mod 的商超出 int 范围");
    }
    return left % right;
}

double tx_fn_sqrt(double value)
{
    require_finite(value, "sqrt");
    if (value < 0)
    {
        throw std::runtime_error("sqrt 的参数不能为负");
    }
    return finite_result(std::sqrt(value), "sqrt");
}

double tx_fn_pow(double base, double exponent)
{
    require_finite(base, "pow");
    require_finite(exponent, "pow");
    return finite_result(std::pow(base, exponent), "pow");
}

tx_int tx_fn_floor(double value)
{
    require_finite(value, "floor");
    return checked_integer(std::floor(value), "floor");
}

tx_int tx_fn_ceil(double value)
{
    require_finite(value, "ceil");
    return checked_integer(std::ceil(value), "ceil");
}

} // namespace tx_generated

using tx_generated::detail::invoke_checked;

extern "C" int txrt_math_abs_i64(std::int64_t value,
                                  std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_abs(value); });
}

extern "C" int txrt_math_abs_f64(double value, double* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_abs(value); });
}

extern "C" int txrt_math_min_i64(std::int64_t left, std::int64_t right,
                                  std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_min(left, right); });
}

extern "C" int txrt_math_min_f64(double left, double right,
                                  double* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_min(left, right); });
}

extern "C" int txrt_math_max_i64(std::int64_t left, std::int64_t right,
                                  std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_max(left, right); });
}

extern "C" int txrt_math_max_f64(double left, double right,
                                  double* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_max(left, right); });
}

extern "C" int txrt_math_clamp_i64(std::int64_t value, std::int64_t lower,
                                    std::int64_t upper,
                                    std::int64_t* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_clamp(value, lower, upper);
    });
}

extern "C" int txrt_math_clamp_f64(double value, double lower, double upper,
                                    double* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_clamp(value, lower, upper);
    });
}

extern "C" int txrt_math_mod_i64(std::int64_t left, std::int64_t right,
                                  std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_mod(left, right); });
}

extern "C" int txrt_math_sqrt_f64(double value, double* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_sqrt(value); });
}

extern "C" int txrt_math_pow_f64(double base, double exponent,
                                  double* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_pow(base, exponent);
    });
}

extern "C" int txrt_math_floor_f64(double value,
                                    std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_floor(value); });
}

extern "C" int txrt_math_ceil_f64(double value,
                                   std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_fn_ceil(value); });
}
