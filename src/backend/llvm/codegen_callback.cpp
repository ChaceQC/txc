#include "backend/llvm/codegen.hpp"

namespace tx
{
namespace
{

const char* box_symbol(const value_type& type)
{
    if (type == value_type::int_type)
    {
        return "txrt_value_box_i64";
    }
    if (type == value_type::float_type)
    {
        return "txrt_value_box_f64";
    }
    if (type == value_type::bool_type)
    {
        return "txrt_value_box_bool";
    }
    if (type == value_type::str_type)
    {
        return "txrt_value_box_str";
    }
    return "txrt_value_clone";
}

std::string error_return(std::string_view result_type)
{
    if (result_type == "void")
    {
        return "ret void";
    }
    if (result_type == "i64")
    {
        return "ret i64 0";
    }
    if (result_type == "i1")
    {
        return "ret i1 false";
    }
    if (result_type == "double")
    {
        return "ret double 0.0";
    }
    return "ret ptr null";
}

} // namespace

void llvm_code_generator::write_nullable_callback_boxes(
    const function_decl& function)
{
    for (std::size_t index = 0; index < function.parameters.size(); ++index)
    {
        if (parameter_is_nullable(function.parameters[index]))
        {
            module_ << "  %nullable_slot_" << index << " = alloca ptr\n"
                    << "  store ptr null, ptr %nullable_slot_" << index << '\n';
        }
    }
    for (std::size_t index = 0; index < function.parameters.size(); ++index)
    {
        const auto& parameter = function.parameters[index];
        if (!parameter_is_nullable(parameter))
        {
            continue;
        }
        module_ << "  %nullable_status_" << index << " = call i32 @"
                << box_symbol(parameter.type) << '('
                << llvm_type(parameter.type, parameter.position)
                << " %value" << index << ", ptr %nullable_slot_" << index
                << ")\n"
                << "  %nullable_failed_" << index << " = icmp ne i32 "
                << "%nullable_status_" << index << ", 0\n"
                << "  br i1 %nullable_failed_" << index
                << ", label %nullable_error, label %nullable_ready_" << index
                << "\nnullable_ready_" << index << ":\n"
                << "  %boxed_" << index << " = load ptr, ptr "
                << "%nullable_slot_" << index << '\n';
    }
}

void llvm_code_generator::write_callback_releases(
    const function_decl& function, bool call_completed)
{
    if (!call_completed)
    {
        for (std::size_t index = 0; index < function.parameters.size(); ++index)
        {
            if (parameter_is_nullable(function.parameters[index]))
            {
                module_ << "  %cleanup_" << index << " = load ptr, ptr "
                        << "%nullable_slot_" << index << '\n'
                        << "  call void @txrt_value_release(ptr %cleanup_"
                        << index << ")\n";
            }
        }
    }
    for (std::size_t index = 0; index < function.parameters.size(); ++index)
    {
        const auto& parameter = function.parameters[index];
        if (call_completed && !parameter_is_nullable(parameter) &&
            !ordinary_parameter_borrowed(function, index))
        {
            continue;
        }
        const auto* release = parameter.type == value_type::str_type
            ? "txrt_str_release" : is_value_handle(parameter.type)
            ? "txrt_value_release" : nullptr;
        if (release)
        {
            module_ << "  call void @" << release << "(ptr %value"
                    << index << ")\n";
        }
    }
}

void llvm_code_generator::emit_callback_wrapper(
    const function_decl& function, const std::string& symbol,
    std::size_t overload)
{
    std::string wrapper_parameters = "ptr %environment";
    std::string wrapper_arguments = "ptr %tx_context";
    bool nullable = false;
    for (std::size_t index = 0; index < function.parameters.size(); ++index)
    {
        const auto& parameter = function.parameters[index];
        const auto type = llvm_type(parameter.type, parameter.position);
        wrapper_parameters += ", ";
        wrapper_arguments += ", ";
        wrapper_parameters += type + " %value" + std::to_string(index);
        if (parameter_is_nullable(parameter))
        {
            nullable = true;
            wrapper_arguments += "ptr %boxed_" + std::to_string(index);
        }
        else
        {
            wrapper_arguments += type + " %value" + std::to_string(index);
        }
    }
    const auto result_type = llvm_type(function.return_type, function.position);
    module_ << "define " << result_type << ' '
            << callback_name(symbol, overload) << '(' << wrapper_parameters
            << ") {\nentry:\n";
    write_context_boundary();
    write_nullable_callback_boxes(function);
    if (function.return_type != value_type::void_type)
    {
        module_ << "  %result = ";
    }
    else
    {
        module_ << "  ";
    }
    module_ << "call " << result_type << ' ' << function_name(symbol, overload)
            << '(' << wrapper_arguments << ")\n";
    write_callback_releases(function, true);
    module_ << "  %pending = load i32, ptr %tx_error_kind\n"
            << "  call void @txrt_require_success(i32 %pending)\n"
            << (function.return_type == value_type::void_type
                ? "  ret void\n" : "  ret " + result_type + " %result\n");
    if (nullable)
    {
        module_ << "nullable_error:\n";
        write_callback_releases(function, false);
        module_ << "  call void @txrt_require_success(i32 1)\n"
                << "  " << error_return(result_type) << '\n';
    }
    module_ << "}\n\n";
}

} // namespace tx
