#include "backend/llvm/codegen.hpp"

namespace tx
{

llvm_code_generator::ir_value llvm_code_generator::read_only_string_value(
    const expression& item, bool& borrowed)
{
    const auto* index = std::get_if<index_expression>(&item.data);
    if (index && index->object->type == value_type::vector_of(value_type::str_type) &&
        stable_value_expression(*index->index))
    {
        const auto* member = std::get_if<member_expression>(&index->object->data);
        const bool stable_owner = std::holds_alternative<name_reference>(index->object->data) ||
            (member && std::holds_alternative<name_reference>(member->object->data));
        if (stable_owner)
        {
            // 仅在已知无回调的只读字符串操作中使用；拥有者存活且后续无用户代码。
            bool owner_borrowed = false;
            const auto owner = container_value(*index->object, true, owner_borrowed);
            const auto at = expression_value(*index->index);
            const auto slot = vector_slot(owner, at, item.position);
            const auto result = temporary();
            write_instruction(result + " = load ptr, ptr " + slot);
            if (!owner_borrowed)
            {
                release(owner);
            }
            borrowed = true;
            return {value_type::str_type, result};
        }
    }
    return expression_value_or_borrow(item, borrowed);
}

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_string_key_query(
    const expression& item, const call_expression& call, const function_decl& target)
{
    if ((target.external_name != "dictionary.get" &&
         target.external_name != "dictionary.contains") || call.arguments.size() != 2)
    {
        return std::nullopt;
    }
    const auto* key = std::get_if<binary_operation>(&call.arguments[1].value->data);
    if (!key || key->binding || key->operation != token_kind::plus ||
        key->left->type != value_type::str_type ||
        !stable_value_expression(*key->left) || !stable_value_expression(*key->right))
    {
        return std::nullopt;
    }
    bool dict_borrowed = false;
    bool left_borrowed = false;
    bool right_borrowed = false;
    const auto dictionary = container_value(*call.arguments[0].value, true, dict_borrowed);
    const auto left = read_only_string_value(*key->left, left_borrowed);
    const auto right = read_only_string_value(*key->right, right_borrowed);
    const auto output = allocate(item.type, item.position);
    const auto status = temporary();
    const auto operation = target.external_name == "dictionary.get" ? "get" : "contains";
    write_instruction(status + " = call i32 @txrt_dictionary_" + operation +
        "_concat(ptr " + dictionary.text + ", ptr " + left.text + ", ptr " + right.text +
        ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    if (!dict_borrowed)
    {
        release(dictionary);
    }
    if (!left_borrowed)
    {
        release(left);
    }
    if (!right_borrowed)
    {
        release(right);
    }
    const auto result = temporary();
    write_instruction(result + " = load " + llvm_type(item.type, item.position) +
                      ", ptr " + output);
    return ir_value{item.type, result};
}

} // namespace tx
