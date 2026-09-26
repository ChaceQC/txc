#include "stdlib/random_generator.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"

#include <any>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <string>
#include <utility>

namespace tx_generated::random_instance
{
namespace
{

dynamic_struct_data& state_at(void* source)
{
    auto& holder = *static_cast<std::any*>(source);
    auto& value = std::any_cast<dynamic_struct&>(holder);
    if (value->display_name != "generator" || value->fields.size() != 1)
    {
        fail("invalid_argument", "随机生成器状态无效");
    }
    return *value.data;
}

double next_fraction(void* source)
{
    return static_cast<double>(next_word(source) >> 11) *
        (1.0 / 9007199254740992.0);
}

void require_finite(double value, const char* message)
{
    if (!std::isfinite(value))
    {
        fail("invalid_argument", message);
    }
}

} // namespace

[[noreturn]] void fail(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::runtime, code, message});
}

std::uint64_t next_word(void* source)
{
    auto& field = state_at(source).fields[0].value;
    auto state = std::bit_cast<std::uint64_t>(
        std::any_cast<std::int64_t>(field));
    state += 0x9e3779b97f4a7c15ULL;
    field = std::bit_cast<std::int64_t>(state);
    // SplitMix64 v1：所有运算均定义在无符号 64 位环上。
    auto mixed = state;
    mixed = (mixed ^ (mixed >> 30)) * 0xbf58476d1ce4e5b9ULL;
    mixed = (mixed ^ (mixed >> 27)) * 0x94d049bb133111ebULL;
    return mixed ^ (mixed >> 31);
}

std::uint64_t next_index(void* source, std::uint64_t bound)
{
    if (bound == 0)
    {
        return next_word(source);
    }
    // 拒绝模除偏差；bound=0 专指完整 2^64 范围。
    const auto threshold = (std::uint64_t{0} - bound) % bound;
    while (true)
    {
        const auto candidate = next_word(source);
        if (candidate >= threshold)
        {
            return candidate % bound;
        }
    }
}

extern "C" int txrt_random_make_generator(std::int64_t seed,
    const char* type_name, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        struct_fields fields(1);
        fields[0] = {"state", seed};
        *result = detail::make_handle<std::any>(dynamic_struct({
            type_name, "generator", std::move(fields)}));
    });
}

extern "C" int txrt_random_reseed(void* source, std::int64_t seed) noexcept
{
    return detail::invoke_checked([&]
    {
        state_at(source).fields[0].value = seed;
    });
}

extern "C" int txrt_random_algorithm_version(void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = detail::make_handle<std::string>("splitmix64-v1");
    });
}

extern "C" int txrt_random_next_int(void* source, std::int64_t lower,
    std::int64_t upper, std::int64_t* result) noexcept
{
    return detail::invoke_checked([&]
    {
        if (lower > upper)
        {
            fail("invalid_argument", "随机整数下界不能大于上界");
        }
        const auto span = std::bit_cast<std::uint64_t>(upper) -
            std::bit_cast<std::uint64_t>(lower) + 1;
        const auto offset = next_index(source, span);
        *result = std::bit_cast<std::int64_t>(
            std::bit_cast<std::uint64_t>(lower) + offset);
    });
}

extern "C" int txrt_random_next_float(void* source, double* result) noexcept
{
    return detail::invoke_checked([&] { *result = next_fraction(source); });
}

extern "C" int txrt_random_bernoulli(void* source, double probability,
    bool* result) noexcept
{
    return detail::invoke_checked([&]
    {
        require_finite(probability, "伯努利概率必须是有限数");
        if (probability < 0.0 || probability > 1.0)
        {
            fail("invalid_argument", "伯努利概率须位于 [0,1]");
        }
        *result = probability == 1.0 ||
            (probability != 0.0 && next_fraction(source) < probability);
    });
}

extern "C" int txrt_random_normal(void* source, double mean,
    double standard_deviation, double* result) noexcept
{
    return detail::invoke_checked([&]
    {
        require_finite(mean, "正态分布均值必须是有限数");
        require_finite(standard_deviation, "正态分布标准差必须是有限数");
        if (standard_deviation < 0.0)
        {
            fail("invalid_argument", "正态分布标准差不能为负");
        }
        if (standard_deviation == 0.0)
        {
            *result = mean;
            return;
        }
        const auto radius = std::sqrt(-2.0 * std::log(1.0 - next_fraction(source)));
        const auto angle = 2.0 * std::numbers::pi * next_fraction(source);
        *result = mean + standard_deviation * radius * std::cos(angle);
        if (!std::isfinite(*result))
        {
            fail("out_of_range", "正态分布结果超出有限范围");
        }
    });
}

extern "C" int txrt_random_exponential(void* source, double rate,
    double* result) noexcept
{
    return detail::invoke_checked([&]
    {
        require_finite(rate, "指数分布速率必须是有限数");
        if (rate <= 0.0)
        {
            fail("invalid_argument", "指数分布速率必须为正");
        }
        *result = -std::log(1.0 - next_fraction(source)) / rate;
        if (!std::isfinite(*result))
        {
            fail("out_of_range", "指数分布结果超出有限范围");
        }
    });
}

} // namespace tx_generated::random_instance
