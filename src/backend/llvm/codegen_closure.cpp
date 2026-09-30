#include "backend/llvm/codegen.hpp"
#include <algorithm>

namespace tx
{
namespace
{

std::string failed_return(const std::string& type)
{
    if (type == "void")
    {
        return "ret void";
    }
    if (type == "i64")
    {
        return "ret i64 0";
    }
    if (type == "i1")
    {
        return "ret i1 false";
    }
    if (type == "double")
    {
        return "ret double 0.0";
    }
    return "ret ptr null";
}

const char* capture_reader(const value_type& type)
{
    if (type == value_type::str_type)
    {
        return "txrt_value_to_str";
    }
    return "txrt_value_clone";
}

const char* scalar_capture_reader(const value_type& type)
{
    if (type == value_type::int_type)
    {
        return "txrt_closure_capture_i64";
    }
    if (type == value_type::float_type)
    {
        return "txrt_closure_capture_f64";
    }
    if (type == value_type::bool_type)
    {
        return "txrt_closure_capture_bool";
    }
    return nullptr;
}

} // namespace

void llvm_code_generator::emit_bind_wrapper(const std::string& name,
    const value_type& parent_type, std::size_t captured)
{
    // 包装函数按静态签名解出捕获值，原函数仍直接接收类型明确的实参。
    const auto argument_count = parent_type.parameters.size() - 1;
    const auto result_type = llvm_type(parent_type.parameters.back(), {});
    module_ << "define " << result_type << " " << name
            << "(ptr %environment";
    for (std::size_t index = captured; index < argument_count; ++index)
    {
        module_ << ", " << llvm_type(parent_type.parameters[index], {})
                << " %argument_" << index;
    }
    module_ << ") {\nentry:\n";
    write_context_boundary();
    module_ << "  %parent_slot = alloca ptr\n"
            << "  store ptr null, ptr %parent_slot\n";
    for (std::size_t index = 0; index < captured; ++index)
    {
        const auto& type = parent_type.parameters[index];
        if (!scalar_capture_reader(type))
        {
            module_ << "  %raw_slot_" << index << " = alloca ptr\n"
                    << "  store ptr null, ptr %raw_slot_" << index << "\n";
        }
        module_ << "  %typed_slot_" << index << " = alloca "
                << llvm_type(type, {}) << '\n';
        if (type == value_type::str_type || is_value_handle(type))
        {
            module_ << "  store ptr null, ptr %typed_slot_" << index << '\n';
        }
    }
    module_ << "  %parent_status = call i32 @txrt_closure_parent_borrow(ptr "
            << "%environment, ptr %parent_slot)\n"
            << "  %parent_failed = icmp ne i32 %parent_status, 0\n"
            << "  br i1 %parent_failed, label %failed, label %capture_0\n";
    for (std::size_t index = 0; index < captured; ++index)
    {
        const auto& type = parent_type.parameters[index];
        if (const auto* reader = scalar_capture_reader(type))
        {
            module_ << "capture_" << index << ":\n"
                    << "  %capture_status_" << index << " = call i32 @"
                    << reader << "(ptr %environment, i64 " << index
                    << ", ptr %typed_slot_" << index << ")\n"
                    << "  %capture_failed_" << index << " = icmp ne i32 "
                    << "%capture_status_" << index << ", 0\n"
                    << "  br i1 %capture_failed_" << index
                    << ", label %failed, label %"
                    << (index + 1 == captured
                        ? "call_ready" : "capture_" + std::to_string(index + 1))
                    << '\n';
            continue;
        }
        module_ << "capture_" << index << ":\n"
                << "  %capture_status_" << index
                << " = call i32 @txrt_closure_capture(ptr %environment, i64 "
                << index << ", ptr %raw_slot_" << index << ")\n"
                << "  %capture_failed_" << index << " = icmp ne i32 "
                << "%capture_status_" << index << ", 0\n"
                << "  br i1 %capture_failed_" << index
                << ", label %failed, label %convert_" << index << "\n"
                << "convert_" << index << ":\n"
                << "  %raw_" << index << " = load ptr, ptr %raw_slot_"
                << index << '\n'
                << "  %convert_status_" << index << " = call i32 @"
                << capture_reader(type) << "(ptr %raw_" << index
                << ", ptr %typed_slot_" << index << ")\n"
                << "  %convert_failed_" << index << " = icmp ne i32 "
                << "%convert_status_" << index << ", 0\n"
                << "  br i1 %convert_failed_" << index
                << ", label %failed, label %captured_" << index << "\n"
                << "captured_" << index << ":\n"
                << "  call void @txrt_value_release(ptr %raw_" << index << ")\n"
                << "  store ptr null, ptr %raw_slot_" << index << "\n"
                << "  br label %" << (index + 1 == captured
                    ? "call_ready" : "capture_" + std::to_string(index + 1))
                << '\n';
    }
    module_ << "call_ready:\n"
            << "  %parent = load ptr, ptr %parent_slot\n"
            << "  %code = call ptr @txrt_closure_code(ptr %parent)\n";
    std::string arguments = "ptr %parent";
    for (std::size_t index = 0; index < captured; ++index)
    {
        const auto type = llvm_type(parent_type.parameters[index], {});
        module_ << "  %captured_value_" << index << " = load " << type
                << ", ptr %typed_slot_" << index << '\n';
        arguments += ", " + type + " %captured_value_" + std::to_string(index);
    }
    for (std::size_t index = captured; index < argument_count; ++index)
    {
        arguments += ", " + llvm_type(parent_type.parameters[index], {}) +
            " %argument_" + std::to_string(index);
    }
    module_ << "  ";
    if (result_type != "void")
    {
        module_ << "%result = ";
    }
    module_ << "call " << result_type << " %code(" << arguments << ")\n"
            << "  %pending = load i32, ptr %tx_error_kind\n"
            << "  call void @txrt_require_success(i32 %pending)\n"
            << (result_type == "void" ? "  ret void\n"
                : "  ret " + result_type + " %result\n")
            << "failed:\n";
    // 只有调用目标之前的失败会进入这里；此时所有传入句柄仍由包装函数持有。
    for (std::size_t index = 0; index < captured; ++index)
    {
        const auto& type = parent_type.parameters[index];
        if (!scalar_capture_reader(type))
        {
            module_ << "  %raw_cleanup_" << index
                    << " = load ptr, ptr %raw_slot_" << index << '\n'
                    << "  call void @txrt_value_release(ptr %raw_cleanup_"
                    << index << ")\n";
        }
        if (type == value_type::str_type || is_value_handle(type))
        {
            module_ << "  %typed_cleanup_" << index
                    << " = load ptr, ptr %typed_slot_" << index << '\n'
                    << "  call void @"
                    << (type == value_type::str_type
                        ? "txrt_str_release" : "txrt_value_release")
                    << "(ptr %typed_cleanup_" << index << ")\n";
        }
    }
    for (std::size_t index = captured; index < argument_count; ++index)
    {
        const auto& type = parent_type.parameters[index];
        if (type == value_type::str_type || is_value_handle(type))
        {
            module_ << "  call void @"
                    << (type == value_type::str_type
                        ? "txrt_str_release" : "txrt_value_release")
                    << "(ptr %argument_" << index << ")\n";
        }
    }
    module_ << "  call void @txrt_require_success(i32 1)\n"
            << "  " << failed_return(result_type) << "\n}\n\n";
}

llvm_code_generator::ir_value llvm_code_generator::emit_bind_call(
    const expression& item, const call_expression& call)
{
    const auto& parent_type = call.arguments.front().value->type;
    const auto captured_count = call.arguments.size() - 1;
    const auto wrapper = "@tx_bind_" + std::to_string(next_bind_++);
    const auto* parent_name = std::get_if<name_reference>(&call.arguments.front().value->data);
    const function_decl* direct_target = nullptr;
    if (parent_name && parent_name->function_value)
    {
        const auto* candidate = functions_.at(parent_name->function_symbol).front();
        if (!candidate->external && std::none_of(candidate->parameters.begin(),
            candidate->parameters.end(), [&](const parameter& value)
            {
                return parameter_is_nullable(value);
            }))
        {
            direct_target = candidate;
        }
    }
    emit_typed_bind_wrapper(wrapper, parent_type, captured_count, direct_target);

    const auto parent = expression_value(*call.arguments.front().value);
    std::vector<ir_value> captures;
    std::vector<ir_value> boxes;
    captures.reserve(captured_count);
    boxes.reserve(captured_count);
    for (std::size_t index = 1; index < call.arguments.size(); ++index)
    {
        const auto value = expression_value(*call.arguments[index].value);
        captures.push_back(value);
        boxes.push_back(value.type == value_type::str_type
            ? box_any(value, item.position)
            : ir_value{value_type::void_type, {}});
    }
    const auto array = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << array << " = alloca [" << captured_count
                 << " x i64]\n";
    std::string kinds;
    for (std::size_t index = 0; index < captured_count; ++index)
    {
        const auto address = temporary();
        write_instruction(address + " = getelementptr inbounds [" +
            std::to_string(captured_count) + " x i64], ptr " + array +
            ", i64 0, i64 " + std::to_string(index));
        const auto& box = boxes[index];
        const auto& value = captures[index];
        kinds += value.type == value_type::int_type ? '\1' :
            value.type == value_type::float_type ? '\2' :
            value.type == value_type::bool_type ? '\3' : '\0';
        write_instruction("store " + llvm_type(value.type, item.position) + " " +
            (box.type == value_type::void_type ? value.text : box.text) +
            ", ptr " + address);
    }
    const auto output = allocate(item.type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_closure_bind_internal(ptr " +
        parent.text + ", ptr " + wrapper + ", ptr " + wrapper + "_internal, ptr " +
        global_bytes(item.type.name) + ", ptr " + global_bytes(kinds) +
        ", ptr " + array + ", i64 " +
        std::to_string(captured_count) + ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    for (std::size_t index = 0; index < captured_count; ++index)
    {
        release(boxes[index]);
        release(captures[index]);
    }
    release(parent);
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    return {item.type, result};
}

} // namespace tx
