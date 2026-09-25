#include "frontend/sema/sema.hpp"

#include <algorithm>
#include <string>
#include <limits>
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
           type == value_type::str_type || type == value_type::bytes_type ||
           type == value_type::none_type ||
           type == value_type::array_type || type == value_type::dict_type ||
           type == value_type::any_type || type.is_vector() || type.is_typed_container();
}

} // namespace

bool semantic_analyzer::matches_signature(
    const function_signature& signature, const call_expression& call,
    const std::vector<value_type>& types, bool allow_upcast) const
{
    const auto accepts = [this, allow_upcast, &signature](
        const value_type& actual, const value_type& expected)
    {
        if (signature.accepts_any_value && expected == value_type::any_type)
        {
            return actual != value_type::void_type &&
                   actual != value_type::unknown_type && !actual.is_function();
        }
        return allow_upcast ? is_assignable(actual, expected)
                            : actual == expected;
    };
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
                if (!accepts(type, signature.parameters[positional].type))
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
            !accepts(type, signature.parameters[parameter_index].type))
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

value_type semantic_analyzer::check_builtin(expression& item, call_expression& call)
{
    if (call.name == "print")
    {
        bool seen_keyword = false;
        bool seen_sep = false;
        bool seen_end = false;
        for (const auto& argument : call.arguments)
        {
            if (argument.kind == argument_kind::positional)
            {
                if (seen_keyword)
                {
                    throw compile_error(argument.position,
                                        "print 的值必须写在 sep 和 end 前面");
                }
                const auto actual = check_expression(*argument.value);
                if (!is_printable(actual) && !structs_.contains(actual.name) &&
                    !classes_.contains(actual.name))
                {
                    throw compile_error(argument.position, "print 不支持此类型：" +
                                                        std::string(type_name(actual)));
                }
                continue;
            }
            if (argument.kind != argument_kind::keyword ||
                (argument.name != "sep" && argument.name != "end"))
            {
                throw compile_error(argument.position,
                                    "print 只接受位置值以及 sep、end 命名参数");
            }
            seen_keyword = true;
            bool& seen = argument.name == "sep" ? seen_sep : seen_end;
            if (seen)
            {
                throw compile_error(argument.position,
                                    "print 重复指定 " + argument.name);
            }
            seen = true;
            require_type(check_expression(*argument.value), value_type::str_type,
                         argument.position, "print " + argument.name);
        }
        return value_type::void_type;
    }
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position, "内置函数只接受普通位置实参");
        }
    }
    if (call.name == "input" || call.name == "input_or_none")
    {
        if (call.arguments.size() > 1)
        {
            throw compile_error(item.position,
                                call.name + " 最多接收一个提示字符串");
        }
        if (!call.arguments.empty())
        {
            require_type(check_expression(*call.arguments.front().value),
                         value_type::str_type,
                         call.arguments.front().position, call.name + " 提示");
        }
        return call.name == "input" ? value_type::str_type
                                    : value_type::any_type;
    }
    if (call.arguments.size() != 1)
    {
        throw compile_error(item.position, call.name + " 需要一个参数");
    }
    auto& argument = *call.arguments.front().value;
    const auto actual = check_expression(argument);
    if (call.name == "deep_copy")
    {
        if (actual == value_type::void_type)
        {
            throw compile_error(argument.position, "deep_copy 不能复制 void 值");
        }
        return actual;
    }
    if (call.name == "len")
    {
        if (actual != value_type::array_type &&
            actual != value_type::dict_type &&
            actual != value_type::str_type &&
            actual != value_type::bytes_type &&
            actual != value_type::any_type && !actual.is_vector() &&
            !actual.is_typed_container())
        {
            throw compile_error(argument.position, "len 需要容器或字符串");
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

std::vector<value_type> semantic_analyzer::check_call_arguments(call_expression& call)
{
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
    return types;
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
    const auto types = check_call_arguments(call);
    if (!matches_signature(signature, call, types, false))
    {
        throw compile_error(item.position, "结构体构造参数不匹配：" + call.source_name);
    }
    call.is_constructor = true;
    return value_type(call.name);
}

value_type semantic_analyzer::check_call(expression& item, call_expression& call)
{
    if (call.container_type)
    {
        return call.container_type->is_vector()
            ? check_vector_call(item, call, *call.container_type)
            : check_container_call(item, call, *call.container_type);
    }
    if (call.is_super_view)
    {
        const auto found = classes_.find(call.super_type);
        if (current_class_ == nullptr || found == classes_.end() ||
            found->second->is_interface ||
            class_distance(value_type(current_class_->name),
                           value_type(call.super_type)) ==
                std::numeric_limits<std::size_t>::max() ||
            current_class_->name == call.super_type)
        {
            throw compile_error(item.position,
                                "super(类型) 需要当前类的具体父类");
        }
        return value_type(call.super_type);
    }
    if (call.receiver)
    {
        return check_method_call(item, call);
    }
    const auto& display_name = call.source_name.empty()
        ? call.name : call.source_name;
    if (auto* callback = find_symbol(display_name);
        callback != nullptr &&
        (callback->type.is_function() || callback->type.is_inferred_function()))
    {
        if (callback->type.is_inferred_function())
        {
            if (!call.expected_result)
            {
                throw compile_error(item.position,
                    "fn 的返回类型无法从当前调用推断，请写出完整签名：" +
                    display_name);
            }
            std::vector<value_type> arguments;
            for (auto& argument : call.arguments)
            {
                if (argument.kind != argument_kind::positional)
                {
                    throw compile_error(argument.position,
                        "fn 签名推断只接受位置实参");
                }
                const auto type = check_expression(*argument.value);
                if (type == value_type::void_type || type.is_inferred_function())
                {
                    throw compile_error(argument.position,
                        "fn 的参数类型无法确定");
                }
                arguments.push_back(type);
            }
            callback->type = value_type::function_of(std::move(arguments),
                                                      *call.expected_result);
        }
        const auto& signature = callback->type.parameters;
        if (call.arguments.size() + 1 != signature.size())
        {
            throw compile_error(item.position, "函数值调用的参数个数不匹配：" +
                                               display_name);
        }
        for (std::size_t index = 0; index < call.arguments.size(); ++index)
        {
            if (call.arguments[index].kind != argument_kind::positional ||
                check_expression(*call.arguments[index].value) != signature[index])
            {
                throw compile_error(call.arguments[index].position,
                                    "函数值调用需要位置实参与精确类型：" + display_name);
            }
        }
        call.indirect = true;
        return signature.back();
    }
    if (call.name == "print" || call.name == "len" ||
        call.name == "to_float" || call.name == "input" ||
        call.name == "input_or_none" ||
        call.name == "is_none" || call.name == "deep_copy")
    {
        return check_builtin(item, call);
    }
    if (structs_.contains(call.name))
    {
        return check_constructor(item, call);
    }
    if (classes_.contains(call.name))
    {
        return check_class_constructor(item, call);
    }
    const auto found = functions_.find(call.name);
    if (found == functions_.end())
    {
        throw compile_error(item.position, "未定义函数：" + display_name);
    }
    const auto actual_types = check_call_arguments(call);
    std::vector<std::size_t> candidates;
    for (std::size_t index = 0; index < found->second.size(); ++index)
    {
        const auto& signature = found->second[index];
        if (matches_signature(signature, call, actual_types, true))
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
        const bool has_spread = std::any_of(call.arguments.begin(),
            call.arguments.end(), [](const call_argument& argument)
            {
                return argument.kind == argument_kind::spread_array ||
                       argument.kind == argument_kind::spread_dict;
            });
        if (!has_spread)
        {
            std::size_t best_cost = std::numeric_limits<std::size_t>::max();
            std::optional<std::size_t> best;
            for (const auto candidate : candidates)
            {
                const auto& parameters = found->second[candidate].parameters;
                std::size_t positional = 0;
                std::size_t cost = 0;
                for (std::size_t i = 0; i < call.arguments.size(); ++i)
                {
                    const auto& argument = call.arguments[i];
                    const parameter* target = nullptr;
                    if (argument.kind == argument_kind::positional)
                    {
                        if (positional < parameters.size() &&
                            parameters[positional].kind == parameter_kind::ordinary)
                        {
                            target = &parameters[positional];
                        }
                        ++positional;
                    }
                    else if (argument.kind == argument_kind::keyword)
                    {
                        for (const auto& parameter : parameters)
                        {
                            if (parameter.kind == parameter_kind::ordinary &&
                                parameter.name == argument.name)
                            {
                                target = &parameter;
                                break;
                            }
                        }
                    }
                    if (target != nullptr && target->type != actual_types[i])
                    {
                        cost += class_distance(actual_types[i], target->type);
                    }
                }
                if (cost < best_cost)
                {
                    best_cost = cost;
                    best = candidate;
                }
                else if (cost == best_cost)
                {
                    best.reset();
                }
            }
            if (best)
            {
                call.overload_index = *best;
                return found->second[*best].result;
            }
        }
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
