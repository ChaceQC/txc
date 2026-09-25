#include "backend/llvm/codegen.hpp"

namespace tx
{

std::string llvm_code_generator::vector_suffix(const value_type& type)
{
    const auto& element = type.parameters.front();
    return element == value_type::int_type ? "i64"
        : element == value_type::float_type ? "f64"
        : element == value_type::bool_type ? "bool" : "str";
}

bool llvm_code_generator::stable_value_expression(const expression& item)
{
    if (std::holds_alternative<name_reference>(item.data) ||
        std::holds_alternative<integer_literal>(item.data) ||
        std::holds_alternative<floating_literal>(item.data) ||
        std::holds_alternative<boolean_literal>(item.data) ||
        std::holds_alternative<string_literal>(item.data))
    {
        return true;
    }
    if (const auto* member = std::get_if<member_expression>(&item.data))
    {
        return stable_value_expression(*member->object);
    }
    if (const auto* index = std::get_if<index_expression>(&item.data))
    {
        return stable_value_expression(*index->object) &&
               stable_value_expression(*index->index);
    }
    if (const auto* binary = std::get_if<binary_operation>(&item.data))
    {
        return !binary->binding && stable_value_expression(*binary->left) &&
               stable_value_expression(*binary->right);
    }
    return false;
}

llvm_code_generator::ir_value llvm_code_generator::container_value(
    const expression& item, bool allow_borrow, bool& borrowed)
{
    borrowed = false;
    if (allow_borrow)
    {
        if (const auto* member = std::get_if<member_expression>(&item.data);
            member && std::holds_alternative<name_reference>(member->object->data))
        {
            lvalue_indices indices;
            borrowed = true;
            return {item.type, lvalue_address(item, indices)};
        }
        return expression_value_or_borrow(item, borrowed);
    }
    return expression_value(item);
}

llvm_code_generator::ir_value llvm_code_generator::vector_length(
    const ir_value& value, bool capacity)
{
    const auto view = temporary();
    write_instruction(view + " = call ptr @txrt_vector_ref_" + vector_suffix(value.type) +
                      "(ptr " + value.text + ")");
    const auto slot = temporary();
    write_instruction(slot + " = getelementptr { ptr, i64, i64 }, ptr " + view +
                      ", i32 0, i32 " + (capacity ? "2" : "1"));
    const auto result = temporary();
    write_instruction(result + " = load i64, ptr " + slot);
    return {value_type::int_type, result};
}

std::string llvm_code_generator::vector_slot(
    const ir_value& value, const ir_value& index, source_pos position)
{
    const auto view = temporary();
    write_instruction(view + " = call ptr @txrt_vector_ref_" + vector_suffix(value.type) +
                      "(ptr " + value.text + ")");
    const auto size_slot = temporary();
    write_instruction(size_slot + " = getelementptr { ptr, i64, i64 }, ptr " +
                      view + ", i32 0, i32 1");
    const auto size = temporary();
    write_instruction(size + " = load i64, ptr " + size_slot);
    const auto valid = temporary();
    // 长度非负；无符号比较同时排除负下标。
    write_instruction(valid + " = icmp ult i64 " + index.text + ", " + size);
    const auto ready = label();
    const auto error = label();
    write_instruction("br i1 " + valid + ", label %" + ready + ", label %" + error);
    start_block(error);
    write_instruction("call void @txrt_vector_index_error()");
    write_instruction("unreachable");
    start_block(ready);
    const auto data = temporary();
    write_instruction(data + " = load ptr, ptr " + view);
    const auto element = value.type.parameters.front();
    const auto storage_type = element == value_type::bool_type
        ? "i8" : llvm_type(element, position);
    const auto result = temporary();
    write_instruction(result + " = getelementptr " + storage_type + ", ptr " +
                      data + ", i64 " + index.text);
    return result;
}

llvm_code_generator::ir_value llvm_code_generator::vector_read(
    const ir_value& value, const ir_value& index, source_pos position)
{
    const auto slot = vector_slot(value, index, position);
    const auto& element = value.type.parameters.front();
    if (element == value_type::bool_type)
    {
        const auto byte = temporary();
        write_instruction(byte + " = load i8, ptr " + slot);
        const auto result = temporary();
        write_instruction(result + " = trunc i8 " + byte + " to i1");
        return {element, result};
    }
    return load({element, slot});
}

void llvm_code_generator::vector_write(const ir_value& value,
    const ir_value& index, const ir_value& element, source_pos position)
{
    const auto slot = vector_slot(value, index, position);
    if (element.type == value_type::str_type)
    {
        write_instruction("call void @txrt_vector_store_str(ptr " + slot +
                          ", ptr " + element.text + ")");
    }
    else if (element.type == value_type::bool_type)
    {
        const auto byte = temporary();
        write_instruction(byte + " = zext i1 " + element.text + " to i8");
        write_instruction("store i8 " + byte + ", ptr " + slot);
    }
    else
    {
        write_instruction("store " + llvm_type(element.type, position) + " " +
                          element.text + ", ptr " + slot);
    }
}

llvm_code_generator::ir_value llvm_code_generator::emit_vector_index(
    const index_expression& access, source_pos position)
{
    bool borrowed = false;
    const auto vector = container_value(*access.object,
        stable_value_expression(*access.index), borrowed);
    const auto index = expression_value(*access.index);
    const auto result = vector_read(vector, index, position);
    if (!borrowed)
    {
        release(vector);
    }
    return result;
}

} // namespace tx
