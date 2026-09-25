#include "backend/cpp/algorithm_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"
#include "stdlib/algorithm.hpp"

using namespace tx_generated;
using namespace tx_generated::detail;

extern "C" int txrt_algorithm_sort_i64(const void* values) noexcept
{
    return invoke_checked([&]
    {
        tx_algorithm_sort(vector_value<std::int64_t>(values));
    });
}

extern "C" int txrt_algorithm_sorted_i64(const void* values, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_algorithm_sorted(vector_value<std::int64_t>(values)));
    });
}

extern "C" int txrt_algorithm_find_i64(const void* values, std::int64_t value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_find(vector_value<std::int64_t>(values), value);
    });
}

extern "C" int txrt_algorithm_count_i64(const void* values, std::int64_t value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_count(vector_value<std::int64_t>(values), value);
    });
}

extern "C" int txrt_algorithm_lower_bound_i64(const void* values, std::int64_t value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_lower_bound(vector_value<std::int64_t>(values), value);
    });
}

extern "C" int txrt_algorithm_upper_bound_i64(const void* values, std::int64_t value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_upper_bound(vector_value<std::int64_t>(values), value);
    });
}

extern "C" int txrt_algorithm_reverse_i64(const void* values) noexcept
{
    return invoke_checked([&]
    {
        tx_algorithm_reverse(vector_value<std::int64_t>(values));
    });
}

extern "C" int txrt_algorithm_sum_i64(const void* values, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_sum(vector_value<std::int64_t>(values));
    });
}

extern "C" int txrt_algorithm_min_element_i64(const void* values, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_min_element(vector_value<std::int64_t>(values));
    });
}

extern "C" int txrt_algorithm_max_element_i64(const void* values, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_max_element(vector_value<std::int64_t>(values));
    });
}

extern "C" int txrt_algorithm_sort_f64(const void* values) noexcept
{
    return invoke_checked([&]
    {
        tx_algorithm_sort(vector_value<double>(values));
    });
}

extern "C" int txrt_algorithm_sorted_f64(const void* values, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_algorithm_sorted(vector_value<double>(values)));
    });
}

extern "C" int txrt_algorithm_find_f64(const void* values, double value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_find(vector_value<double>(values), value);
    });
}

extern "C" int txrt_algorithm_count_f64(const void* values, double value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_count(vector_value<double>(values), value);
    });
}

extern "C" int txrt_algorithm_lower_bound_f64(const void* values, double value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_lower_bound(vector_value<double>(values), value);
    });
}

extern "C" int txrt_algorithm_upper_bound_f64(const void* values, double value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_upper_bound(vector_value<double>(values), value);
    });
}

extern "C" int txrt_algorithm_reverse_f64(const void* values) noexcept
{
    return invoke_checked([&]
    {
        tx_algorithm_reverse(vector_value<double>(values));
    });
}

extern "C" int txrt_algorithm_sum_f64(const void* values, double* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_sum(vector_value<double>(values));
    });
}

extern "C" int txrt_algorithm_min_element_f64(const void* values, double* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_min_element(vector_value<double>(values));
    });
}

extern "C" int txrt_algorithm_max_element_f64(const void* values, double* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_max_element(vector_value<double>(values));
    });
}

extern "C" int txrt_algorithm_sort_str(const void* values) noexcept
{
    return invoke_checked([&]
    {
        tx_algorithm_sort(vector_value<text_reference>(values));
    });
}

extern "C" int txrt_algorithm_sorted_str(const void* values, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_algorithm_sorted(vector_value<text_reference>(values)));
    });
}

extern "C" int txrt_algorithm_find_str(const void* values, const void* value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_find(vector_value<text_reference>(values), text_reference(value));
    });
}

extern "C" int txrt_algorithm_count_str(const void* values, const void* value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_count(vector_value<text_reference>(values), text_reference(value));
    });
}

extern "C" int txrt_algorithm_lower_bound_str(const void* values, const void* value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_lower_bound(vector_value<text_reference>(values), text_reference(value));
    });
}

extern "C" int txrt_algorithm_upper_bound_str(const void* values, const void* value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_upper_bound(vector_value<text_reference>(values), text_reference(value));
    });
}

extern "C" int txrt_algorithm_reverse_str(const void* values) noexcept
{
    return invoke_checked([&]
    {
        tx_algorithm_reverse(vector_value<text_reference>(values));
    });
}

extern "C" int txrt_algorithm_find_bool(const void* values, bool value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_find(vector_value<std::uint8_t>(values), static_cast<std::uint8_t>(value));
    });
}

extern "C" int txrt_algorithm_count_bool(const void* values, bool value, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_algorithm_count(vector_value<std::uint8_t>(values), static_cast<std::uint8_t>(value));
    });
}

extern "C" int txrt_algorithm_reverse_bool(const void* values) noexcept
{
    return invoke_checked([&]
    {
        tx_algorithm_reverse(vector_value<std::uint8_t>(values));
    });
}
