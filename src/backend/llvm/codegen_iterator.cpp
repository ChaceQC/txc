#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::write_iterator_declarations()
{
    module_ << "%tx_iterator_cursor = type { ptr, i64, i64, i8, i8 }\n"
            << "declare ptr @txrt_iterator_snapshot_cursor(ptr)\n"
            << "declare i32 @txrt_iterator_closed_error()\n";
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

void llvm_code_generator::cache_iterator_cursor(variable_slot& slot,
    const variable_declaration& declaration, const std::string& handle)
{
    if (!slot.type.is_iterator() || !scalar_option_suffix(slot.type.parameters.front()))
    {
        return;
    }
    const auto* call = declaration.initializer
        ? std::get_if<call_expression>(&declaration.initializer->data) : nullptr;
    if (!call || !call->receiver || !call->receiver->type.is_vector() ||
        call->name != "snapshot_iter" || !confined_local(declaration, false))
    {
        return;
    }
    slot.iterator_cursor = allocate(value_type::any_type, {}, false);
    const auto cursor = temporary();
    write_instruction(cursor + " = call ptr @txrt_iterator_snapshot_cursor(ptr " + handle + ")");
    write_instruction("store ptr " + cursor + ", ptr " + slot.iterator_cursor);
}

bool llvm_code_generator::emit_cursor_next(const call_expression& call,
    const std::string& present, const std::string& scalar)
{
    const auto* name = std::get_if<name_reference>(&call.receiver->data);
    if (!name)
    {
        return false;
    }
    const auto slot = find_variable(name->name, call.receiver->position);
    if (slot.iterator_cursor.empty())
    {
        return false;
    }
    const auto cursor = temporary();
    write_instruction(cursor + " = load ptr, ptr " + slot.iterator_cursor);
    std::vector<std::string> fields;
    for (int index = 0; index < 5; ++index)
    {
        fields.push_back(temporary());
        write_instruction(fields.back() + " = getelementptr inbounds %tx_iterator_cursor, ptr " +
            cursor + ", i32 0, i32 " + std::to_string(index));
    }
    const auto closed = temporary();
    const auto is_closed = temporary();
    write_instruction(closed + " = load i8, ptr " + fields[4]);
    write_instruction(is_closed + " = icmp ne i8 " + closed + ", 0");
    const auto error = label();
    const auto check = label();
    write_instruction("br i1 " + is_closed + ", label %" + error + ", label %" + check);
    start_block(error);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_iterator_closed_error()");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    write_instruction("unreachable");
    start_block(check);
    const auto index = temporary();
    const auto size = temporary();
    const auto valid = temporary();
    write_instruction(index + " = load i64, ptr " + fields[2]);
    write_instruction(size + " = load i64, ptr " + fields[1]);
    // snapshot 长度固定，index == size 后永远读空，无需重复读取 exhausted。
    write_instruction(valid + " = icmp ult i64 " + index + ", " + size);
    write_instruction("store i1 " + valid + ", ptr " + present);
    const auto read = label();
    const auto empty = label();
    const auto ready = label();
    write_instruction("br i1 " + valid + ", label %" + read + ", label %" + empty);
    start_block(read);
    const auto data = temporary();
    const auto address = temporary();
    const auto value = temporary();
    const auto& element = slot.type.parameters.front();
    const auto type = llvm_type(element, {});
    const auto stored_type = element == value_type::bool_type ? "i8" : type;
    write_instruction(data + " = load ptr, ptr " + fields[0]);
    write_instruction(address + " = getelementptr inbounds " + stored_type + ", ptr " +
        data + ", i64 " + index);
    write_instruction(value + " = load " + stored_type + ", ptr " + address);
    auto result = value;
    if (element == value_type::bool_type)
    {
        result = temporary();
        write_instruction(result + " = icmp ne i8 " + value + ", 0");
    }
    write_instruction("store " + type + " " + result + ", ptr " + scalar);
    const auto next = temporary();
    write_instruction(next + " = add nuw i64 " + index + ", 1");
    write_instruction("store i64 " + next + ", ptr " + fields[2]);
    write_instruction("br label %" + ready);
    start_block(empty);
    write_instruction("store i8 1, ptr " + fields[3]);
    write_instruction("store " + type + " " +
        (element == value_type::float_type ? "0.0" : "0") + ", ptr " + scalar);
    write_instruction("br label %" + ready);
    start_block(ready);
    return true;
}

} // namespace tx
