#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/iterator.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

namespace tx_generated::statistics
{

[[noreturn]] inline void fail(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::runtime, code, message});
}

inline void require_finite(double value)
{
    if (!std::isfinite(value))
    {
        fail("non_finite", "统计样本必须是有限 float");
    }
}

inline double checked_float(long double value)
{
    if (!std::isfinite(value) ||
        std::fabs(value) > std::numeric_limits<double>::max())
    {
        fail("out_of_range", "统计结果超出有限 float 范围");
    }
    return static_cast<double>(value);
}

template<class operation>
void visit(const void* source, bool from_iterator, operation&& apply)
{
    if (!from_iterator)
    {
        const auto& values = std::any_cast<const float_vector&>(
            *static_cast<const std::any*>(source)).data().values;
        for (const auto value : values)
        {
            require_finite(value);
            apply(value);
        }
        return;
    }
    auto& cursor = std::any_cast<const tx_iterator&>(
        *static_cast<const std::any*>(source)).data();
    if (cursor.closed)
    {
        fail("invalid_state", "已关闭的统计迭代器不能继续读取");
    }
    if (cursor.exhausted)
    {
        return;
    }
    const auto& values = std::any_cast<const float_vector&>(
        cursor.values).data().values;
    while (cursor.index < values.size())
    {
        const auto value = values[cursor.index++];
        require_finite(value);
        apply(value);
    }
    cursor.exhausted = true;
}

struct moments
{
    std::int64_t count = 0;
    long double mean = 0;
    long double m2 = 0;

    void add(double value)
    {
        if (count == std::numeric_limits<std::int64_t>::max())
        {
            fail("out_of_range", "统计样本计数超出 int 范围");
        }
        const auto next_count = count + 1;
        const long double delta = static_cast<long double>(value) - mean;
        const long double next_mean = mean + delta / next_count;
        const long double next_m2 = m2 +
            delta * (static_cast<long double>(value) - next_mean);
        if (!std::isfinite(next_mean) || !std::isfinite(next_m2))
        {
            fail("out_of_range", "统计累计中间值超出范围");
        }
        count = next_count;
        mean = next_mean;
        m2 = next_m2;
    }
};

struct mean_accumulator
{
    std::int64_t count = 0;
    long double total = 0;
    long double correction = 0;

    // 最多 int64 项有限 double 的和，在该条件下必定位于 long double 范围内。
    static constexpr bool wide_sum = std::numeric_limits<long double>::max_exponent >
        std::numeric_limits<double>::max_exponent + std::numeric_limits<std::int64_t>::digits + 1;

    void add(double value)
    {
        if (count == std::numeric_limits<std::int64_t>::max())
        {
            fail("out_of_range", "统计样本计数超出 int 范围");
        }
        ++count;
        const auto sample = static_cast<long double>(value);
        if constexpr (wide_sum)
        {
            // Neumaier 补偿保留大数正负抵消时的小项，只在收尾除以样本数。
            const auto next = total + sample;
            correction += std::fabs(total) >= std::fabs(sample)
                ? (total - next) + sample : (sample - next) + total;
            total = next;
        }
        else
        {
            // 扩展精度不足的平台使用加权均值，异号时避免差值溢出。
            total = std::signbit(total) == std::signbit(sample)
                ? total + (sample - total) / count
                : total * (static_cast<long double>(count - 1) / count) + sample / count;
        }
    }

    [[nodiscard]] double finish() const
    {
        if (count == 0)
        {
            fail("empty_sample", "空样本没有均值");
        }
        return checked_float(wide_sum ? (total + correction) / count : total);
    }
};

inline long double checked_variance(const moments& value, bool sample)
{
    if (value.count == 0)
    {
        fail("empty_sample", "空样本没有方差");
    }
    if (sample && value.count == 1)
    {
        fail("insufficient_sample", "样本方差至少需要两项");
    }
    const auto result = value.m2 / (sample ? value.count - 1 : value.count);
    // 浮点累计可能产生极小的负误差，但方差的真实值非负。
    return result < 0 ? 0 : result;
}

inline dynamic_class& accumulator_at(const void* target)
{
    auto& holder = *const_cast<std::any*>(static_cast<const std::any*>(target));
    auto& value = std::any_cast<class_handle&>(holder);
    if (!value || std::string_view(value->display_name) != "accumulator" ||
        value->fields.size() != 3)
    {
        fail("invalid_argument", "统计累计器状态无效");
    }
    return *value;
}

inline moments read_accumulator(const void* target)
{
    const auto& fields = accumulator_at(target).fields;
    const auto count = std::any_cast<std::int64_t>(fields[0]);
    const auto mean = std::any_cast<double>(fields[1]);
    const auto m2 = std::any_cast<double>(fields[2]);
    if (count < 0 || !std::isfinite(mean) || !std::isfinite(m2) || m2 < 0)
    {
        fail("invalid_argument", "统计累计器字段无效");
    }
    return {count, mean, m2};
}

} // namespace tx_generated::statistics
