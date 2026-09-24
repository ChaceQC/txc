#include "frontend/sema/sema.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace tx
{
namespace
{

bool is_numeric(const value_type& type)
{
    return type == value_type::int_type || type == value_type::float_type;
}

bool is_printable(const value_type& type)
{
    return is_numeric(type) || type == value_type::bool_type ||
           type == value_type::str_type || type == value_type::none_type ||
           type == value_type::any_type;
}

bool matches_signature(const function_signature& signature,
                       const call_expression& call,
                       const std::vector<value_type>& types)
{
    std::size_t fixed_count = 0;
    bool accepts_args = false;
    bool accepts_kwargs = false;
    for (const auto& parameter : signature.parameters)
    {
        if (parameter.kind == parameter_kind::ordinary)
        {
            ++fixed_count;
        }
        else if (parameter.kind == parameter_kind::variadic_array)
        {
            accepts_args = true;
        }
        else
        {
            accepts_kwargs = true;
        }
    }
    std::vector<bool> filled(fixed_count, false);
    std::unordered_set<std::string> keyword_names;
    std::size_t positional = 0;
    bool has_spread = false;
    for (std::size_t index = 0; index < call.arguments.size(); ++index)
    {
        const auto& argument = call.arguments[index];
        const auto& type = types[index];
        if (argument.kind == argument_kind::spread_array ||
            argument.kind == argument_kind::spread_dict)
        {
            has_spread = true;
            continue;
        }
        if (argument.kind == argument_kind::positional)
        {
            if (has_spread)
            {
                continue;
            }
            if (positional < fixed_count)
            {
                if (signature.parameters[positional].type != type)
                {
                    return false;
                }
                filled[positional] = true;
            }
            else if (!accepts_args)
            {
                return false;
            }
            ++positional;
            continue;
        }
        if (!keyword_names.insert(argument.name).second)
        {
            return false;
        }
        std::size_t parameter_index = 0;
        while (parameter_index < fixed_count &&
               signature.parameters[parameter_index].name != argument.name)
        {
            ++parameter_index;
        }
        if (parameter_index == fixed_count)
        {
            if (!accepts_kwargs)
            {
                return false;
            }
            continue;
        }
        if (filled[parameter_index] ||
            signature.parameters[parameter_index].type != type)
        {
            return false;
        }
        filled[parameter_index] = true;
    }
    if (!has_spread)
    {
        for (const bool present : filled)
        {
            if (!present)
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace

value_type semantic_analyzer::check_builtin(expression& item, call_expression& call)
{
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position, "内置函数只接受普通位置实参");
        }
    }
    if (call.name == "input")
    {
        if (call.arguments.size() > 1)
        {
            throw compile_error(item.position, "input 最多接收一个提示字符串");
        }
        if (!call.arguments.empty())
        {
            require_type(check_expression(*call.arguments.front().value),
                         value_type::str_type,
                         call.arguments.front().position, "input 提示");
        }
        return value_type::str_type;
    }
    if (call.arguments.size() != 1)
    {
        throw compile_error(item.position, call.name + " 需要一个参数");
    }
    auto& argument = *call.arguments.front().value;
    const auto actual = check_expression(argument);
    if (call.name == "print")
    {
        if (!is_printable(actual))
        {
            throw compile_error(argument.position, "print 不支持此类型：" +
                                                    std::string(type_name(actual)));
        }
        return value_type::void_type;
    }
    if (call.name == "len")
    {
        if (actual != value_type::array_type &&
            actual != value_type::dict_type &&
            actual != value_type::str_type &&
            actual != value_type::any_type)
        {
            throw compile_error(argument.position, "len 需要数组、字典或字符串");
        }
        return value_type::int_type;
    }
    if (call.name == "to_float")
    {
        require_type(actual, value_type::int_type, argument.position, "to_float 参数");
        return value_type::float_type;
    }
    if (actual != value_type::any_type && actual != value_type::none_type)
    {
        throw compile_error(argument.position, "is_none 只接受数组元素或 none");
    }
    return value_type::bool_type;
}

value_type semantic_analyzer::check_constructor(expression& item,
                                                call_expression& call)
{
    const auto& definition = *structs_.at(call.name);
    function_signature signature{{}, value_type(call.name)};
    for (const auto& field : definition.fields)
    {
        signature.parameters.push_back({field.name, field.type, field.position});
    }
    std::vector<value_type> types;
    for (auto& argument : call.arguments)
    {
        types.push_back(check_expression(*argument.value));
        if (argument.kind == argument_kind::spread_array &&
            types.back() != value_type::array_type &&
            types.back() != value_type::any_type)
        {
            throw compile_error(argument.position, "* 需要数组");
        }
        if (argument.kind == argument_kind::spread_dict &&
            types.back() != value_type::dict_type &&
            types.back() != value_type::any_type)
        {
            throw compile_error(argument.position, "** 需要字典");
        }
    }
    if (!matches_signature(signature, call, types))
    {
        throw compile_error(item.position, "结构体构造参数不匹配：" + call.source_name);
    }
    call.is_constructor = true;
    return value_type(call.name);
}

value_type semantic_analyzer::check_call(expression& item, call_expression& call)
{
    const auto& display_name = call.source_name.empty()
        ? call.name : call.source_name;
    if (call.name == "print" || call.name == "len" ||
        call.name == "to_float" || call.name == "input" ||
        call.name == "is_none")
    {
        return check_builtin(item, call);
    }
    if (structs_.contains(call.name))
    {
        return check_constructor(item, call);
    }
    const auto found = functions_.find(call.name);
    if (found == functions_.end())
    {
        throw compile_error(item.position, "未定义函数：" + display_name);
    }
    std::vector<value_type> actual_types;
    actual_types.reserve(call.arguments.size());
    for (auto& argument : call.arguments)
    {
        actual_types.push_back(check_expression(*argument.value));
        if (argument.kind == argument_kind::spread_array &&
            actual_types.back() != value_type::array_type &&
            actual_types.back() != value_type::any_type)
        {
            throw compile_error(argument.position, "* 需要数组");
        }
        if (argument.kind == argument_kind::spread_dict &&
            actual_types.back() != value_type::dict_type &&
            actual_types.back() != value_type::any_type)
        {
            throw compile_error(argument.position, "** 需要字典");
        }
    }
    std::vector<std::size_t> candidates;
    for (std::size_t index = 0; index < found->second.size(); ++index)
    {
        const auto& signature = found->second[index];
        if (matches_signature(signature, call, actual_types))
        {
            candidates.push_back(index);
        }
    }
    if (candidates.size() == 1)
    {
        call.overload_index = candidates.front();
        return found->second[candidates.front()].result;
    }
    if (candidates.size() > 1)
    {
        throw compile_error(item.position, "函数调用的重载不唯一：" + display_name);
    }
    std::string actual = display_name + "(";
    for (std::size_t index = 0; index < actual_types.size(); ++index)
    {
        if (index != 0)
        {
            actual += ", ";
        }
        actual += type_name(actual_types[index]);
    }
    throw compile_error(item.position, "没有匹配的函数重载：" + actual + ")");
}

} // namespace tx
