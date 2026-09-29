#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <unordered_set>

namespace tx
{
namespace
{

struct literal_argument
{
    const expression* value;
    std::optional<std::string> keyword;
    std::size_t target = 0;
};

std::optional<std::vector<literal_argument>> flatten_literals(const call_expression& call,
    std::string (*decode)(std::string_view))
{
    std::vector<literal_argument> result;
    std::unordered_set<std::string> keywords;
    for (const auto& argument : call.arguments)
    {
        if (argument.kind == argument_kind::spread_array)
        {
            const auto* array = std::get_if<array_literal>(&argument.value->data);
            if (!array)
            {
                return std::nullopt;
            }
            for (const auto& element : array->elements)
            {
                result.push_back({element.get(), std::nullopt});
            }
        }
        else if (argument.kind == argument_kind::spread_dict)
        {
            const auto* dictionary = std::get_if<dictionary_literal>(&argument.value->data);
            if (!dictionary)
            {
                return std::nullopt;
            }
            for (const auto& entry : dictionary->entries)
            {
                const auto* key = std::get_if<string_literal>(&entry.key->data);
                if (!key)
                {
                    return std::nullopt;
                }
                auto name = decode(key->text);
                if (name.find('\0') != std::string::npos || !keywords.insert(name).second)
                {
                    return std::nullopt;
                }
                result.push_back({entry.value.get(), std::move(name)});
            }
        }
        else
        {
            std::optional<std::string> name;
            if (argument.kind == argument_kind::keyword)
            {
                name = argument.name;
                if (!keywords.insert(*name).second)
                {
                    return std::nullopt;
                }
            }
            result.push_back({argument.value.get(), std::move(name)});
        }
    }
    return result;
}

bool bind_literals(std::vector<literal_argument>& arguments,
    const std::vector<parameter>& parameters, std::size_t fixed_count)
{
    std::vector<bool> filled(fixed_count, false);
    const auto variadic = [&](parameter_kind kind)
    {
        return std::find_if(parameters.begin(), parameters.end(),
            [&](const parameter& parameter)
            {
                return parameter.kind == kind;
            });
    };
    std::size_t positional = 0;
    for (auto& argument : arguments)
    {
        auto target = parameters.end();
        if (argument.keyword)
        {
            target = std::find_if(parameters.begin(), parameters.begin() + fixed_count,
                [&](const parameter& parameter)
                {
                    return parameter.name == *argument.keyword;
                });
            if (target == parameters.begin() + fixed_count)
            {
                target = variadic(parameter_kind::variadic_dict);
            }
        }
        else
        {
            target = positional < fixed_count ? parameters.begin() + positional :
                variadic(parameter_kind::variadic_array);
            ++positional;
        }
        if (target == parameters.end())
        {
            return false;
        }
        argument.target = static_cast<std::size_t>(target - parameters.begin());
        if (argument.target < fixed_count)
        {
            // 类型不完全匹配时保留运行时 require_type（包括动态 any / 向下转换）。
            if (filled[argument.target] || argument.value->type != target->type ||
                parameter_is_nullable(*target))
            {
                return false;
            }
            filled[argument.target] = true;
        }
    }
    for (std::size_t index = 0; index < fixed_count; ++index)
    {
        if (!filled[index] && !parameters[index].default_value)
        {
            return false;
        }
    }
    return true;
}

} // namespace

std::optional<std::vector<llvm_code_generator::ir_value>>
llvm_code_generator::emit_literal_spreads(const expression& item,
    const call_expression& call, const std::vector<parameter>& parameters)
{
    auto arguments = flatten_literals(call, decode_string_literal);
    const auto fixed_count = static_cast<std::size_t>(std::count_if(
        parameters.begin(), parameters.end(), [](const parameter& parameter)
        {
            return parameter.kind == parameter_kind::ordinary;
        }));
    if (!arguments || !bind_literals(*arguments, parameters, fixed_count))
    {
        return std::nullopt;
    }
    std::vector<ir_value> result(parameters.size(), {value_type::void_type, {}});
    std::vector<ir_value> extra_positional;
    std::vector<std::pair<std::string, ir_value>> extra_keywords;
    for (const auto& argument : *arguments)
    {
        const auto value = expression_value(*argument.value);
        if (argument.target < fixed_count)
        {
            result[argument.target] = value;
        }
        else if (argument.keyword)
        {
            extra_keywords.emplace_back(*argument.keyword, value);
        }
        else
        {
            extra_positional.push_back(value);
        }
    }
    for (std::size_t index = 0; index < fixed_count; ++index)
    {
        if (result[index].type == value_type::void_type)
        {
            result[index] = expression_value(*parameters[index].default_value);
        }
    }
    for (std::size_t index = fixed_count; index < parameters.size(); ++index)
    {
        result[index] = parameters[index].kind == parameter_kind::variadic_array
            ? emit_variadic_array(extra_positional, item.position)
            : emit_variadic_dict(extra_keywords, item.position);
    }
    return result;
}

} // namespace tx
