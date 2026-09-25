#pragma once

#include "backend/cpp/text_reference.hpp"

#include <any>
#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace tx_generated
{

// 仅格式化动态边界使用 std::any，容器内部始终保存原生元素。
std::string format_repr_value(const std::any& value);

template<class element_type>
std::string scalar_name()
{
    if constexpr (std::is_same_v<element_type, std::int64_t>)
    {
        return "int";
    }
    else if constexpr (std::is_same_v<element_type, double>)
    {
        return "float";
    }
    else if constexpr (std::is_same_v<element_type, std::uint8_t>)
    {
        return "bool";
    }
    else
    {
        return "str";
    }
}

template<class element_type>
decltype(auto) scalar_value(const element_type& value)
{
    if constexpr (std::is_same_v<element_type, text_reference>)
    {
        return value.get();
    }
    else if constexpr (std::is_same_v<element_type, std::uint8_t>)
    {
        return value != 0;
    }
    else
    {
        return value;
    }
}

template<class element_type>
void require_ordered_key(const element_type& value)
{
    if constexpr (std::is_same_v<element_type, double>)
    {
        if (std::isnan(value))
        {
            throw std::runtime_error("NaN 不能用作 map 键、set 元素或 heap 元素");
        }
    }
}

template<class element_type>
struct scalar_hash
{
    std::size_t operator()(const element_type& value) const
    {
        const auto& scalar = scalar_value(value);
        return std::hash<std::decay_t<decltype(scalar)>>{}(scalar);
    }
};

template<class element_type>
struct scalar_equal
{
    bool operator()(const element_type& left, const element_type& right) const
    {
        return scalar_value(left) == scalar_value(right);
    }
};

template<class element_type>
struct heap_compare
{
    bool descending = false;

    bool operator()(const element_type& left, const element_type& right) const
    {
        return descending ? scalar_value(left) < scalar_value(right)
                          : scalar_value(right) < scalar_value(left);
    }
};

} // namespace tx_generated
