#pragma once

#include "stdlib/container_scalar.hpp"
#include "stdlib/vector.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace tx_generated
{
namespace detail
{

template<class element_type>
struct algorithm_less
{
    bool operator()(const element_type& left, const element_type& right) const
    {
        if constexpr (std::is_same_v<element_type, double>)
        {
            // NaN 统一置后，使排序和二分共享严格弱序；数值相等仍沿用 IEEE 规则。
            return !std::isnan(left) && (std::isnan(right) || left < right);
        }
        else
        {
            // std::string 按无符号字节比较，文本引用只解引用，不复制载荷。
            return scalar_value(left) < scalar_value(right);
        }
    }
};

} // namespace detail

template<class element_type>
void tx_algorithm_sort(const tx_vector<element_type>& input)
{
    auto& values = input.data().values;
    std::sort(values.begin(), values.end(), detail::algorithm_less<element_type>{});
    // 只重排元素，不改变存储地址、长度或容量，已有 vector_view 仍然有效。
}

template<class element_type>
tx_vector<element_type> tx_algorithm_sorted(const tx_vector<element_type>& input)
{
    auto result = input.copy();
    tx_algorithm_sort(result);
    return result;
}

template<class element_type>
std::int64_t tx_algorithm_find(const tx_vector<element_type>& input,
                               const element_type& value)
{
    const auto& values = input.data().values;
    const auto found = std::find_if(values.begin(), values.end(), [&](const auto& item)
    {
        return scalar_equal<element_type>{}(item, value);
    });
    return found == values.end() ? -1 : static_cast<std::int64_t>(found - values.begin());
}

template<class element_type>
std::int64_t tx_algorithm_count(const tx_vector<element_type>& input,
                                const element_type& value)
{
    const auto& values = input.data().values;
    return static_cast<std::int64_t>(std::count_if(
        values.begin(), values.end(), [&](const auto& item)
        {
            return scalar_equal<element_type>{}(item, value);
        }));
}

template<class element_type>
std::int64_t tx_algorithm_lower_bound(const tx_vector<element_type>& input,
                                      const element_type& value)
{
    const auto& values = input.data().values;
    // 有序是调用方前提，不引入每次 O(n) 的验证扫描。
    const auto found = std::lower_bound(values.begin(), values.end(), value,
                                       detail::algorithm_less<element_type>{});
    return static_cast<std::int64_t>(found - values.begin());
}

template<class element_type>
std::int64_t tx_algorithm_upper_bound(const tx_vector<element_type>& input,
                                      const element_type& value)
{
    const auto& values = input.data().values;
    const auto found = std::upper_bound(values.begin(), values.end(), value,
                                       detail::algorithm_less<element_type>{});
    return static_cast<std::int64_t>(found - values.begin());
}

template<class element_type>
void tx_algorithm_reverse(const tx_vector<element_type>& input)
{
    auto& values = input.data().values;
    std::reverse(values.begin(), values.end());
}

std::int64_t tx_algorithm_sum(const int_vector& input);
double tx_algorithm_sum(const float_vector& input);
std::int64_t tx_algorithm_min_element(const int_vector& input);
double tx_algorithm_min_element(const float_vector& input);
std::int64_t tx_algorithm_max_element(const int_vector& input);
double tx_algorithm_max_element(const float_vector& input);

} // namespace tx_generated
