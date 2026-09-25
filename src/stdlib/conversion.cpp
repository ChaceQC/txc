#include "stdlib/stdlib.hpp"
#include "stdlib/error.hpp"

#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <system_error>

namespace tx_generated
{
namespace
{

bool is_ascii_space(char value)
{
    return value == ' ' || value == '\t' || value == '\n' ||
           value == '\r' || value == '\v' || value == '\f';
}

std::string_view trim_ascii(std::string_view text)
{
    while (!text.empty() && is_ascii_space(text.front()))
    {
        text.remove_prefix(1);
    }
    while (!text.empty() && is_ascii_space(text.back()))
    {
        text.remove_suffix(1);
    }
    return text;
}

} // namespace

tx_int tx_parse_int(const std::string& text)
{
    auto value = trim_ascii(text);
    if (!value.empty() && value.front() == '+')
    {
        value.remove_prefix(1);
    }
    tx_int result = 0;
    const auto [end, error] =
        std::from_chars(value.data(), value.data() + value.size(), result);
    if (value.empty() || error != std::errc{} || end != value.data() + value.size())
    {
        throw runtime_failure({tx::error_kind::parse, "invalid_syntax", "字符串不能转换为 int"});
    }
    return result;
}

double tx_parse_float(const std::string& text)
{
    auto value = trim_ascii(text);
    if (!value.empty() && value.front() == '+')
    {
        value.remove_prefix(1);
    }
    double result = 0.0;
    const auto [end, error] =
        std::from_chars(value.data(), value.data() + value.size(),
                        result, std::chars_format::general);
    if (value.empty() || error != std::errc{} ||
        end != value.data() + value.size() || !std::isfinite(result))
    {
        throw runtime_failure({tx::error_kind::parse, "invalid_syntax", "字符串不能转换为 float"});
    }
    return result;
}

tx_int tx_float_to_int(double value)
{
    const auto limit = std::ldexp(1.0, 63);
    if (!std::isfinite(value) || value < -limit || value >= limit)
    {
        throw std::runtime_error("float 转 int 超出范围");
    }
    return static_cast<tx_int>(value);
}

std::string tx_int_to_string(tx_int value)
{
    return std::to_string(value);
}

std::string tx_float_to_string(double value)
{
    char buffer[64];
    const auto [end, error] =
        std::to_chars(buffer, buffer + sizeof(buffer),
                      value, std::chars_format::general);
    if (error != std::errc{})
    {
        throw std::runtime_error("float 转 str 失败");
    }
    std::string result(buffer, end);
    if (std::isfinite(value) && result.find_first_of(".eE") == std::string::npos)
    {
        result += ".0";
    }
    return result;
}

std::string tx_bool_to_string(bool value)
{
    return value ? "true" : "false";
}

tx_int tx_to_int(const std::any& value)
{
    if (value.type() == typeid(tx_int))
    {
        return std::any_cast<const tx_int&>(value);
    }
    if (value.type() == typeid(double))
    {
        return tx_float_to_int(std::any_cast<const double&>(value));
    }
    if (value.type() == typeid(std::string))
    {
        return tx_parse_int(std::any_cast<const std::string&>(value));
    }
    throw std::runtime_error("as int 需要 int、float 或 str");
}

double tx_to_float(const std::any& value)
{
    if (value.type() == typeid(tx_int))
    {
        return static_cast<double>(std::any_cast<const tx_int&>(value));
    }
    if (value.type() == typeid(double))
    {
        return std::any_cast<const double&>(value);
    }
    if (value.type() == typeid(std::string))
    {
        return tx_parse_float(std::any_cast<const std::string&>(value));
    }
    throw std::runtime_error("as float 需要 int、float 或 str");
}

std::string tx_to_string(const std::any& value)
{
    if (value.type() == typeid(tx_int))
    {
        return tx_int_to_string(std::any_cast<const tx_int&>(value));
    }
    if (value.type() == typeid(double))
    {
        return tx_float_to_string(std::any_cast<const double&>(value));
    }
    if (value.type() == typeid(bool))
    {
        return tx_bool_to_string(std::any_cast<const bool&>(value));
    }
    if (value.type() == typeid(std::string))
    {
        return std::any_cast<const std::string&>(value);
    }
    throw std::runtime_error("as str 需要 int、float、bool 或 str");
}

} // namespace tx_generated
