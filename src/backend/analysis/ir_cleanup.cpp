#include "backend/analysis/ir_builder.hpp"

#include <algorithm>

namespace tx
{

bool ir_builder::inert_destruction(const value_type& type, std::set<std::string>& visiting) const
{
    if (!analysis_reference_type(type) || type == value_type::str_type || type == value_type::bytes_type)
    {
        return true;
    }
    // heap 即使保存标量，也可能拥有捕获对象的比较器闭包。
    if (type.container_name() == "heap")
    {
        return false;
    }
    if (!visiting.insert(type.name).second)
    {
        return false;
    }
    bool inert = false;
    if (type.is_vector() || type.is_typed_container() || type.is_sum_type() || type.is_priority_entry())
    {
        inert = std::all_of(type.parameters.begin(), type.parameters.end(), [&](const value_type& element)
        {
            return inert_destruction(element, visiting);
        });
    }
    else
    {
        const auto found = std::find_if(source_.structs.begin(), source_.structs.end(), [&](const struct_decl& record)
        {
            return record.name == type.name;
        });
        inert = found != source_.structs.end() && std::all_of(found->fields.begin(), found->fields.end(),
            [&](const struct_field& field)
            {
                return inert_destruction(field.type, visiting);
            });
    }
    visiting.erase(type.name);
    return inert;
}

void ir_builder::cleanup_scopes(std::size_t depth, bool may_fail)
{
    for (std::size_t index = scopes_.size(); index-- > depth;)
    {
        for (const auto& [name, variable] : scopes_[index])
        {
            (void)name;
            const auto& local = result_.variables[variable];
            if (!analysis_reference_type(local.type))
            {
                continue;
            }
            analysis_instruction cleanup;
            cleanup.operation = ir_operation::cleanup;
            cleanup.variable = variable;
            std::set<std::string> visiting;
            if (!inert_destruction(local.type, visiting))
            {
                cleanup.effects = {false, false, true, true, true, true};
                cleanup.target = target(class_method_symbol(local.type.name, "deinit"), 0);
                cleanup.unknown_target = !cleanup.target;
                cleanup.argument_parameters = {0};
            }
            const bool fails = may_fail && cleanup.effects.runs_user_code;
            append(std::move(cleanup), fails);
        }
    }
}

void ir_builder::finalize_parameter_cleanups()
{
    std::set<analysis_id> rebound;
    for (const auto& instruction : result_.instructions)
    {
        if (instruction.operation == ir_operation::bind_local)
        {
            rebound.insert(instruction.variable);
        }
    }
    for (auto& instruction : result_.instructions)
    {
        if (instruction.operation != ir_operation::cleanup)
        {
            continue;
        }
        const auto& local = result_.variables[instruction.variable];
        const auto& function = *result_.function;
        const auto offset = function.owner_class.empty() ? 0 : 1;
        bool borrowed = local.parameter_index == 0 && offset != 0;
        if (local.parameter_index != no_analysis_id && local.parameter_index >= static_cast<std::size_t>(offset))
        {
            const auto index = local.parameter_index - offset;
            const auto& parameter = function.parameters[index];
            borrowed = !rebound.contains(instruction.variable) && !parameter_is_nullable(parameter) &&
                parameter.kind == parameter_kind::ordinary && (function.owner_class.empty() ||
                ((function.name == "init" || function.operator_kind) && parameter.type != value_type::str_type));
        }
        if (borrowed)
        {
            instruction.target = nullptr;
            instruction.unknown_target = false;
            instruction.effects = {false, false, false, false, false, false};
        }
    }
}

} // namespace tx
