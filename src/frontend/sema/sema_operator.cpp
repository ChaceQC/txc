#include "frontend/sema/sema.hpp"

#include <limits>
#include <string_view>
#include <vector>

namespace tx
{
namespace
{

std::string_view operator_text(token_kind kind)
{
    switch (kind)
    {
    case token_kind::plus: return "+";
    case token_kind::minus: return "-";
    case token_kind::star: return "*";
    case token_kind::slash: return "/";
    case token_kind::ampersand: return "&";
    case token_kind::caret: return "^";
    case token_kind::pipe: return "|";
    case token_kind::equal_equal: return "==";
    case token_kind::bang_equal: return "!=";
    case token_kind::less: return "<";
    case token_kind::less_equal: return "<=";
    case token_kind::greater: return ">";
    case token_kind::greater_equal: return ">=";
    case token_kind::bang: return "!";
    default: return "?";
    }
}

bool is_comparison(token_kind kind)
{
    return kind == token_kind::equal_equal ||
           kind == token_kind::bang_equal ||
           kind == token_kind::less ||
           kind == token_kind::less_equal ||
           kind == token_kind::greater ||
           kind == token_kind::greater_equal ||
           kind == token_kind::bang;
}

bool is_super_receiver(const expression& receiver)
{
    if (const auto* name = std::get_if<name_reference>(&receiver.data))
    {
        return name->name == "super";
    }
    if (const auto* call = std::get_if<call_expression>(&receiver.data))
    {
        return call->is_super_view;
    }
    return false;
}

} // namespace

void semantic_analyzer::validate_operator_method(
    const function_decl& method) const
{
    if (!method.operator_kind)
    {
        throw compile_error(method.position, "需要运算符成员声明");
    }
    const auto kind = *method.operator_kind;
    const auto count = method.parameters.size();
    const bool valid_count = kind == token_kind::bang ? count == 0
        : kind == token_kind::minus ? count <= 1 : count == 1;
    if (!valid_count)
    {
        throw compile_error(method.position,
                            "运算符参数数量不正确：" + method.source_name);
    }
    for (const auto& parameter : method.parameters)
    {
        if (parameter.kind != parameter_kind::ordinary)
        {
            throw compile_error(parameter.position,
                                "运算符不能使用可变参数");
        }
    }
    if (method.return_type == value_type::void_type ||
        (is_comparison(kind) && method.return_type != value_type::bool_type))
    {
        throw compile_error(method.position,
                            "运算符返回类型不正确：" + method.source_name);
    }
}

value_type semantic_analyzer::bind_operator(
    token_kind kind, const value_type& receiver,
    const std::optional<value_type>& argument,
    const expression& receiver_expression, source_pos position,
    std::optional<operator_binding>& binding) const
{
    const auto name = operator_method_name(kind);
    std::vector<const function_decl*> methods;
    const auto found_class = classes_.find(receiver.name);
    if (found_class != classes_.end())
    {
        methods = collect_methods(*found_class->second, name);
    }
    else if (const auto found_struct = structs_.find(receiver.name);
             found_struct != structs_.end())
    {
        for (const auto& method : found_struct->second->methods)
        {
            if (method.name == name)
            {
                methods.push_back(&method);
            }
        }
    }

    const function_decl* selected = nullptr;
    std::size_t best_cost = std::numeric_limits<std::size_t>::max();
    bool ambiguous = false;
    for (const auto* method : methods)
    {
        if (method->parameters.size() != static_cast<std::size_t>(argument.has_value()))
        {
            continue;
        }
        std::size_t cost = 0;
        if (argument)
        {
            const auto& expected = method->parameters.front().type;
            if (!is_assignable(*argument, expected))
            {
                continue;
            }
            if (*argument != expected)
            {
                cost = class_distance(*argument, expected);
            }
        }
        if (selected == nullptr || cost < best_cost)
        {
            selected = method;
            best_cost = cost;
            ambiguous = false;
        }
        else if (cost == best_cost)
        {
            ambiguous = true;
        }
    }
    if (selected == nullptr || ambiguous)
    {
        throw compile_error(position,
            std::string(ambiguous ? "运算符重载不唯一：" : "没有匹配的运算符重载：") +
            receiver.name + " " + std::string(operator_text(kind)) +
            (argument ? " " + argument->name : ""));
    }
    const bool super_call = is_super_receiver(receiver_expression);
    if (found_class != classes_.end())
    {
        check_access(selected->access, *classes_.at(selected->owner_class),
                     position, selected->source_name);
        if (super_call && selected->is_abstract)
        {
            throw compile_error(position,
                                "super 不能直接调用抽象运算符：" +
                                selected->source_name);
        }
    }
    operator_binding resolved;
    resolved.symbol = class_method_symbol(selected->owner_class, selected->name);
    resolved.overload_index = selected->overload_index;
    if (!super_call && !selected->virtual_slots.empty())
    {
        resolved.virtual_slot = selected->virtual_slots.front();
    }
    binding = std::move(resolved);
    return selected->return_type;
}

} // namespace tx
