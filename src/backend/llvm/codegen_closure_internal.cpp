#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

void llvm_code_generator::emit_native_closure_adapter(
    const std::string& name, const value_type& type, bool bound)
{
    const auto result = llvm_type(type.parameters.back(), {});
    std::string parameters = "ptr %environment";
    std::string arguments = "ptr %tx_context, ptr " +
        std::string(bound ? "%view" : "null");
    for (std::size_t index = 0; index + 1 < type.parameters.size(); ++index)
    {
        const auto parameter = llvm_type(type.parameters[index], {}) +
            " %value" + std::to_string(index);
        parameters += ", " + parameter;
        arguments += ", " + parameter;
    }
    module_ << "define " << result << ' ' << name << '(' << parameters
            << ") {\nentry:\n";
    // 每次原生调用在实际执行线程取得上下文，闭包不保存创建线程的 context。
    write_context_boundary();
    if (bound)
    {
        module_ << "  %view = call ptr @txrt_closure_view(ptr %environment)\n";
    }
    module_ << "  " << (result == "void" ? "" : "%result = ") << "call "
            << result << ' ' << name << "_internal(" << arguments << ")\n"
            << (result == "void" ? "  ret void\n"
                : "  ret " + result + " %result\n") << "}\n\n";
}

void llvm_code_generator::cache_closure_local(variable_slot& slot,
    const variable_declaration& declaration, const std::string& value)
{
    if (!slot.type.is_function() || !immutable_format_local(declaration))
    {
        return;
    }
    slot.closure_view = temporary();
    write_instruction(slot.closure_view + " = call ptr @txrt_closure_view(ptr " + value + ")");
    const auto* name = std::get_if<name_reference>(&declaration.initializer->data);
    if (name && name->function_value)
    {
        slot.closure_target = callback_name(name->function_symbol, 0) + "_internal";
        slot.closure_function = functions_.at(name->function_symbol).front();
    }
    else
    {
        slot.closure_target = temporary();
        write_instruction(slot.closure_target + " = load ptr, ptr " + slot.closure_view);
        const auto* call = std::get_if<call_expression>(&declaration.initializer->data);
        if (call && call->name == "bind" && !call->arguments.empty())
        {
            const auto* parent = std::get_if<name_reference>(&call->arguments.front().value->data);
            const bool scalar = std::all_of(call->arguments.begin() + 1, call->arguments.end(),
                [](const call_argument& argument)
                {
                    const auto& type = argument.value->type;
                    return type == value_type::int_type || type == value_type::float_type ||
                        type == value_type::bool_type;
                });
            if (parent && parent->function_value && scalar)
            {
                slot.closure_function = functions_.at(parent->function_symbol).front();
            }
        }
    }
}

void llvm_code_generator::emit_typed_bind_wrapper(const std::string& name,
    const value_type& parent_type, std::size_t captured)
{
    const auto argument_count = parent_type.parameters.size() - 1;
    const auto result = llvm_type(parent_type.parameters.back(), {});
    module_ << "define " << result << ' ' << name
            << "_internal(ptr %tx_context, ptr %environment";
    for (std::size_t index = captured; index < argument_count; ++index)
    {
        module_ << ", " << llvm_type(parent_type.parameters[index], {})
                << " %argument_" << index;
    }
    module_ << ") {\nentry:\n"
            << "  %parent_slot = getelementptr inbounds %tx_closure_view, ptr %environment, i32 0, i32 1\n"
            << "  %parent = load ptr, ptr %parent_slot\n"
            << "  %code = load ptr, ptr %parent\n"
            << "  %capture_slot = getelementptr inbounds %tx_closure_view, ptr %environment, i32 0, i32 2\n"
            << "  %captures = load ptr, ptr %capture_slot\n";
    std::string arguments = "ptr %tx_context, ptr %parent";
    for (std::size_t index = 0; index < captured; ++index)
    {
        const auto& type = parent_type.parameters[index];
        const auto suffix = std::to_string(index);
        const auto native = llvm_type(type, {});
        module_ << "  %slot_" << index << " = getelementptr inbounds i64, ptr %captures, i64 " << index << '\n'
                << "  %capture_" << index << " = load " << native << ", ptr %slot_" << index << '\n';
        if (type == value_type::str_type || is_value_handle(type))
        {
            module_ << "  %owned_" << index << " = alloca ptr\n"
                    << "  store ptr null, ptr %owned_" << index << '\n';
        }
        arguments += ", " + native + " %value_" + suffix;
    }
    // 引用捕获借自不可变环境，目标按原约定接收独立的根句柄。
    for (std::size_t index = 0; index < captured; ++index)
    {
        const auto& type = parent_type.parameters[index];
        if (type == value_type::str_type || is_value_handle(type))
        {
            module_ << "  %status_" << index << " = call i32 @"
                    << (type == value_type::str_type ? "txrt_value_to_str" : "txrt_value_clone")
                    << "(ptr %capture_" << index << ", ptr %owned_" << index << ")\n"
                    << "  %failed_" << index << " = icmp ne i32 %status_" << index << ", 0\n"
                    << "  br i1 %failed_" << index << ", label %failed, label %ready_" << index
                    << "\nready_" << index << ":\n"
                    << "  %value_" << index << " = load ptr, ptr %owned_" << index << '\n';
        }
        else
        {
            const auto native = llvm_type(type, {});
            module_ << "  %value_" << index << " = select i1 true, " << native
                    << " %capture_" << index << ", " << native << " %capture_" << index << '\n';
        }
    }
    for (std::size_t index = captured; index < argument_count; ++index)
    {
        arguments += ", " + llvm_type(parent_type.parameters[index], {}) +
            " %argument_" + std::to_string(index);
    }
    module_ << "  " << (result == "void" ? "" : "%result = ") << "call "
            << result << " %code(" << arguments << ")\n"
            << (result == "void" ? "  ret void\n" : "  ret " + result + " %result\n");
    module_ << "failed:\n";
    for (std::size_t index = 0; index < argument_count; ++index)
    {
        const auto& type = parent_type.parameters[index];
        if (type != value_type::str_type && !is_value_handle(type))
        {
            continue;
        }
        if (index < captured)
        {
            module_ << "  %cleanup_" << index << " = load ptr, ptr %owned_" << index << '\n';
        }
        module_ << "  call void @" << (type == value_type::str_type
            ? "txrt_str_release" : "txrt_value_release") << "(ptr %"
            << (index < captured ? "cleanup_" : "argument_") << index << ")\n";
    }
    module_ << "  call void @txrt_require_success(i32 1)\n"
            << (result == "void" ? "  ret void\n" : "  ret " + result + " zeroinitializer\n")
            << "}\n\n";
    value_type bound_type;
    bound_type.parameters.assign(parent_type.parameters.begin() + captured,
                                  parent_type.parameters.end());
    emit_native_closure_adapter(name, bound_type, true);
}

} // namespace tx
