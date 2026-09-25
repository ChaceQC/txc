#include "backend/llvm/codegen.hpp"

namespace tx
{

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_string_concat(
    const expression& item, const binary_operation& operation)
{
    if (operation.binding || operation.operation != token_kind::plus ||
        operation.left->type != value_type::str_type)
    {
        return std::nullopt;
    }
    const auto* left_literal = std::get_if<string_literal>(&operation.left->data);
    const auto* right_literal = std::get_if<string_literal>(&operation.right->data);
    if (left_literal || right_literal)
    {
        const auto output = allocate(value_type::str_type, item.position);
        const auto status = temporary();
        if (left_literal && right_literal)
        {
            const auto text = decode_string_literal(left_literal->text) +
                              decode_string_literal(right_literal->text);
            write_instruction(status + " = call i32 @txrt_str_new(ptr " + global_bytes(text) +
                ", i64 " + std::to_string(text.size()) + ", ptr " + output + ")");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
        }
        else
        {
            bool borrowed = false;
            const auto value = read_only_string_value(
                left_literal ? *operation.right : *operation.left, borrowed);
            const auto text = decode_string_literal(
                (left_literal ? left_literal : right_literal)->text);
            write_instruction(status + " = call i32 @txrt_str_concat_literal(ptr " +
                value.text + ", ptr " + global_bytes(text) + ", i64 " +
                std::to_string(text.size()) + ", i1 " + (left_literal ? "true" : "false") +
                ", ptr " + output + ")");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
            if (!borrowed)
            {
                release(value);
            }
        }
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + output);
        return ir_value{value_type::str_type, result};
    }
    bool left_borrowed = false;
    bool right_borrowed = false;
    // 后续表达式可执行用户代码时，保留左值的拥有型快照。
    const auto left = stable_value_expression(*operation.right)
        ? read_only_string_value(*operation.left, left_borrowed)
        : expression_value(*operation.left);
    const auto right = read_only_string_value(*operation.right, right_borrowed);
    const auto result = checked_binary("txrt_str_concat", left, right,
                                       value_type::str_type, item.position);
    if (!left_borrowed)
    {
        release(left);
    }
    if (!right_borrowed)
    {
        release(right);
    }
    return result;
}

} // namespace tx
