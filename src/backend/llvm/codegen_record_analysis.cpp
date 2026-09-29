#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <functional>

namespace tx
{

bool llvm_code_generator::scalar_record_type(const value_type& type) const
{
    const auto found = structs_.find(type.name);
    return found != structs_.end() && static_record_type(type) &&
        found->second->fields.size() <= 4 && std::all_of(
            found->second->fields.begin(), found->second->fields.end(), [](const struct_field& field)
            {
                return field.type == value_type::int_type || field.type == value_type::float_type ||
                    field.type == value_type::bool_type;
            });
}

bool llvm_code_generator::native_record_function(const function_decl& function) const
{
    const auto scalar = [](const value_type& type)
    {
        return type == value_type::int_type || type == value_type::float_type || type == value_type::bool_type;
    };
    if (function.external || function.is_async || function.body.size() != 1 ||
        (!function.owner_class.empty() && !scalar_record_type(value_type(function.owner_class))) ||
        (!scalar(function.return_type) && !scalar_record_type(function.return_type)) ||
        !std::all_of(function.parameters.begin(), function.parameters.end(), [&](const parameter& parameter)
        {
            return parameter.kind == parameter_kind::ordinary && !parameter_is_nullable(parameter) &&
                (scalar(parameter.type) || scalar_record_type(parameter.type));
        }))
    {
        return false;
    }
    const auto* result = std::get_if<return_statement>(&function.body.front()->data);
    if (!result || !result->value)
    {
        return false;
    }
    // 可证明的效果摘要：只读取标量/字段，不保存、捕获或修改参数；结构体结果必须新建。
    std::function<bool(const expression&)> pure = [&](const expression& item)
    {
        if (std::holds_alternative<integer_literal>(item.data) ||
            std::holds_alternative<floating_literal>(item.data) ||
            std::holds_alternative<boolean_literal>(item.data))
        {
            return true;
        }
        if (const auto* name = std::get_if<name_reference>(&item.data))
        {
            return !name->function_value && scalar(item.type);
        }
        if (const auto* member = std::get_if<member_expression>(&item.data))
        {
            return scalar(item.type) && scalar_record_type(member->object->type) &&
                std::holds_alternative<name_reference>(member->object->data);
        }
        if (const auto* binary = std::get_if<binary_operation>(&item.data))
        {
            return !binary->binding && pure(*binary->left) && pure(*binary->right);
        }
        if (const auto* unary = std::get_if<unary_operation>(&item.data))
        {
            return !unary->binding && pure(*unary->operand);
        }
        if (const auto* cast = std::get_if<cast_expression>(&item.data))
        {
            return scalar(item.type) && pure(*cast->value);
        }
        const auto* call = std::get_if<call_expression>(&item.data);
        return call && call->is_constructor && native_record_expression(item) &&
            std::all_of(call->arguments.begin(), call->arguments.end(), [&](const call_argument& argument)
            {
                return pure(*argument.value);
            });
    };
    const bool records = !function.owner_class.empty() || scalar_record_type(function.return_type) ||
        std::any_of(function.parameters.begin(), function.parameters.end(), [&](const parameter& parameter)
        {
            return scalar_record_type(parameter.type);
        });
    return records && pure(*result->value);
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
