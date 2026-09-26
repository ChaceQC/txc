#pragma once

#include "stdlib/algorithm.hpp"
#include "stdlib/algorithm_callback.hpp"

#include <algorithm>
#include <any>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace tx_generated
{

template<class element_type>
struct algorithm_order
{
    ordered_compare<element_type> comparison;
    bool custom;

    algorithm_order(const void* less, const void* callback)
        : comparison(less, callback
            ? *static_cast<const std::any*>(callback) : std::any{}),
          custom(callback != nullptr)
    {
    }

    [[nodiscard]] bool operator()(const element_type& left,
                                  const element_type& right) const
    {
        if constexpr (std::is_same_v<element_type, byte_value>)
        {
            if (!custom)
            {
                throw std::runtime_error("bytes 需要显式比较器");
            }
            return invoke_algorithm_callback<std::int64_t>(
                comparison.closure, left, right) < 0;
        }
        else
        {
            if constexpr (std::is_same_v<element_type, double>)
            {
                if (!custom)
                {
                    return detail::algorithm_less<double>{}(left, right);
                }
            }
            return comparison(left, right);
        }
    }

    void validate(const element_type& value) const
    {
        if (custom)
        {
            require_ordered_key(value);
        }
    }
};

template<class element_type>
[[nodiscard]] std::vector<element_type> algorithm_working_copy(
    const tx_vector<element_type>& input)
{
    const auto& values = input.data().values;
    if (values.size() > static_cast<std::size_t>(
            std::numeric_limits<std::int64_t>::max()))
    {
        throw std::overflow_error("algorithm 元素个数超出 int 范围");
    }
    return values;
}

template<class element_type>
void algorithm_replace(const tx_vector<element_type>& input,
                       std::vector<element_type> values)
{
    auto& data = input.data();
    data.values.swap(values);
    data.refresh();
    note_gc_allocation();
}

template<class element_type>
void algorithm_stable_sort(const tx_vector<element_type>& input,
                           const void* callback, const void* less)
{
    auto values = algorithm_working_copy(input);
    algorithm_order<element_type> order(less, callback);
    for (const auto& item : values)
    {
        order.validate(item);
    }
    std::stable_sort(values.begin(), values.end(), order);
    algorithm_replace(input, std::move(values));
}

template<class element_type>
[[nodiscard]] tx_vector<element_type> algorithm_stable_sorted(
    const tx_vector<element_type>& input, const void* callback, const void* less)
{
    auto result = input.copy();
    algorithm_stable_sort(result, callback, less);
    return result;
}

template<class element_type>
[[nodiscard]] std::int64_t algorithm_binary_search(
    const tx_vector<element_type>& input, const element_type& key,
    const void* callback, const void* less)
{
    const auto values = algorithm_working_copy(input);
    algorithm_order<element_type> order(less, callback);
    order.validate(key);
    const auto found = std::lower_bound(values.begin(), values.end(), key, order);
    if (found == values.end() || order(key, *found) || order(*found, key))
    {
        return -1;
    }
    return static_cast<std::int64_t>(found - values.begin());
}

template<class element_type>
[[nodiscard]] int_vector algorithm_equal_range(
    const tx_vector<element_type>& input, const element_type& key,
    const void* callback, const void* less)
{
    const auto values = algorithm_working_copy(input);
    algorithm_order<element_type> order(less, callback);
    order.validate(key);
    const auto [first, last] = std::equal_range(
        values.begin(), values.end(), key, order);
    int_vector result;
    result.data().values = {
        static_cast<std::int64_t>(first - values.begin()),
        static_cast<std::int64_t>(last - values.begin())};
    result.data().refresh();
    return result;
}

template<class element_type>
[[nodiscard]] std::int64_t algorithm_unique(const tx_vector<element_type>& input,
    const void* callback, const void* less)
{
    auto values = algorithm_working_copy(input);
    algorithm_order<element_type> order(less, callback);
    for (const auto& item : values)
    {
        order.validate(item);
    }
    const auto end = std::unique(values.begin(), values.end(),
        [&](const auto& left, const auto& right)
        {
            return !order(left, right) && !order(right, left);
        });
    values.erase(end, values.end());
    const auto size = static_cast<std::int64_t>(values.size());
    algorithm_replace(input, std::move(values));
    return size;
}

template<class element_type>
void algorithm_rotate(const tx_vector<element_type>& input, std::int64_t middle)
{
    auto values = algorithm_working_copy(input);
    if (middle < 0 || static_cast<std::uint64_t>(middle) > values.size())
    {
        throw std::out_of_range("algorithm.rotate 的 middle 超出范围");
    }
    std::rotate(values.begin(), values.begin() + middle, values.end());
    algorithm_replace(input, std::move(values));
}

template<class element_type>
[[nodiscard]] std::int64_t algorithm_partition(
    const tx_vector<element_type>& input, const std::any& predicate)
{
    auto values = algorithm_working_copy(input);
    const auto point = std::partition(values.begin(), values.end(),
        [&](const auto& item)
        {
            return invoke_algorithm_callback<std::uint8_t>(predicate, item) != 0;
        });
    const auto index = static_cast<std::int64_t>(point - values.begin());
    algorithm_replace(input, std::move(values));
    return index;
}

template<class input_type, class output_type>
[[nodiscard]] tx_vector<output_type> algorithm_map(
    const tx_vector<input_type>& input, const std::any& transform,
    const char* output_name)
{
    const auto snapshot = algorithm_working_copy(input);
    tx_vector<output_type> result(output_name ? output_name : "");
    auto& output = result.data();
    output.values.reserve(snapshot.size());
    for (const auto& item : snapshot)
    {
        output.values.push_back(
            invoke_algorithm_callback<output_type>(transform, item));
    }
    output.refresh();
    return result;
}

template<class element_type>
[[nodiscard]] tx_vector<element_type> algorithm_filter(
    const tx_vector<element_type>& input, const std::any& predicate)
{
    const auto snapshot = algorithm_working_copy(input);
    tx_vector<element_type> result(input.data().type_name);
    auto& output = result.data();
    for (const auto& item : snapshot)
    {
        if (invoke_algorithm_callback<std::uint8_t>(predicate, item) != 0)
        {
            output.values.push_back(item);
        }
    }
    output.refresh();
    return result;
}

template<class input_type, class output_type>
[[nodiscard]] output_type algorithm_fold(
    const tx_vector<input_type>& input, output_type initial,
    const std::any& combine)
{
    const auto snapshot = algorithm_working_copy(input);
    for (const auto& item : snapshot)
    {
        initial = invoke_algorithm_callback<output_type>(
            combine, initial, item);
    }
    return initial;
}

template<class element_type>
[[nodiscard]] bool algorithm_all(
    const tx_vector<element_type>& input, const std::any& predicate)
{
    const auto snapshot = algorithm_working_copy(input);
    for (const auto& item : snapshot)
    {
        if (invoke_algorithm_callback<std::uint8_t>(predicate, item) == 0)
        {
            return false;
        }
    }
    return true;
}

template<class element_type>
[[nodiscard]] bool algorithm_any(
    const tx_vector<element_type>& input, const std::any& predicate)
{
    const auto snapshot = algorithm_working_copy(input);
    for (const auto& item : snapshot)
    {
        if (invoke_algorithm_callback<std::uint8_t>(predicate, item) != 0)
        {
            return true;
        }
    }
    return false;
}

} // namespace tx_generated
