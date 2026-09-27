#include "stdlib/statistics_internal.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace tx_generated::statistics
{
namespace
{

enum class aggregate_kind
{
    mean,
    variance_population,
    variance_sample,
    stdev_population,
    stdev_sample
};

double finish(const moments& state, aggregate_kind kind)
{
    if (kind == aggregate_kind::mean)
    {
        if (state.count == 0)
        {
            fail("empty_sample", "空样本没有均值");
        }
        return checked_float(state.mean);
    }
    const bool sample = kind == aggregate_kind::variance_sample ||
        kind == aggregate_kind::stdev_sample;
    const double variance = checked_float(checked_variance(state, sample));
    if (kind == aggregate_kind::stdev_population ||
        kind == aggregate_kind::stdev_sample)
    {
        return std::sqrt(variance);
    }
    return variance;
}

double aggregate(const void* source, bool iterator, aggregate_kind kind)
{
    if (kind == aggregate_kind::mean)
    {
        mean_accumulator state;
        visit(source, iterator, [&](double value)
        {
            state.add(value);
        });
        return state.finish();
    }
    moments state;
    visit(source, iterator, [&](double value) { state.add(value); });
    return finish(state, kind);
}

std::vector<double> sorted_values(const void* source, bool iterator)
{
    std::vector<double> result;
    visit(source, iterator, [&](double value)
    {
        if (iterator && result.size() == 1'000'000)
        {
            fail("size_limit", "排序统计的迭代器最多允许 1000000 项");
        }
        result.push_back(value);
    });
    if (result.empty())
    {
        fail("empty_sample", "空样本没有顺序统计量");
    }
    std::sort(result.begin(), result.end());
    return result;
}

double median(const void* source, bool iterator)
{
    const auto values = sorted_values(source, iterator);
    const auto middle = values.size() / 2;
    if (values.size() % 2 == 1)
    {
        return values[middle];
    }
    return checked_float((static_cast<long double>(values[middle - 1]) +
                          static_cast<long double>(values[middle])) / 2);
}

double quantile(const void* source, bool iterator, double probability)
{
    if (!std::isfinite(probability) || probability < 0 || probability > 1)
    {
        fail("invalid_argument", "分位概率须位于 [0,1] 且有限");
    }
    const auto values = sorted_values(source, iterator);
    const long double index = static_cast<long double>(probability) *
        (values.size() - 1);
    const auto lower = static_cast<std::size_t>(index);
    const auto upper = std::min(lower + 1, values.size() - 1);
    const long double fraction = index - lower;
    const long double result = static_cast<long double>(values[lower]) +
        (static_cast<long double>(values[upper]) - values[lower]) * fraction;
    return checked_float(result);
}

} // namespace

} // namespace tx_generated::statistics

#define TX_STAT_AGGREGATE(name, kind, source_name, is_iterator)              \
extern "C" int txrt_statistics_##name##_##source_name(                     \
    const void* source, double* result) noexcept                             \
{                                                                            \
    return tx_generated::detail::invoke_checked([&]                          \
    {                                                                        \
        *result = tx_generated::statistics::aggregate(source, is_iterator,   \
            tx_generated::statistics::aggregate_kind::kind);                \
    });                                                                      \
}

#define TX_STAT_SOURCE(source_name, is_iterator)                             \
TX_STAT_AGGREGATE(mean, mean, source_name, is_iterator)                       \
TX_STAT_AGGREGATE(variance_population, variance_population, source_name,     \
                  is_iterator)                                               \
TX_STAT_AGGREGATE(variance_sample, variance_sample, source_name, is_iterator) \
TX_STAT_AGGREGATE(stdev_population, stdev_population, source_name,           \
                  is_iterator)                                               \
TX_STAT_AGGREGATE(stdev_sample, stdev_sample, source_name, is_iterator)       \
extern "C" int txrt_statistics_median_##source_name(                        \
    const void* source, double* result) noexcept                             \
{                                                                            \
    return tx_generated::detail::invoke_checked([&]                          \
    {                                                                        \
        *result = tx_generated::statistics::median(source, is_iterator);     \
    });                                                                      \
}                                                                            \
extern "C" int txrt_statistics_quantile_##source_name(                      \
    const void* source, double probability, double* result) noexcept         \
{                                                                            \
    return tx_generated::detail::invoke_checked([&]                          \
    {                                                                        \
        *result = tx_generated::statistics::quantile(source, is_iterator,    \
                                                     probability);           \
    });                                                                      \
}

TX_STAT_SOURCE(vector, false)
TX_STAT_SOURCE(iterator, true)

#undef TX_STAT_SOURCE
#undef TX_STAT_AGGREGATE

extern "C" int txrt_statistics_new_accumulator(
    const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto object = std::make_shared<tx_generated::dynamic_class>();
        object->type_name = type_name;
        object->display_name = "accumulator";
        object->owned_ancestors.push_back(type_name);
        object->ancestors = object->owned_ancestors.data();
        object->ancestor_count = 1;
        object->fields = {std::int64_t{0}, 0.0, 0.0};
        tx_generated::register_class_gc(object);
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::class_handle(std::move(object)));
    });
}

extern "C" int txrt_statistics_add(const void* target, double value) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        using namespace tx_generated::statistics;
        require_finite(value);
        auto next = read_accumulator(target);
        next.add(value);
        const double mean = checked_float(next.mean);
        const double m2 = checked_float(std::max(0.0L, next.m2));
        auto& fields = accumulator_at(target).fields;
        // 所有校验完成后一次提交，失败不污染可复用的累计器。
        fields[0] = next.count;
        fields[1] = mean;
        fields[2] = m2;
    });
}

extern "C" int txrt_statistics_count(const void* target,
                                       std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::statistics::read_accumulator(target).count;
    });
}

#define TX_STAT_OF(name, kind)                                                \
extern "C" int txrt_statistics_##name(const void* target,                   \
                                       double* result) noexcept              \
{                                                                            \
    return tx_generated::detail::invoke_checked([&]                          \
    {                                                                        \
        *result = tx_generated::statistics::finish(                           \
            tx_generated::statistics::read_accumulator(target),              \
            tx_generated::statistics::aggregate_kind::kind);                \
    });                                                                      \
}

TX_STAT_OF(mean_of, mean)
TX_STAT_OF(variance_population_of, variance_population)
TX_STAT_OF(variance_sample_of, variance_sample)
TX_STAT_OF(stdev_population_of, stdev_population)
TX_STAT_OF(stdev_sample_of, stdev_sample)

#undef TX_STAT_OF
