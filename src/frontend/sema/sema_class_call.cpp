#include "frontend/sema/sema.hpp"

#include <algorithm>
#include <limits>

namespace tx
{

std::size_t semantic_analyzer::method_conversion_cost(
    const function_decl& method, const call_expression& call,
    const std::vector<value_type>& types) const
{
    std::size_t positional = 0;
    std::size_t cost = 0;
    for (std::size_t index = 0; index < call.arguments.size(); ++index)
    {
        const auto& argument = call.arguments[index];
        const parameter* target = nullptr;
        if (argument.kind == argument_kind::positional)
        {
            if (positional < method.parameters.size() &&
                method.parameters[positional].kind == parameter_kind::ordinary)
            {
                target = &method.parameters[positional];
            }
            ++positional;
        }
        else if (argument.kind == argument_kind::keyword)
        {
            for (const auto& parameter : method.parameters)
            {
                if (parameter.kind == parameter_kind::ordinary &&
                    parameter.name == argument.name)
                {
                    target = &parameter;
                    break;
                }
            }
        }
        if (target != nullptr && target->type != types[index])
        {
            cost += target->type == value_type::any_type
                ? 1000 : class_distance(types[index], target->type);
        }
    }
    return cost;
}

const function_decl* semantic_analyzer::select_method(
    const std::vector<const function_decl*>& candidates,
    const call_expression& call, const std::vector<value_type>& types,
    source_pos position, std::string_view description) const
{
    std::vector<const function_decl*> matches;
    for (const auto* method : candidates)
    {
        if (matches_signature({method->parameters, method->return_type},
                              call, types, true))
        {
            matches.push_back(method);
        }
    }
    if (matches.empty())
    {
        throw compile_error(position,
                            "没有匹配的" + std::string(description) + "重载：" +
                            call.source_name);
    }
    if (matches.size() == 1)
    {
        return matches.front();
    }
    const bool has_spread = std::any_of(call.arguments.begin(),
        call.arguments.end(), [](const call_argument& argument)
        {
            return argument.kind == argument_kind::spread_array ||
                   argument.kind == argument_kind::spread_dict;
        });
    if (has_spread)
    {
        throw compile_error(position,
                            std::string(description) + "重载不唯一：" +
                            call.source_name);
    }
    std::size_t best_cost = std::numeric_limits<std::size_t>::max();
    const function_decl* best = nullptr;
    for (const auto* method : matches)
    {
        const auto cost = method_conversion_cost(*method, call, types);
        if (cost < best_cost)
        {
            best_cost = cost;
            best = method;
        }
        else if (cost == best_cost)
        {
            best = nullptr;
        }
    }
    if (best == nullptr)
    {
        throw compile_error(position,
                            std::string(description) + "重载不唯一：" +
                            call.source_name);
    }
    return best;
}

value_type semantic_analyzer::check_class_constructor(
    expression& item, call_expression& call)
{
    const auto& definition = *classes_.at(call.name);
    if (definition.is_interface || definition.is_abstract)
    {
        throw compile_error(item.position,
                            "不能构造接口或抽象类：" + call.source_name);
    }
    const auto types = check_call_arguments(call);
    std::vector<const function_decl*> candidates;
    for (const auto& method : definition.methods)
    {
        if (method.name == "init")
        {
            candidates.push_back(&method);
        }
    }
    if (candidates.empty())
    {
        bool inherited_init = false;
        for (const auto& base : definition.bases)
        {
            const auto inherited = collect_methods(*classes_.at(base), "init");
            inherited_init = inherited_init || !inherited.empty();
            for (const auto* method : inherited)
            {
                if (matches_signature({method->parameters, method->return_type},
                                      call, types, true))
                {
                    candidates.push_back(method);
                }
            }
            if (!candidates.empty())
            {
                break;
            }
        }
        if (candidates.empty() && inherited_init)
        {
            throw compile_error(item.position,
                                "类构造参数不匹配：" + call.source_name);
        }
    }
    if (candidates.empty())
    {
        if (!call.arguments.empty())
        {
            throw compile_error(item.position,
                                "类构造参数不匹配：" + call.source_name);
        }
    }
    else
    {
        const auto* init = select_method(candidates, call, types,
                                         item.position, "构造方法");
        call.constructor_init_symbol =
            class_method_symbol(init->owner_class, init->name);
        call.constructor_init_index = init->overload_index;
    }
    call.is_constructor = true;
    return value_type(call.name);
}

value_type semantic_analyzer::check_method_call(
    expression& item, call_expression& call)
{
    const auto receiver_type = check_expression(*call.receiver);
    if (receiver_type.is_sum_type())
    {
        return check_sum_call(item, call, receiver_type);
    }
    if (receiver_type.is_typed_container())
    {
        return check_container_call(item, call, receiver_type);
    }
    if (receiver_type.is_vector())
    {
        return check_vector_call(item, call, receiver_type);
    }
    if (receiver_type.is_iterator())
    {
        return check_iterator_call(item, call, receiver_type);
    }
    const auto found = classes_.find(receiver_type.name);
    if (found == classes_.end())
    {
        throw compile_error(item.position, "方法调用需要 class 或 interface 对象");
    }
    if (call.name == "deinit")
    {
        throw compile_error(item.position, "deinit 由运行时自动调用");
    }
    const auto methods = collect_methods(*found->second, call.name);
    const auto types = check_call_arguments(call);
    const auto* method = select_method(methods, call, types,
                                       item.position, "方法");
    check_access(method->access, *classes_.at(method->owner_class),
                 item.position, method->name);
    const auto* receiver_name = std::get_if<name_reference>(&call.receiver->data);
    const auto* receiver_call =
        std::get_if<call_expression>(&call.receiver->data);
    const bool super_call =
        (receiver_name != nullptr && receiver_name->name == "super") ||
        (receiver_call != nullptr && receiver_call->is_super_view);
    if (super_call && method->is_abstract)
    {
        throw compile_error(item.position,
                            "super 不能直接调用抽象方法：" + method->name);
    }
    call.virtual_dispatch = !super_call && !method->virtual_slots.empty();
    if (call.virtual_dispatch)
    {
        call.virtual_slot = method->virtual_slots.front();
    }
    call.name = class_method_symbol(method->owner_class, method->name);
    call.overload_index = method->overload_index;
    return method->return_type;
}

} // namespace tx
