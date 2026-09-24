#include "backend/cpp/value_format.hpp"

#include "stdlib/stdlib.hpp"

#include <stdexcept>
#include <string_view>
#include <typeinfo>

namespace tx_generated
{
namespace
{

void append_quoted(std::string& output, std::string_view text)
{
    constexpr char hex[] = "0123456789ABCDEF";
    output.push_back('"');
    for (const unsigned char value : text)
    {
        if (value == '\\' || value == '"')
        {
            output.push_back('\\');
            output.push_back(static_cast<char>(value));
        }
        else if (value == '\n' || value == '\r' || value == '\t')
        {
            output.push_back('\\');
            output.push_back(value == '\n' ? 'n' : value == '\r' ? 'r' : 't');
        }
        else if (value < 0x20 || value == 0x7f)
        {
            output += "\\x";
            output.push_back(hex[value >> 4]);
            output.push_back(hex[value & 0x0f]);
        }
        else
        {
            output.push_back(static_cast<char>(value));
        }
    }
    output.push_back('"');
}

void append_value(std::string& output, const std::any& value,
                  bool nested, std::size_t depth);

void append_array(std::string& output, const tx_array& items,
                  std::size_t depth)
{
    output.push_back('[');
    for (std::size_t index = 0; index < items.size(); ++index)
    {
        if (index != 0)
        {
            output += ", ";
        }
        append_value(output, items[index], true, depth + 1);
    }
    output.push_back(']');
}

void append_dict(std::string& output, const tx_dict& items,
                 std::size_t depth)
{
    output.push_back('{');
    std::size_t index = 0;
    for (const auto& [key, item] : items)
    {
        if (index++ != 0)
        {
            output += ", ";
        }
        append_value(output, key, true, depth + 1);
        output += ": ";
        append_value(output, item, true, depth + 1);
    }
    output.push_back('}');
}

void append_struct(std::string& output, const dynamic_struct& item,
                   std::size_t depth)
{
    output += item.display_name;
    output.push_back('(');
    for (std::size_t index = 0; index < item.fields.size(); ++index)
    {
        if (index != 0)
        {
            output += ", ";
        }
        output += item.fields[index].name;
        output.push_back('=');
        append_value(output, item.fields[index].value, true, depth + 1);
    }
    output.push_back(')');
}

void append_value(std::string& output, const std::any& value,
                  bool nested, std::size_t depth)
{
    // 限制递归展示深度，避免极深的动态嵌套耗尽运行时栈。
    if (depth > 64)
    {
        throw std::runtime_error("print 复合值嵌套过深");
    }
    if (!value.has_value())
    {
        output += "none";
    }
    else if (value.type() == typeid(tx_int))
    {
        output += tx_int_to_string(std::any_cast<const tx_int&>(value));
    }
    else if (value.type() == typeid(double))
    {
        output += tx_float_to_string(std::any_cast<const double&>(value));
    }
    else if (value.type() == typeid(bool))
    {
        output += tx_bool_to_string(std::any_cast<const bool&>(value));
    }
    else if (value.type() == typeid(std::string))
    {
        const auto& text = std::any_cast<const std::string&>(value);
        if (nested)
        {
            append_quoted(output, text);
        }
        else
        {
            output += text;
        }
    }
    else if (value.type() == typeid(tx_array))
    {
        append_array(output, std::any_cast<const tx_array&>(value), depth);
    }
    else if (value.type() == typeid(tx_dict))
    {
        append_dict(output, std::any_cast<const tx_dict&>(value), depth);
    }
    else if (value.type() == typeid(dynamic_struct))
    {
        append_struct(output, std::any_cast<const dynamic_struct&>(value), depth);
    }
    else
    {
        throw std::runtime_error("print 不支持此动态类型");
    }
}

} // namespace

std::string format_print_value(const std::any& value)
{
    std::string result;
    append_value(result, value, false, 0);
    return result;
}

} // namespace tx_generated
