#include "backend/analysis/static_function.hpp"

#include <algorithm>
#include <unordered_set>

namespace tx
{
namespace
{

bool scalar(const value_type& type)
{
    return type == value_type::int_type || type == value_type::float_type ||
        type == value_type::bool_type;
}

class body_analysis
{
public:
    const static_function_rules& rules;
    std::unordered_set<std::string> names;

    bool value(const expression& item) const
    {
        if (std::holds_alternative<integer_literal>(item.data) ||
            std::holds_alternative<floating_literal>(item.data) ||
            std::holds_alternative<boolean_literal>(item.data))
        {
            return true;
        }
        if (const auto* name = std::get_if<name_reference>(&item.data))
        {
            return !name->function_value && names.contains(name->name) &&
                (scalar(item.type) || rules.record(item.type));
        }
        if (const auto* member = std::get_if<member_expression>(&item.data))
        {
            return scalar(item.type) && rules.record(member->object->type) &&
                std::holds_alternative<name_reference>(member->object->data) && value(*member->object);
        }
        if (const auto* binary = std::get_if<binary_operation>(&item.data))
        {
            return (!binary->binding || rules.operation(*binary->binding)) &&
                value(*binary->left) && value(*binary->right);
        }
        if (const auto* unary = std::get_if<unary_operation>(&item.data))
        {
            return !unary->binding && scalar(item.type) && value(*unary->operand);
        }
        if (const auto* cast = std::get_if<cast_expression>(&item.data))
        {
            return scalar(item.type) && scalar(cast->value->type) && value(*cast->value);
        }
        if (const auto* update = std::get_if<update_expression>(&item.data))
        {
            return scalar(update->target->type) &&
                std::holds_alternative<name_reference>(update->target->data) && value(*update->target);
        }
        if (const auto* call = std::get_if<call_expression>(&item.data))
        {
            return rules.call(*call) && (!call->receiver || value(*call->receiver)) &&
                std::all_of(call->arguments.begin(), call->arguments.end(), [&](const call_argument& argument)
                {
                    return value(*argument.value);
                });
        }
        return false;
    }

    bool body(const std::vector<stmt_ptr>& statements)
    {
        return std::all_of(statements.begin(), statements.end(), [&](const stmt_ptr& item)
        {
            return statement_safe(*item);
        });
    }

    bool statement_safe(const statement& item)
    {
        if (const auto* declaration = std::get_if<variable_declaration>(&item.data))
        {
            return !declaration->array_length && declaration->initializer &&
                scalar(declaration->initializer->type) && value(*declaration->initializer) &&
                names.insert(declaration->name).second;
        }
        if (const auto* assignment = std::get_if<variable_assignment>(&item.data))
        {
            // record 参数保持只读；修改标量参数只影响调用时传入的副本。
            return !assignment->binding && scalar(assignment->target->type) &&
                std::holds_alternative<name_reference>(assignment->target->data) &&
                value(*assignment->target) && value(*assignment->value);
        }
        if (const auto* branch = std::get_if<if_statement>(&item.data))
        {
            return value(*branch->condition) && body(branch->then_body) && body(branch->else_body);
        }
        if (const auto* loop = std::get_if<while_statement>(&item.data))
        {
            return value(*loop->condition) && body(loop->body);
        }
        if (const auto* loop = std::get_if<for_loop>(&item.data))
        {
            return value(*loop->first) && value(*loop->last) &&
                names.insert(loop->name).second && body(loop->body);
        }
        if (const auto* result = std::get_if<return_statement>(&item.data))
        {
            // 返回参数本身会保留结构体别名；不能把这种结果变成新的内联副本。
            return result->value && (scalar(result->value->type) ||
                !std::holds_alternative<name_reference>(result->value->data)) && value(*result->value);
        }
        if (const auto* evaluated = std::get_if<expression_statement>(&item.data))
        {
            return scalar(evaluated->value->type) && value(*evaluated->value);
        }
        return false;
    }
};

} // namespace

bool analyze_static_function(const function_decl& function, const static_function_rules& rules)
{
    const auto representable = [&](const value_type& type)
    {
        return scalar(type) || rules.record(type);
    };
    if (function.external || function.is_async || function.body.empty() ||
        (!function.owner_class.empty() && !rules.record(value_type(function.owner_class))) ||
        !representable(function.return_type))
    {
        return false;
    }
    body_analysis analysis{rules, {}};
    if (!function.owner_class.empty())
    {
        analysis.names.insert("self");
    }
    for (const auto& parameter : function.parameters)
    {
        if (parameter.kind != parameter_kind::ordinary || parameter_is_nullable(parameter) ||
            !representable(parameter.type))
        {
            return false;
        }
        analysis.names.insert(parameter.name);
    }
    return analysis.body(function.body);
}

} // namespace tx
