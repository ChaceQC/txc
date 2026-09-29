#include "stdlib/format_internal.hpp"
#include "stdlib/format_plan_cache.hpp"
#include "stdlib/format_arguments.hpp"
#include "stdlib/stdlib.hpp"

#include <charconv>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

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

struct field_state
{
    std::size_t next_auto = 0;
    bool used_auto = false;
    bool used_manual = false;
    std::unordered_map<std::string, std::size_t> positional_names;
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

template<class arguments_type>
const auto& resolve_field(const field& item, const arguments_type& args, field_state& state)
{
    if (item.name.empty())
    {
        if (state.used_manual)
        {
            throw std::runtime_error("format 不能混用自动和手动位置编号");
        }
        state.used_auto = true;
        if (state.next_auto >= args.size())
        {
            throw std::runtime_error("format 缺少位置参数");
        }
        return args[state.next_auto++];
    }
    if (is_digit(item.name.front()))
    {
        if (state.used_auto)
        {
            throw std::runtime_error("format 不能混用自动和手动位置编号");
        }
        state.used_manual = true;
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
    if (const auto* keyword = args.find(item.name))
    {
        return *keyword;
    }
    if (state.used_manual)
    {
        throw std::runtime_error("format 不能混用自动和手动位置编号");
    }
    state.used_auto = true;
    const auto name = std::string(item.name);
    if (const auto found = state.positional_names.find(name);
        found != state.positional_names.end())
    {
        return args[found->second];
    }
    if (state.next_auto >= args.size())
    {
        throw std::runtime_error("format 缺少位置参数供字段：" + name);
    }
    const auto index = state.next_auto++;
    state.positional_names.emplace(name, index);
    return args[index];
}

template<class arguments_type>
std::string format_values(const std::string& text, const arguments_type& args)
{
    if (const auto plan = find_format_plan(text))
    {
        std::string result;
        result.reserve(plan->literal_bytes);
        field_state state;
        for (const auto& part : plan->parts)
        {
            result += part.literal;
            if (part.field)
            {
                const field selected{part.name, {}, part.conversion};
                const auto& value = resolve_field(selected, args, state);
                append_format_value(result, value, part.spec, part.conversion);
            }
        }
        return result;
    }
    (void)tx_len(text);
    std::string result;
    field_state state;
    dynamic_format_plan plan;
    const bool cacheable = text.size() <= format_cache_template_bytes;
    std::size_t literal_start = 0;
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
            const auto& value = resolve_field(item, args, state);
            const auto spec = tx::parse_format_spec(item.spec);
            if (cacheable)
            {
                auto literal = result.substr(literal_start);
                plan.literal_bytes += literal.size();
                plan.parts.push_back({std::move(literal), std::string(item.name), spec, item.conversion, true});
            }
            append_format_value(result, value, spec, item.conversion);
            literal_start = result.size();
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
    if (cacheable)
    {
        auto literal = result.substr(literal_start);
        plan.literal_bytes += literal.size();
        plan.parts.push_back({std::move(literal), {}, {}, 0, false});
        plan.text = text;
        // 仅完整成功时缓存；非法模板始终按原扫描顺序报错，不提前解析后面的错误。
        remember_format_plan(std::move(plan));
    }
    return result;
}

} // namespace

std::string tx_fn_format(const std::string& text, const tx_array& args, const tx_dict& kwargs)
{
    return format_values(text, legacy_format_arguments{args, kwargs});
}

std::string format_direct(const std::string& text, std::span<const format_argument> positional,
    std::span<const format_argument> keywords)
{
    return format_values(text, direct_format_arguments{positional, keywords});
}

} // namespace tx_generated
