#include "stdlib/algorithm.hpp"

#include <functional>
#include <limits>
#include <stdexcept>
#include <string>

namespace tx_generated
{
namespace
{

template<class element_type>
void require_finite_element(element_type value, const char* operation)
{
    if constexpr (std::is_same_v<element_type, double>)
    {
        if (!std::isfinite(value))
        {
            throw std::runtime_error(std::string(operation) + " 需要有限 float 元素");
        }
    }
}

template<class element_type, class compare_type>
element_type numeric_extreme(const tx_vector<element_type>& input,
                             compare_type compare, const char* operation)
{
    const auto& values = input.data().values;
    if (values.empty())
    {
        throw std::runtime_error(std::string(operation) + " 不能用于空 vector");
    }
    auto result = values.front();
    for (const auto& value : values)
    {
        require_finite_element(value, operation);
        if (compare(value, result))
        {
            result = value;
        }
    }
    return result;
}

} // namespace

std::int64_t tx_algorithm_sum(const int_vector& input)
{
    std::int64_t result = 0;
    for (const auto value : input.data().values)
    {
        // 在相加之前判断边界，避免 C++ 有符号溢出；中间溢出也属于错误。
        if ((value > 0 && result > std::numeric_limits<std::int64_t>::max() - value) ||
            (value < 0 && result < std::numeric_limits<std::int64_t>::min() - value))
        {
            throw std::runtime_error("algorithm.sum 的结果超出 int 范围");
        }
        result += value;
    }
    return result;
}

double tx_algorithm_sum(const float_vector& input)
{
    double result = 0.0;
    for (const auto value : input.data().values)
    {
        require_finite_element(value, "algorithm.sum");
        result += value;
        if (!std::isfinite(result))
        {
            throw std::runtime_error("algorithm.sum 的结果不是有限 float");
        }
    }
    return result;
}

std::int64_t tx_algorithm_min_element(const int_vector& input)
{
    return numeric_extreme(input, std::less<>{}, "algorithm.min_element");
}

double tx_algorithm_min_element(const float_vector& input)
{
    return numeric_extreme(input, std::less<>{}, "algorithm.min_element");
}

std::int64_t tx_algorithm_max_element(const int_vector& input)
{
    return numeric_extreme(input, std::greater<>{}, "algorithm.max_element");
}

double tx_algorithm_max_element(const float_vector& input)
{
    return numeric_extreme(input, std::greater<>{}, "algorithm.max_element");
}

} // namespace tx_generated
