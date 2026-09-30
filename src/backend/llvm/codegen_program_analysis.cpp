#include "backend/llvm/codegen.hpp"
#include "backend/analysis/static_function.hpp"
#include "frontend/ast/call_properties.hpp"

#include <algorithm>

namespace tx
{

void llvm_code_generator::prepare_static_functions()
{
    for (const auto& [symbol, overloads] : functions_)
    {
        (void)symbol;
        for (const auto* function : overloads)
        {
            static_function_cache_[function] = !function->external;
        }
    }
    // 表示可用性从候选全集向下收敛，允许全由静态操作组成的递归 SCC。
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (const auto& [symbol, overloads] : functions_)
        {
            (void)symbol;
            for (const auto* function : overloads)
            {
                if (!static_function_cache_.at(function))
                {
                    continue;
                }
                // 当前函数的递归边读取暂定真值，不把访问中等同于失败。
                const auto target_safe = [&](const std::string& name, std::size_t overload)
                {
                    const auto found = functions_.find(name);
                    return found != functions_.end() && overload < found->second.size() &&
                        static_function_cache_.at(found->second[overload]);
                };
                const static_function_rules rules{
                    [this](const value_type& type)
                    {
                        return scalar_record_type(type);
                    },
                    [&](const call_expression& call)
                    {
                        if (call.indirect || call.virtual_dispatch)
                        {
                            return false;
                        }
                        if (call.is_constructor)
                        {
                            const auto found = structs_.find(call.name);
                            return found != structs_.end() && scalar_record_type(value_type(call.name)) &&
                                call.arguments.size() == found->second->fields.size();
                        }
                        const auto found = functions_.find(call.name);
                        return call.overload_index && found != functions_.end() &&
                            *call.overload_index < found->second.size() &&
                            call.arguments.size() == found->second[*call.overload_index]->parameters.size() &&
                            std::all_of(call.arguments.begin(), call.arguments.end(), [](const call_argument& argument)
                            {
                                return argument.kind == argument_kind::positional;
                            }) && target_safe(call.name, *call.overload_index);
                    },
                    [&](const operator_binding& binding)
                    {
                        return !binding.virtual_slot && target_safe(binding.symbol, binding.overload_index);
                    }};
                if (!analyze_static_function(*function, rules))
                {
                    static_function_cache_[function] = false;
                    changed = true;
                }
            }
        }
    }
}

std::optional<llvm_code_generator::ir_value> llvm_code_generator::inferred_move(
    const expression& item, const variable_slot& variable)
{
    if (!current_analysis_ || !current_analysis_->ir.move_candidates.contains(&item) ||
        variable.borrowed || !variable.stack_record.empty() || !variable.local_guard.empty() ||
        !variable.projected_fields.empty() || !variable.snapshot_kind.empty() || variable.local_array_length ||
        !variable.dynamic_array_length.empty() || !variable.native_option_value.empty() ||
        !variable.native_parse_ok.empty() || !variable.readonly_parse_error.empty())
    {
        return std::nullopt;
    }
    const bool inert = variable.type == value_type::str_type || variable.type == value_type::bytes_type ||
        scalar_container_type(variable.type);
    const bool returns_owner = std::any_of(current_analysis_->ir.instructions.begin(),
        current_analysis_->ir.instructions.end(), [&](const analysis_instruction& instruction)
        {
            return instruction.operation == ir_operation::return_value && !instruction.inputs.empty() &&
                current_analysis_->ir.instructions[instruction.inputs.front()].source == &item;
        });
    if (!inert && !returns_owner)
    {
        return std::nullopt;
    }
    const auto moved = temporary();
    write_instruction(moved + " = load ptr, ptr " + variable.address);
    if (recoverable_errors_)
    {
        const auto root = allocate(item.type, item.position);
        write_instruction("store ptr " + moved + ", ptr " + root);
    }
    write_instruction("store ptr null, ptr " + variable.address);
    // 从原槽清出后立即建立临时拥有根；后续实参求值失败也能清理转交值。
    return ir_value{item.type, moved};
}

} // namespace tx
