#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

bool llvm_code_generator::rebinds_name(const std::vector<stmt_ptr>& body,
                                       std::string_view name)
{
    for (const auto& item : body)
    {
        if (const auto* guarded = std::get_if<try_statement>(&item->data))
        {
            if (rebinds_name(guarded->body, name))
            {
                return true;
            }
            for (const auto& handler : guarded->handlers)
            {
                if (rebinds_name(handler.body, name))
                {
                    return true;
                }
            }
        }
        if (const auto* assignment =
                std::get_if<variable_assignment>(&item->data))
        {
            if (const auto* target =
                    std::get_if<name_reference>(&assignment->target->data);
                target && target->name == name)
            {
                return true;
            }
        }
        if (const auto* unpack = std::get_if<unpack_assignment>(&item->data))
        {
            for (std::size_t index = 0; index < unpack->names.size(); ++index)
            {
                // 首次解构声明不会覆盖已有根；把声明计为重绑定会立即
                // 装箱刚建立的标量快照，连只读局部数组也退回堆句柄。
                if (!unpack->declares[index] && unpack->names[index] == name)
                {
                    return true;
                }
            }
        }
        if (const auto* branch = std::get_if<if_statement>(&item->data))
        {
            if (rebinds_name(branch->then_body, name) ||
                rebinds_name(branch->else_body, name))
            {
                return true;
            }
        }
        if (const auto* loop = std::get_if<while_statement>(&item->data))
        {
            if (rebinds_name(loop->body, name))
            {
                return true;
            }
        }
        if (const auto* loop = std::get_if<for_loop>(&item->data))
        {
            if (rebinds_name(loop->body, name))
            {
                return true;
            }
        }
        if (const auto* loop = std::get_if<for_each>(&item->data))
        {
            if (rebinds_name(loop->body, name))
            {
                return true;
            }
        }
    }
    return false;
}

bool llvm_code_generator::operator_parameter_intrinsic(
    const function_decl& function)
{
    return function.operator_kind && function.parameters.size() == 1 &&
        is_value_handle(function.parameters[0].type) &&
        !rebinds_name(function.body, function.parameters[0].name);
}

bool llvm_code_generator::init_parameter_borrowed(
    const function_decl& function, std::size_t index)
{
    return function.name == "init" &&
        index < function.parameters.size() &&
        !parameter_is_nullable(function.parameters[index]) &&
        is_value_handle(function.parameters[index].type) &&
        !rebinds_name(function.body, function.parameters[index].name);
}

bool llvm_code_generator::ordinary_parameter_borrowed(
    const function_decl& function, std::size_t index)
{
    return function.owner_class.empty() &&
        index < function.parameters.size() &&
        !parameter_is_nullable(function.parameters[index]) &&
        function.parameters[index].kind == parameter_kind::ordinary &&
        (is_value_handle(function.parameters[index].type) ||
         function.parameters[index].type == value_type::str_type) &&
        !rebinds_name(function.body, function.parameters[index].name);
}

bool llvm_code_generator::operator_parameter_borrowed(
    const function_decl& function) const
{
    if (!operator_parameter_intrinsic(function))
    {
        return false;
    }
    for (const auto slot : function.virtual_slots)
    {
        for (const auto& [name, definition] : classes_)
        {
            (void)name;
            if (slot >= definition->virtual_targets.size())
            {
                continue;
            }
            const auto& target = definition->virtual_targets[slot];
            if (target.symbol.empty())
            {
                continue;
            }
            const auto found = functions_.find(target.symbol);
            if (found == functions_.end() ||
                target.overload_index >= found->second.size() ||
                !operator_parameter_intrinsic(
                    *found->second[target.overload_index]))
            {
                return false;
            }
        }
    }
    return true;
}

bool llvm_code_generator::operator_argument_borrowed(
    const operator_binding& binding) const
{
    const auto borrowed_target = [this](const std::string& symbol,
                                        std::size_t overload)
    {
        const auto found = functions_.find(symbol);
        return found != functions_.end() && overload < found->second.size() &&
            operator_parameter_borrowed(*found->second[overload]);
    };
    if (!binding.virtual_slot)
    {
        return borrowed_target(binding.symbol, binding.overload_index);
    }
    bool found_target = false;
    for (const auto& [name, definition] : classes_)
    {
        (void)name;
        if (*binding.virtual_slot >= definition->virtual_targets.size())
        {
            continue;
        }
        const auto& target = definition->virtual_targets[*binding.virtual_slot];
        if (!target.symbol.empty())
        {
            found_target = true;
            if (!borrowed_target(target.symbol, target.overload_index))
            {
                return false;
            }
        }
    }
    return found_target;
}

} // namespace tx
