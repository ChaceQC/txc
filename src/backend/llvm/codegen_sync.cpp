#include "backend/llvm/codegen.hpp"

namespace tx
{
namespace
{

std::string suffix(const value_type& type)
{
    if (type == value_type::int_type)
    {
        return "i64";
    }
    if (type == value_type::float_type)
    {
        return "f64";
    }
    if (type == value_type::bool_type)
    {
        return "bool";
    }
    return type == value_type::str_type ? "str" : "value";
}

} // namespace

void llvm_code_generator::write_sync_declarations()
{
    for (const auto* kind : {"i64", "f64", "bool", "str", "value"})
    {
        const std::string scalar = std::string(kind) == "i64" ? "i64" :
            std::string(kind) == "f64" ? "double" :
            std::string(kind) == "bool" ? "i1" : "ptr";
        const std::string tail = "_" + std::string(kind);
        module_ << "declare i32 @txrt_sync_new_mutex" << tail << "("
                << scalar << (scalar == "ptr" ? ", ptr, ptr)\n" :
                    ", ptr)\n")
                << "declare i32 @txrt_sync_lock" << tail << "(ptr, ptr)\n"
                << "declare i32 @txrt_sync_guard_get" << tail << "(ptr, ptr)\n"
                << "declare i32 @txrt_sync_guard_set" << tail << "(ptr, "
                << scalar << ")\n"
                << "declare i32 @txrt_sync_guard_close" << tail << "(ptr)\n"
                << "declare i32 @txrt_sync_new_rw_lock" << tail << "("
                << scalar << (scalar == "ptr" ? ", ptr, ptr)\n" :
                    ", ptr)\n")
                << "declare i32 @txrt_sync_read_lock" << tail << "(ptr, ptr)\n"
                << "declare i32 @txrt_sync_write_lock" << tail << "(ptr, ptr)\n"
                << "declare i32 @txrt_sync_read_get" << tail << "(ptr, ptr)\n"
                << "declare i32 @txrt_sync_write_get" << tail << "(ptr, ptr)\n"
                << "declare i32 @txrt_sync_write_set" << tail << "(ptr, "
                << scalar << ")\n"
                << "declare i32 @txrt_sync_read_close" << tail << "(ptr)\n"
                << "declare i32 @txrt_sync_write_close" << tail << "(ptr)\n"
                << "declare i32 @txrt_sync_wait" << tail
                << "(ptr, ptr, i64, ptr, ptr)\n";
    }
    for (const auto* kind : {"i64", "bool"})
    {
        const std::string scalar = std::string(kind) == "i64" ? "i64" : "i1";
        const std::string tail = "_" + std::string(kind);
        module_ << "declare i32 @txrt_sync_new_atomic" << tail << "("
                << scalar << ", ptr)\n"
                << "declare i32 @txrt_sync_atomic_load" << tail
                << "(ptr, i64, ptr)\n"
                << "declare i32 @txrt_sync_atomic_store" << tail << "(ptr, "
                << scalar << ", i64)\n"
                << "declare i32 @txrt_sync_atomic_exchange" << tail
                << "(ptr, " << scalar << ", i64, ptr)\n"
                << "declare i32 @txrt_sync_atomic_compare_exchange" << tail
                << "(ptr, " << scalar << ", " << scalar << ", i64, ptr)\n";
    }
    module_ << "declare i32 @txrt_sync_atomic_fetch_add_i64(ptr, i64, i64, ptr)\n"
            << "declare i32 @txrt_sync_new_condition(ptr)\n"
            << "declare i32 @txrt_sync_notify_one(ptr)\n"
            << "declare i32 @txrt_sync_notify_all(ptr)\n"
            << "declare i32 @txrt_sync_new_semaphore(i64, i64, ptr)\n"
            << "declare i32 @txrt_sync_acquire(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_sync_release(ptr, i64)\n"
            << "declare i32 @txrt_sync_new_once(ptr)\n"
            << "declare i32 @txrt_sync_run_once(ptr, ptr)\n";
}

llvm_code_generator::ir_value llvm_code_generator::emit_sync_intrinsic(
    const expression& item, const function_decl& target,
    const std::vector<ir_value>& arguments)
{
    const auto operation = target.external_name.substr(5);
    const auto& typed = operation == "wait" ? arguments[1] : arguments.front();
    const auto element = typed.type.is_sync_value()
        ? typed.type.parameters.front() : typed.type;
    const auto symbol = "txrt_sync_" + operation + "_" + suffix(element);
    std::string parameters;
    for (const auto& argument : arguments)
    {
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        parameters += llvm_type(argument.type, item.position) + " " +
            argument.text;
    }
    if ((operation == "new_mutex" || operation == "new_rw_lock") &&
        element != value_type::int_type &&
        element != value_type::float_type &&
        element != value_type::bool_type)
    {
        parameters += ", ptr " + global_bytes(item.type.name);
    }
    const bool returns_value = item.type != value_type::void_type;
    const auto output = returns_value ? allocate(item.type, item.position) : "";
    if (returns_value)
    {
        parameters += ", ptr " + output;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + symbol + "(" +
                      parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    for (const auto& argument : arguments)
    {
        release(argument);
    }
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    if (item.type == value_type::str_type || is_value_handle(item.type))
    {
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + output);
        return {item.type, result};
    }
    return load({item.type, output});
}

} // namespace tx
