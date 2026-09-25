#include "backend/llvm/codegen.hpp"

namespace tx
{

std::optional<llvm_code_generator::ir_value>
llvm_code_generator::emit_literal_string_compare(
    const binary_operation& operation)
{
    const auto kind = operation.operation;
    if (operation.binding ||
        (kind != token_kind::equal_equal && kind != token_kind::bang_equal) ||
        operation.left->type != value_type::str_type)
    {
        return std::nullopt;
    }
    const auto* left = std::get_if<string_literal>(&operation.left->data);
    const auto* right = std::get_if<string_literal>(&operation.right->data);
    if (left && right)
    {
        const bool equal = decode_string_literal(left->text) ==
                           decode_string_literal(right->text);
        return ir_value{value_type::bool_type,
            equal == (kind == token_kind::equal_equal) ? "true" : "false"};
    }
    const auto* literal = left ? left : right;
    if (!literal)
    {
        return std::nullopt;
    }

    // 字面量无副作用；另一侧可借用稳定的字符串变量。
    bool borrowed = false;
    const auto value = read_only_string_value(
        left ? *operation.right : *operation.left, borrowed);
    const auto decoded = decode_string_literal(literal->text);
    const auto compared = temporary();
    write_instruction(compared +
        " = call i1 @txrt_str_equals_literal(ptr " + value.text +
        ", ptr " + global_bytes(decoded) + ", i64 " +
        std::to_string(decoded.size()) + ")");
    if (!borrowed)
    {
        release(value);
    }
    if (kind == token_kind::equal_equal)
    {
        return ir_value{value_type::bool_type, compared};
    }
    const auto inverted = temporary();
    write_instruction(inverted + " = xor i1 " + compared + ", true");
    return ir_value{value_type::bool_type, inverted};
}

} // namespace tx
