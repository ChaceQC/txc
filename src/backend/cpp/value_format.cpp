#include "backend/cpp/value_format.hpp"
#include "backend/cpp/vector_value.hpp"
#include "backend/cpp/container_value.hpp"
#include "stdlib/iterator.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/cancellation.hpp"

#include "stdlib/stdlib.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/file_stream.hpp"
#include "stdlib/encoding_incremental.hpp"
#include "stdlib/regex.hpp"
#include "stdlib/filesystem_watch.hpp"
#include "stdlib/process.hpp"

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
    items.for_each([&](const std::any& key, const std::any& item)
    {
        if (index++ != 0)
        {
            output += ", ";
        }
        append_value(output, key, true, depth + 1);
        output += ": ";
        append_value(output, item, true, depth + 1);
    });
    output.push_back('}');
}

void append_struct(std::string& output, const dynamic_struct& item,
                   std::size_t depth)
{
    output += item->display_name;
    output.push_back('(');
    for (std::size_t index = 0; index < item->fields.size(); ++index)
    {
        if (index != 0)
        {
            output += ", ";
        }
        if (item->fields[index].name)
        {
            output += item->fields[index].name;
        }
        output.push_back('=');
        append_value(output, item->fields[index].value, true, depth + 1);
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
    else if (value.type() == typeid(byte_value))
    {
        output += "bytes(" + std::to_string(bytes_length(
            std::any_cast<const byte_value&>(value))) + ")";
    }
    else if (value.type() == typeid(binary_stream))
    {
        output += "<binary_stream>";
    }
    else if (value.type() == typeid(text_stream))
    {
        output += "<text_stream>";
    }
    else if (value.type() == typeid(cancel_source))
    {
        output += "<cancel_source>";
    }
    else if (value.type() == typeid(cancel_token))
    {
        output += "<cancel_token>";
    }
    else if (value.type() == typeid(encoding_decoder))
    {
        output += "<encoding_decoder>";
    }
    else if (value.type() == typeid(encoding_encoder))
    {
        output += "<encoding_encoder>";
    }
    else if (value.type() == typeid(regex_pattern))
    {
        output += "<regex_pattern>";
    }
    else if (value.type() == typeid(process_child))
    {
        output += "<process_child>";
    }
    else if (value.type() == typeid(process_pipe))
    {
        output += "<process_pipe>";
    }
    else if (value.type() == typeid(fs_watcher))
    {
        output += "<fs_watcher>";
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
    else if (value.type() == typeid(class_handle))
    {
        const auto& object = std::any_cast<const class_handle&>(value);
        output.push_back('<');
        output += object->display_name;
        output += " object>";
    }
    else if (const auto* container = std::any_cast<container_handle>(&value))
    {
        output += (*container)->repr();
    }
    else if (const auto* iterator = std::any_cast<tx_iterator>(&value))
    {
        output += "<iterator<" + iterator->data().element_type + ">>";
    }
    else if (const auto* closure = std::any_cast<closure_handle>(&value))
    {
        output += "<" + closure->data().type_name + ">";
    }
    else if (!visit_vector(value, [&](const auto& vector)
    {
        output.push_back('[');
        bool first = true;
        for (const auto& element : vector.data().values)
        {
            if (!first)
            {
                output += ", ";
            }
            first = false;
            using element_type = std::decay_t<decltype(element)>;
            if constexpr (std::is_same_v<element_type, text_reference>)
            {
                append_quoted(output, element.get());
            }
            else if constexpr (std::is_same_v<element_type, std::uint8_t>)
            {
                output += tx_bool_to_string(element != 0);
            }
            else if constexpr (std::is_same_v<element_type, double>)
            {
                output += tx_float_to_string(element);
            }
            else if constexpr (std::is_same_v<element_type, byte_value>)
            {
                output += "bytes(" + std::to_string(bytes_length(element)) + ")";
            }
            else if constexpr (std::is_same_v<element_type, std::any>)
            {
                append_value(output, element, true, depth + 1);
            }
            else
            {
                output += tx_int_to_string(element);
            }
        }
        output.push_back(']');
    }))
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

std::string format_repr_value(const std::any& value)
{
    std::string result;
    append_value(result, value, true, 0);
    return result;
}

} // namespace tx_generated
