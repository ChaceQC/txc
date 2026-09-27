#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::write_iterator_declarations()
{
    for (const auto* suffix : {"i64", "f64", "bool", "str", "bytes", "object"})
    {
        module_ << "declare i32 @txrt_iterator_new_" << suffix
                << "(ptr, ptr, i1, ptr)\n"
                << "declare i32 @txrt_iterator_next_" << suffix
                << "(ptr, ptr, ptr)\n";
    }
    module_ << "declare i32 @txrt_iterator_next_scalar_i64(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_iterator_next_scalar_f64(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_iterator_next_scalar_bool(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_iterator_close(ptr)\n";
}

llvm_code_generator::ir_value llvm_code_generator::emit_iterator_call(
    const expression& item, const call_expression& call)
{
    bool borrowed_receiver = false;
    const auto receiver = expression_value_or_borrow(*call.receiver,
                                                     borrowed_receiver);
    const auto& type = call.receiver->type;
    std::string symbol;
    std::string parameters = "ptr " + receiver.text;
    if (type.is_vector())
    {
        symbol = "txrt_iterator_new_" + vector_suffix(type);
        parameters += ", ptr " + global_bytes(type.parameters.front().name) +
            ", i1 " + (call.name == "snapshot_iter" ? "true" : "false");
    }
    else if (call.name == "next")
    {
        symbol = "txrt_iterator_next_" +
            vector_suffix(value_type::vector_of(type.parameters.front()));
        parameters += ", ptr " + global_bytes(item.type.name);
    }
    else
    {
        symbol = "txrt_iterator_close";
    }
    const bool returns_value = item.type != value_type::void_type;
    const auto output = returns_value ? allocate(item.type, item.position) : "";
    if (returns_value)
    {
        parameters += ", ptr " + output;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + symbol + "(" + parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    if (!borrowed_receiver)
    {
        release(receiver);
    }
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    return {item.type, result};
}

} // namespace tx
