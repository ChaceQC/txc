#include "backend/cpp/runtime.hpp"
#include "backend/cpp/vector_value.hpp"

#include <limits>
#include <stdexcept>
#include <typeinfo>
#include <utility>

namespace tx_generated
{

tx_array tx_make_array(tx_int length)
{
    if (length < 0)
    {
        throw std::runtime_error("数组长度不能为负");
    }
    return tx_array(static_cast<std::size_t>(length));
}

tx_array tx_make_array(tx_int length, tx_array initial)
{
    if (length < 0)
    {
        throw std::runtime_error("数组长度不能为负");
    }
    if (initial.size() > static_cast<std::size_t>(length))
    {
        throw std::runtime_error("数组初值数量超过声明长度");
    }
    tx_array result(static_cast<std::size_t>(length));
    const auto& source = static_cast<const tx_array&>(initial);
    for (std::size_t index = 0; index < initial.size(); ++index)
    {
        result[index] = source[index];
    }
    return result;
}

tx_int tx_len(const tx_array& values)
{
    if (values.size() > static_cast<std::size_t>(std::numeric_limits<tx_int>::max()))
    {
        throw std::overflow_error("数组长度超出 int 范围");
    }
    return static_cast<tx_int>(values.size());
}

tx_int tx_len(const std::any& value)
{
    tx_int vector_size = 0;
    if (visit_vector(value, [&](const auto& vector)
    {
        vector_size = static_cast<tx_int>(vector.data().values.size());
    }))
    {
        return vector_size;
    }
    if (value.type() == typeid(tx_array))
    {
        return tx_len(std::any_cast<const tx_array&>(value));
    }
    if (value.type() == typeid(std::string))
    {
        return tx_len(std::any_cast<const std::string&>(value));
    }
    if (value.type() == typeid(tx_dict))
    {
        const auto size = std::any_cast<const tx_dict&>(value).size();
        if (size > static_cast<std::size_t>(std::numeric_limits<tx_int>::max()))
        {
            throw std::overflow_error("字典长度超出 int 范围");
        }
        return static_cast<tx_int>(size);
    }
    throw std::runtime_error("len 的对象不是数组、字典或字符串");
}

std::any& tx_at(tx_array& values, tx_int index)
{
    if (index < 0 || static_cast<std::uint64_t>(index) >= values.size())
    {
        throw std::out_of_range("数组索引越界");
    }
    return values[static_cast<std::size_t>(index)];
}

const std::any& tx_at(const tx_array& values, tx_int index)
{
    if (index < 0 || static_cast<std::uint64_t>(index) >= values.size())
    {
        throw std::out_of_range("数组索引越界");
    }
    return values[static_cast<std::size_t>(index)];
}

std::any& tx_at(std::any& value, tx_int index)
{
    if (value.type() != typeid(tx_array))
    {
        throw std::runtime_error("索引对象不是数组");
    }
    return tx_at(std::any_cast<tx_array&>(value), index);
}

const std::any& tx_at(const std::any& value, tx_int index)
{
    if (value.type() != typeid(tx_array))
    {
        throw std::runtime_error("索引对象不是数组");
    }
    return tx_at(std::any_cast<const tx_array&>(value), index);
}

bool tx_is_none(const std::any& value)
{
    return !value.has_value();
}

tx_int tx_add(tx_int left, tx_int right)
{
    const auto maximum = std::numeric_limits<tx_int>::max();
    const auto minimum = std::numeric_limits<tx_int>::min();
    if ((right > 0 && left > maximum - right) ||
        (right < 0 && left < minimum - right))
    {
        throw std::overflow_error("整数加法溢出");
    }
    return left + right;
}

tx_int tx_sub(tx_int left, tx_int right)
{
    const auto maximum = std::numeric_limits<tx_int>::max();
    const auto minimum = std::numeric_limits<tx_int>::min();
    if ((right < 0 && left > maximum + right) ||
        (right > 0 && left < minimum + right))
    {
        throw std::overflow_error("整数减法溢出");
    }
    return left - right;
}

tx_int tx_mul(tx_int left, tx_int right)
{
    const auto maximum = std::numeric_limits<tx_int>::max();
    const auto minimum = std::numeric_limits<tx_int>::min();
    if (left == 0 || right == 0)
    {
        return 0;
    }
    if ((left == minimum && right == -1) ||
        (right == minimum && left == -1))
    {
        throw std::overflow_error("整数乘法溢出");
    }
    if (left > 0 && ((right > 0 && left > maximum / right) ||
                     (right < 0 && right < minimum / left)))
    {
        throw std::overflow_error("整数乘法溢出");
    }
    if (left < 0 && ((right > 0 && left < minimum / right) ||
                     (right < 0 && left < maximum / right)))
    {
        throw std::overflow_error("整数乘法溢出");
    }
    return left * right;
}

tx_int tx_div(tx_int left, tx_int right)
{
    if (right == 0)
    {
        throw std::runtime_error("整数除零");
    }
    if (left == std::numeric_limits<tx_int>::min() && right == -1)
    {
        throw std::overflow_error("整数除法溢出");
    }
    return left / right;
}

double tx_float_div(double left, double right)
{
    if (right == 0.0)
    {
        throw std::runtime_error("浮点数除零");
    }
    return left / right;
}

tx_int tx_neg(tx_int value)
{
    if (value == std::numeric_limits<tx_int>::min())
    {
        throw std::overflow_error("整数取负溢出");
    }
    return -value;
}

double tx_neg(double value)
{
    return -value;
}

} // namespace tx_generated
