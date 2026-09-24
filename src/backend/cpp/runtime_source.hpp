#pragma once

#include <string_view>

namespace tx
{

inline constexpr std::string_view runtime_source = R"TXCPP(
#include <any>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <vector>

namespace tx_generated
{

using tx_int = std::int64_t;
using tx_array = std::vector<std::any>;

std::string tx_input();
std::string tx_input(const std::string& prompt);
void tx_prepare_console();
tx_int tx_parse_int(const std::string& text);
double tx_parse_float(const std::string& text);
tx_int tx_float_to_int(double value);
std::string tx_int_to_string(tx_int value);
std::string tx_float_to_string(double value);
std::string tx_bool_to_string(bool value);
tx_int tx_to_int(const std::any& value);
double tx_to_float(const std::any& value);
std::string tx_to_string(const std::any& value);
tx_int tx_len(const std::string& text);
void tx_print(tx_int value);
void tx_print(double value);
void tx_print(bool value);
void tx_print(const std::string& value);
void tx_print(const std::any& value);

template<class value>
std::any tx_box(value&& item)
{
    return std::any(std::forward<value>(item));
}

std::any tx_box(const std::any& item)
{
    return item;
}

std::any tx_box(std::any&& item)
{
    return std::move(item);
}

tx_array tx_make_array(tx_int length, tx_array initial = {})
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
    for (std::size_t index = 0; index < initial.size(); ++index)
    {
        result[index] = std::move(initial[index]);
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
    if (value.type() == typeid(tx_array))
    {
        return tx_len(std::any_cast<const tx_array&>(value));
    }
    if (value.type() == typeid(std::string))
    {
        return tx_len(std::any_cast<const std::string&>(value));
    }
    throw std::runtime_error("len 的对象不是数组或字符串");
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

)TXCPP";

} // namespace tx
