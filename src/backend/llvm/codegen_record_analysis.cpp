#include "backend/llvm/codegen.hpp"
#include "common/slot_layout.hpp"

#include <algorithm>

namespace tx
{

bool llvm_code_generator::scalar_record_type(const value_type& type) const
{
    const auto found = structs_.find(type.name);
    return found != structs_.end() && static_record_type(type) &&
        found->second->fields.size() <= native_record_budget / slot_bytes && std::all_of(
            found->second->fields.begin(), found->second->fields.end(), [](const struct_field& field)
            {
                return field.type == value_type::int_type || field.type == value_type::float_type ||
                    field.type == value_type::bool_type;
            });
}

bool llvm_code_generator::static_function_body(const function_decl& function) const
{
    const auto found = static_function_cache_.find(&function);
    return found != static_function_cache_.end() && found->second;
}

bool llvm_code_generator::native_record_function(const function_decl& function) const
{
    const bool records = !function.owner_class.empty() || scalar_record_type(function.return_type) ||
        std::any_of(function.parameters.begin(), function.parameters.end(), [&](const parameter& parameter)
        {
            return scalar_record_type(parameter.type);
        });
    return records && static_function_body(function);
}

const function_decl* llvm_code_generator::native_record_target(const call_expression& call) const
{
    const auto found = functions_.find(call.name);
    if (!call.overload_index || call.indirect || call.virtual_dispatch || call.is_constructor ||
        found == functions_.end() || *call.overload_index >= found->second.size())
    {
        return nullptr;
    }
    const auto* target = found->second[*call.overload_index];
    return call.arguments.size() == target->parameters.size() &&
        std::all_of(call.arguments.begin(), call.arguments.end(), [](const call_argument& argument)
        {
            return argument.kind == argument_kind::positional;
        }) && native_record_function(*target) ? target : nullptr;
}

const function_decl* llvm_code_generator::native_record_target(const operator_binding& binding) const
{
    const auto found = functions_.find(binding.symbol);
    if (binding.virtual_slot || found == functions_.end() || binding.overload_index >= found->second.size())
    {
        return nullptr;
    }
    const auto* target = found->second[binding.overload_index];
    return native_record_function(*target) ? target : nullptr;
}

bool llvm_code_generator::native_record_expression(const expression& item) const
{
    if (!scalar_record_type(item.type))
    {
        return false;
    }
    if (const auto* binary = std::get_if<binary_operation>(&item.data))
    {
        return binary->binding && native_record_target(*binary->binding);
    }
    const auto* call = std::get_if<call_expression>(&item.data);
    if (!call)
    {
        return false;
    }
    return call->is_constructor ? call->arguments.size() == structs_.at(item.type.name)->fields.size() &&
        std::all_of(call->arguments.begin(), call->arguments.end(), [](const call_argument& argument)
        {
            return argument.kind == argument_kind::positional || argument.kind == argument_kind::keyword;
        }) : native_record_target(*call) != nullptr;
}

bool llvm_code_generator::native_record_gc_neutral(const expression& item)
{
    if (!scalar_record_type(item.type))
    {
        return gc_neutral_expression(item);
    }
    if (std::holds_alternative<name_reference>(item.data))
    {
        return true;
    }
    if (!native_record_expression(item))
    {
        return false;
    }
    if (const auto* binary = std::get_if<binary_operation>(&item.data))
    {
        return native_record_gc_neutral(*binary->left) && native_record_gc_neutral(*binary->right);
    }
    const auto& call = std::get<call_expression>(item.data);
    return (!call.receiver || native_record_gc_neutral(*call.receiver)) &&
        std::all_of(call.arguments.begin(), call.arguments.end(), [&](const call_argument& argument)
        {
            return native_record_gc_neutral(*argument.value);
        });
}

} // namespace tx
