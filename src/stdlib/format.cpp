#include "stdlib/format_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <charconv>
#include <stdexcept>
#include <string_view>

namespace tx_generated
{
namespace
{

struct field
{
    std::string_view name;
    std::string_view spec;
    char conversion = 0;
};

bool is_digit(char value)
{
    return value >= '0' && value <= '9';
}

bool is_name_start(char value)
{
    return (value >= 'a' && value <= 'z') ||
           (value >= 'A' && value <= 'Z') || value == '_';
}

bool valid_name(std::string_view name)
{
    if (name.empty() || !is_name_start(name.front()))
    {
        return false;
    }
    for (const char value : name.substr(1))
    {
        if (!is_name_start(value) && !is_digit(value))
        {
            return false;
        }
    }
    return true;
}

std::size_t parse_index(std::string_view name)
{
    std::size_t result = 0;
    const auto [end, error] =
        std::from_chars(name.data(), name.data() + name.size(), result);
    if (error != std::errc{} || end != name.data() + name.size())
    {
        throw std::runtime_error("format 位置参数下标无效");
    }
    return result;
}

field parse_field(std::string_view text)
{
    field result;
    const auto marker = text.find_first_of("!:");
    result.name = text.substr(0, marker);
    if (marker == std::string_view::npos)
    {
        return result;
    }
    std::size_t offset = marker;
    if (text[offset] == '!')
    {
        if (++offset == text.size() ||
            (text[offset] != 's' && text[offset] != 'r'))
        {
            throw std::runtime_error("format 只支持 !s 和 !r 转换");
        }
        result.conversion = text[offset++];
    }
    if (offset < text.size())
    {
        if (text[offset] != ':')
        {
            throw std::runtime_error("format 字段语法无效");
        }
        result.spec = text.substr(offset + 1);
    }
    return result;
}

const std::any& find_keyword(const tx_dict& kwargs, std::string_view name)
{
    for (const auto& [key, value] : kwargs)
    {
        if (key.type() == typeid(std::string) &&
            std::any_cast<const std::string&>(key) == name)
        {
            return value;
        }
    }
    throw std::runtime_error("format 缺少命名参数：" + std::string(name));
}

const std::any& resolve_field(const field& item, const tx_array& args,
                              const tx_dict& kwargs, std::size_t& next_auto,
                              bool& used_auto, bool& used_manual)
{
    if (item.name.empty())
    {
        if (used_manual)
        {
            throw std::runtime_error("format 不能混用自动和手动位置编号");
        }
        used_auto = true;
        if (next_auto >= args.size())
        {
            throw std::runtime_error("format 缺少位置参数");
        }
        return args[next_auto++];
    }
    if (is_digit(item.name.front()))
    {
        if (used_auto)
        {
            throw std::runtime_error("format 不能混用自动和手动位置编号");
        }
        used_manual = true;
        const auto index = parse_index(item.name);
        if (index >= args.size())
        {
            throw std::runtime_error("format 位置参数下标越界");
        }
        return args[index];
    }
    if (!valid_name(item.name))
    {
        throw std::runtime_error("format 不支持此字段名称或字段访问：" +
                                 std::string(item.name));
    }
    return find_keyword(kwargs, item.name);
}

} // namespace

std::string tx_fn_format(const std::string& text, const tx_array& args,
                         const tx_dict& kwargs)
{
    (void)tx_len(text);
    std::string result;
    std::size_t next_auto = 0;
    bool used_auto = false;
    bool used_manual = false;
    for (std::size_t offset = 0; offset < text.size();)
    {
        const char current = text[offset];
        if (current == '{' && offset + 1 < text.size() &&
            text[offset + 1] == '{')
        {
            result.push_back('{');
            offset += 2;
        }
        else if (current == '}' && offset + 1 < text.size() &&
                 text[offset + 1] == '}')
        {
            result.push_back('}');
            offset += 2;
        }
        else if (current == '{')
        {
            const auto closing = text.find('}', offset + 1);
            if (closing == std::string::npos ||
                text.find('{', offset + 1) < closing)
            {
                throw std::runtime_error("format 替换字段缺少匹配的右大括号");
            }
            const auto item = parse_field(std::string_view(text).substr(
                offset + 1, closing - offset - 1));
            const auto& value = resolve_field(item, args, kwargs, next_auto,
                                              used_auto, used_manual);
            result += format_field_value(value, item.spec, item.conversion);
            offset = closing + 1;
        }
        else if (current == '}')
        {
            throw std::runtime_error("format 存在孤立的右大括号");
        }
        else
        {
            result.push_back(current);
            ++offset;
        }
    }
    return result;
}

} // namespace tx_generated
