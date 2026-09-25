#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

bool llvm_code_generator::gc_neutral_binding(const operator_binding& binding)
{
    if (binding.virtual_slot)
    {
        bool found_target = false;
        for (const auto& [name, definition] : classes_)
        {
            (void)name;
            if (*binding.virtual_slot >= definition->virtual_targets.size())
            {
                continue;
            }
            const auto& target = definition->virtual_targets[*binding.virtual_slot];
            if (target.symbol.empty())
            {
                continue;
            }
            found_target = true;
            const auto found = functions_.find(target.symbol);
            if (found == functions_.end() ||
                target.overload_index >= found->second.size() ||
                !gc_neutral_function(*found->second[target.overload_index]))
            {
                return false;
            }
        }
        return found_target;
    }
    const auto found = functions_.find(binding.symbol);
    return found != functions_.end() &&
        binding.overload_index < found->second.size() &&
        gc_neutral_function(*found->second[binding.overload_index]);
}

bool llvm_code_generator::gc_neutral_expression(const expression& item)
{
    if (std::holds_alternative<integer_literal>(item.data) ||
        std::holds_alternative<floating_literal>(item.data) ||
        std::holds_alternative<boolean_literal>(item.data) ||
        std::holds_alternative<string_literal>(item.data) ||
        std::holds_alternative<none_literal>(item.data) ||
        std::holds_alternative<name_reference>(item.data))
    {
        return true;
    }
    if (const auto* member = std::get_if<member_expression>(&item.data))
    {
        return gc_neutral_expression(*member->object);
    }
    if (const auto* index = std::get_if<index_expression>(&item.data))
    {
        return gc_neutral_expression(*index->object) &&
               gc_neutral_expression(*index->index);
    }
    if (const auto* cast = std::get_if<cast_expression>(&item.data))
    {
        return gc_neutral_expression(*cast->value);
    }
    if (const auto* unary = std::get_if<unary_operation>(&item.data))
    {
        return gc_neutral_expression(*unary->operand) &&
               (!unary->binding || gc_neutral_binding(*unary->binding));
    }
    if (const auto* update = std::get_if<update_expression>(&item.data))
    {
        return gc_neutral_expression(*update->target);
    }
    if (const auto* binary = std::get_if<binary_operation>(&item.data))
    {
        return gc_neutral_expression(*binary->left) &&
               gc_neutral_expression(*binary->right) &&
               (!binary->binding || gc_neutral_binding(*binary->binding));
    }
    const auto* call = std::get_if<call_expression>(&item.data);
    if (!call || call->is_constructor)
    {
        return false;
    }
    if (call->is_super_view)
    {
        return true;
    }
    if (call->receiver && !gc_neutral_expression(*call->receiver))
    {
        return false;
    }
    for (const auto& argument : call->arguments)
    {
        if (argument.kind == argument_kind::spread_array ||
            argument.kind == argument_kind::spread_dict ||
            !gc_neutral_expression(*argument.value))
        {
            return false;
        }
    }
    if (call->name == "len" || call->name == "is_none" ||
        call->name == "to_float")
    {
        return true;
    }
    if (!call->overload_index)
    {
        return false;
    }
    const auto found = functions_.find(call->name);
    if (found == functions_.end() ||
        *call->overload_index >= found->second.size())
    {
        return false;
    }
    const auto& target = *found->second[*call->overload_index];
    if (std::any_of(target.parameters.begin(), target.parameters.end(),
        [](const parameter& value)
        { return value.kind != parameter_kind::ordinary; }))
    {
        return false;
    }
    if (call->virtual_dispatch)
    {
        return gc_neutral_binding({call->name, *call->overload_index,
                                   call->virtual_slot});
    }
    return gc_neutral_function(target);
}

