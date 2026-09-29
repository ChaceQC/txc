#include "backend/llvm/codegen_format_plan.hpp"
#include "common/utf8.hpp"

#include <charconv>
#include <stdexcept>
#include <unordered_map>

namespace tx
{
namespace
{

bool name_start(char value)
{
    return (value >= 'a' && value <= 'z') ||
        (value >= 'A' && value <= 'Z') || value == '_';
}

bool digit(char value)
{
    return value >= '0' && value <= '9';
}

class format_binding
{
public:
    std::vector<std::size_t> positional;
    std::unordered_map<std::string, std::size_t> keywords;

    std::optional<std::size_t> resolve(std::string_view name)
    {
        if (!name.empty() && !digit(name.front()))
        {
            if (!name_start(name.front()))
            {
                return std::nullopt;
            }
            for (const auto character : name)
            {
                if (!name_start(character) && !digit(character))
                {
                    return std::nullopt;
                }
            }
            if (const auto found = keywords.find(std::string(name)); found != keywords.end())
            {
                return found->second;
            }
        }
        const bool manual = !name.empty() && digit(name.front());
        if ((manual && used_auto_) || (!manual && used_manual_))
        {
            return std::nullopt;
        }
        std::size_t index = next_auto_;
        if (manual)
        {
            used_manual_ = true;
            const auto [end, error] = std::from_chars(name.data(), name.data() + name.size(), index);
            if (error != std::errc{} || end != name.data() + name.size())
            {
                return std::nullopt;
            }
        }
        else
        {
            used_auto_ = true;
            if (!name.empty())
            {
                if (const auto found = names_.find(std::string(name)); found != names_.end())
                {
                    return found->second;
                }
            }
            ++next_auto_;
        }
        if (index >= positional.size())
        {
            return std::nullopt;
        }
        if (!manual && !name.empty())
        {
            names_.emplace(name, positional[index]);
        }
        return positional[index];
    }

private:
    bool used_auto_ = false;
    bool used_manual_ = false;
    std::size_t next_auto_ = 0;
    std::unordered_map<std::string, std::size_t> names_;
};

std::optional<static_format_part> parse_part(std::string_view field, format_binding& binding)
{
    static_format_part result;
    auto marker = field.find_first_of("!:");
    result.argument = binding.resolve(field.substr(0, marker));
    if (!result.argument)
    {
        return std::nullopt;
    }
    if (marker != std::string_view::npos && field[marker] == '!')
    {
        if (++marker == field.size() || (field[marker] != 's' && field[marker] != 'r'))
        {
            return std::nullopt;
        }
        result.conversion = field[marker++];
    }
    if (marker != std::string_view::npos && marker < field.size())
    {
        if (field[marker] != ':')
        {
            return std::nullopt;
        }
        result.spec = parse_format_spec(field.substr(marker + 1));
    }
    return result;
}

} // namespace

std::optional<std::vector<static_format_part>> plan_static_format(
    std::string_view text, const call_expression& call)
{
    for (std::size_t offset = 0; offset < text.size();)
    {
        const auto width = utf8_width(text, offset);
        if (width == 0)
        {
            return std::nullopt;
        }
        offset += width;
    }
    format_binding binding;
    for (std::size_t index = 1; index < call.arguments.size(); ++index)
    {
        const auto& argument = call.arguments[index];
        if (argument.kind == argument_kind::positional)
        {
            binding.positional.push_back(index);
        }
        else if (argument.kind == argument_kind::keyword)
        {
            binding.keywords.emplace(argument.name, index);
        }
        else
        {
            return std::nullopt;
        }
    }
    std::vector<static_format_part> result;
    std::string literal;
    for (std::size_t offset = 0; offset < text.size();)
    {
        const auto current = text[offset++];
        if ((current == '{' || current == '}') && offset < text.size() && text[offset] == current)
        {
            literal += current;
            ++offset;
        }
        else if (current == '{')
        {
            const auto closing = text.find('}', offset);
            if (closing == std::string_view::npos || text.find('{', offset) < closing)
            {
                return std::nullopt;
            }
            if (!literal.empty())
            {
                result.push_back({std::move(literal), std::nullopt, {}, 0});
                literal.clear();
            }
            try
            {
                auto part = parse_part(text.substr(offset, closing - offset), binding);
                if (!part)
                {
                    return std::nullopt;
                }
                result.push_back(std::move(*part));
            }
            catch (const std::runtime_error&)
            {
                // 非法模板仍走原运行时入口，保留实参副作用和可捕获的错误。
                return std::nullopt;
            }
            offset = closing + 1;
        }
        else if (current == '}')
        {
            return std::nullopt;
        }
        else
        {
            literal += current;
        }
    }
    if (!literal.empty())
    {
        result.push_back({std::move(literal), std::nullopt, {}, 0});
    }
    std::vector<static_format_part> fused;
    for (auto& part : result)
    {
        if (!part.argument && !fused.empty() && fused.back().argument)
        {
            fused.back().tail = std::move(part.literal);
        }
        else
        {
            fused.push_back(std::move(part));
        }
    }
    return fused;
}

bool plain_format_part(const static_format_part& part, const value_type& type)
{
    return (type == value_type::int_type && format_plain_integer(part.spec, part.conversion)) ||
        (type == value_type::bool_type && format_plain_bool(part.spec, part.conversion)) ||
        (type == value_type::str_type && format_plain_text(part.spec, part.conversion));
}

} // namespace tx
