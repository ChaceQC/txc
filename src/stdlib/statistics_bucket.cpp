#include "stdlib/statistics_internal.hpp"
#include "stdlib/typed_map.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>

namespace tx_generated::statistics
{
namespace
{

void* frequencies(const void* source, bool iterator)
{
    auto values = std::make_shared<map_storage<double, std::int64_t>>();
    visit(source, iterator, [&](double value)
    {
        const auto found = values->values.find(value);
        if (found == values->values.end())
        {
            values->values.emplace(value == 0 ? 0.0 : value, 1);
        }
        else
        {
            if (found->second == std::numeric_limits<std::int64_t>::max())
            {
                fail("out_of_range", "频数超出 int 范围");
            }
            ++found->second;
        }
    });
    note_gc_allocation();
    return detail::make_handle<std::any>(container_handle(std::move(values)));
}

void* histogram(const void* source, bool iterator, double lower,
                double upper, std::int64_t bins, const char* type_name)
{
    if (!std::isfinite(lower) || !std::isfinite(upper) || lower >= upper ||
        bins < 1 || bins > 1'000'000)
    {
        fail("invalid_argument", "直方图区间或桶数无效");
    }
    int_vector counts;
    counts.data().values.resize(static_cast<std::size_t>(bins), 0);
    counts.data().refresh();
    std::int64_t underflow = 0;
    std::int64_t overflow = 0;
    const long double width = static_cast<long double>(upper) - lower;
    visit(source, iterator, [&](double value)
    {
        std::int64_t* counter = nullptr;
        if (value < lower)
        {
            counter = &underflow;
        }
        else if (value > upper)
        {
            counter = &overflow;
        }
        else
        {
            const long double ratio =
                (static_cast<long double>(value) - lower) / width;
            const auto index = value == upper ? bins - 1 :
                std::min(static_cast<std::int64_t>(ratio * bins), bins - 1);
            counter = &counts.data().values[static_cast<std::size_t>(index)];
        }
        if (*counter == std::numeric_limits<std::int64_t>::max())
        {
            fail("out_of_range", "直方图计数超出 int 范围");
        }
        ++*counter;
    });
    struct_fields fields(3);
    fields[0] = {"counts", std::move(counts)};
    fields[1] = {"underflow", underflow};
    fields[2] = {"overflow", overflow};
    return detail::make_handle<std::any>(dynamic_struct({
        type_name, "histogram_result", std::move(fields)}));
}

} // namespace

} // namespace tx_generated::statistics

#define TX_STAT_BUCKET(source_name, is_iterator)                             \
extern "C" int txrt_statistics_frequencies_##source_name(                   \
    const void* source, void** result) noexcept                              \
{                                                                            \
    return tx_generated::detail::invoke_checked([&]                          \
    {                                                                        \
        *result = tx_generated::statistics::frequencies(source, is_iterator); \
    });                                                                      \
}                                                                            \
extern "C" int txrt_statistics_histogram_##source_name(                     \
    const void* source, double lower, double upper, std::int64_t bins,        \
    const char* type_name, void** result) noexcept                           \
{                                                                            \
    return tx_generated::detail::invoke_checked([&]                          \
    {                                                                        \
        *result = tx_generated::statistics::histogram(                       \
            source, is_iterator, lower, upper, bins, type_name);             \
    });                                                                      \
}

TX_STAT_BUCKET(vector, false)
TX_STAT_BUCKET(iterator, true)

#undef TX_STAT_BUCKET