bool llvm_code_generator::gc_neutral_statement(const statement& item)
{
    if (const auto* declaration =
            std::get_if<variable_declaration>(&item.data))
    {
        return !declaration->array_length && declaration->initializer &&
               gc_neutral_expression(*declaration->initializer);
    }
    if (const auto* assignment =
            std::get_if<variable_assignment>(&item.data))
    {
        if (is_value_handle(assignment->target->type))
        {
            // 覆盖旧引用可能触发 deinit，进而创建需要循环回收的节点。
            return false;
        }
        return gc_neutral_expression(*assignment->target) &&
               gc_neutral_expression(*assignment->value) &&
               (!assignment->binding ||
                gc_neutral_binding(*assignment->binding));
    }
    if (const auto* unpack = std::get_if<unpack_assignment>(&item.data))
    {
        for (std::size_t index = 0; index < unpack->names.size(); ++index)
        {
            if (!unpack->declares[index] &&
                is_value_handle(find_variable(unpack->names[index],
                                              item.position).type))
            {
                return false;
            }
        }
        return gc_neutral_expression(*unpack->value);
    }
    if (const auto* branch = std::get_if<if_statement>(&item.data))
    {
        return gc_neutral_expression(*branch->condition);
    }
    if (const auto* loop = std::get_if<while_statement>(&item.data))
    {
        return gc_neutral_expression(*loop->condition);
    }
    if (const auto* loop = std::get_if<for_loop>(&item.data))
    {
        return gc_neutral_expression(*loop->first) &&
               gc_neutral_expression(*loop->last);
    }
    if (const auto* loop = std::get_if<for_each>(&item.data))
    {
        return gc_neutral_expression(*loop->values);
    }
    if (const auto* expression_only =
            std::get_if<expression_statement>(&item.data))
    {
        return !is_value_handle(expression_only->value->type) &&
               gc_neutral_expression(*expression_only->value);
    }
    return false;
}

bool llvm_code_generator::gc_neutral_body(const std::vector<stmt_ptr>& body)
{
    for (const auto& item : body)
    {
        if (const auto* declaration =
                std::get_if<variable_declaration>(&item->data))
        {
            if ((declaration->declared_type &&
                 is_value_handle(*declaration->declared_type)) ||
                (declaration->initializer &&
                 is_value_handle(declaration->initializer->type)) ||
                !gc_neutral_statement(*item))
            {
                return false;
            }
        }
        else if (const auto* result =
                std::get_if<return_statement>(&item->data))
        {
            if (result->value && !gc_neutral_expression(*result->value))
            {
                return false;
            }
        }
        else if (const auto* branch = std::get_if<if_statement>(&item->data))
        {
            if (!gc_neutral_expression(*branch->condition) ||
                !gc_neutral_body(branch->then_body) ||
                !gc_neutral_body(branch->else_body))
            {
                return false;
            }
        }
        else if (const auto* loop = std::get_if<while_statement>(&item->data))
        {
            if (!gc_neutral_expression(*loop->condition) ||
                !gc_neutral_body(loop->body))
            {
                return false;
            }
        }
        else if (const auto* loop = std::get_if<for_loop>(&item->data))
        {
            if (!gc_neutral_expression(*loop->first) ||
                !gc_neutral_expression(*loop->last) ||
                !gc_neutral_body(loop->body))
            {
                return false;
            }
        }
        else if (const auto* loop = std::get_if<for_each>(&item->data))
        {
            if (!gc_neutral_expression(*loop->values) ||
                !gc_neutral_body(loop->body))
            {
                return false;
            }
        }
        else if (!gc_neutral_statement(*item))
        {
            return false;
        }
    }
    return true;
}

bool llvm_code_generator::gc_neutral_function(const function_decl& function)
{
    if (function.external)
    {
        return false;
    }
    if (const auto found = gc_neutral_cache_.find(&function);
        found != gc_neutral_cache_.end())
    {
        return found->second;
    }
    if (!gc_visiting_.insert(&function).second)
    {
        return false;
    }
    const bool result = gc_neutral_body(function.body);
    gc_visiting_.erase(&function);
    gc_neutral_cache_.emplace(&function, result);
    return result;
}

} // namespace tx
