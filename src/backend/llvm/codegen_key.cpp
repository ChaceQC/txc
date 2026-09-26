#include "backend/llvm/codegen.hpp"

namespace tx
{

std::string llvm_code_generator::key_hash_symbol(const value_type& type)
{
    return "@tx_key_hash_" + type.name;
}

std::string llvm_code_generator::key_equal_symbol(const value_type& type)
{
    return "@tx_key_equal_" + type.name;
}

std::string llvm_code_generator::key_less_symbol(const value_type& type)
{
    return "@tx_key_less_" + type.name;
}

void llvm_code_generator::emit_key_callbacks(const struct_decl& definition)
{
    const function_decl* hash = nullptr;
    const function_decl* equal = nullptr;
    const function_decl* less = nullptr;
    for (const auto& method : definition.methods)
    {
        if (method.name == "hash_key" && !method.external)
        {
            hash = &method;
        }
        if (method.operator_kind == token_kind::equal_equal &&
            method.parameters.size() == 1 &&
            method.parameters.front().type == value_type(definition.name) &&
            !method.external)
        {
            equal = &method;
        }
        if (method.operator_kind == token_kind::less &&
            method.parameters.size() == 1 &&
            method.parameters.front().type == value_type(definition.name) &&
            !method.external)
        {
            less = &method;
        }
    }
    const value_type key_type(definition.name);
    if (less)
    {
        const auto target = function_name(
            class_method_symbol(definition.name, less->name),
            less->overload_index);
        module_ << "define i32 " << key_less_symbol(key_type)
                << "(ptr %left, ptr %right, ptr %out) {\nentry:\n"
                << "  %slot = alloca ptr\n"
                << "  %copy_status = call i32 @txrt_value_clone(ptr %right, ptr %slot)\n"
                << "  %copy_failed = icmp ne i32 %copy_status, 0\n"
                << "  br i1 %copy_failed, label %failed, label %ready\n"
                << "ready:\n"
                << "  %copy = load ptr, ptr %slot\n"
                << "  %value = call i1 " << target
                << "(ptr %left, ptr %copy)\n";
        if (operator_parameter_borrowed(*less))
        {
            module_ << "  call void @txrt_value_release(ptr %copy)\n";
        }
        module_ << "  %status = call i32 @txrt_error_status()\n"
                << "  store i1 %value, ptr %out\n"
                << "  ret i32 %status\n"
                << "failed:\n"
                << "  ret i32 %copy_status\n}\n\n";
    }
    if (!hash || !equal)
    {
        return;
    }
    const auto hash_target = function_name(
        class_method_symbol(definition.name, hash->name), hash->overload_index);
    const auto equal_target = function_name(
        class_method_symbol(definition.name, equal->name), equal->overload_index);
    module_ << "define i32 " << key_hash_symbol(key_type)
            << "(ptr %key, ptr %out) {\nentry:\n"
            << "  %value = call i64 " << hash_target << "(ptr %key)\n"
            << "  %status = call i32 @txrt_error_status()\n"
            << "  store i64 %value, ptr %out\n"
            << "  ret i32 %status\n}\n\n";

    // 运算符方法可能取得实参所有权，因此先复制右侧，再按方法的借用约定释放。
    module_ << "define i32 " << key_equal_symbol(key_type)
            << "(ptr %left, ptr %right, ptr %out) {\nentry:\n"
            << "  %slot = alloca ptr\n"
            << "  %copy_status = call i32 @txrt_value_clone(ptr %right, ptr %slot)\n"
            << "  %copy_failed = icmp ne i32 %copy_status, 0\n"
            << "  br i1 %copy_failed, label %failed, label %ready\n"
            << "ready:\n"
            << "  %copy = load ptr, ptr %slot\n"
            << "  %value = call i1 " << equal_target
            << "(ptr %left, ptr %copy)\n";
    if (operator_parameter_borrowed(*equal))
    {
        module_ << "  call void @txrt_value_release(ptr %copy)\n";
    }
    module_ << "  %status = call i32 @txrt_error_status()\n"
            << "  store i1 %value, ptr %out\n"
            << "  ret i32 %status\n"
            << "failed:\n"
            << "  ret i32 %copy_status\n}\n\n";
}

} // namespace tx
