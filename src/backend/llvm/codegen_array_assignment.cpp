#include "backend/llvm/codegen.hpp"

namespace tx
{

namespace
{

bool simple_array_owner(const expression& item)
{
    if (std::holds_alternative<name_reference>(item.data))
    {
        return true;
    }
    const auto* member = std::get_if<member_expression>(&item.data);
    return member &&
        std::holds_alternative<name_reference>(member->object->data);
}

} // namespace

bool llvm_code_generator::emit_array_copy_assignment(
    const statement& item, const variable_assignment& assignment)
{
    const auto* target = std::get_if<index_expression>(&assignment.target->data);
    const auto* source = std::get_if<index_expression>(&assignment.value->data);
    if (!target || !source || assignment.binding ||
        assignment.operation != token_kind::equal ||
        assignment.value->type != value_type::any_type ||
        target->object->type != value_type::array_type ||
        source->object->type != value_type::array_type ||
        !simple_array_owner(*target->object) ||
        !simple_array_owner(*source->object) ||
        (!std::holds_alternative<name_reference>(source->index->data) &&
         !std::holds_alternative<integer_literal>(source->index->data)))
    {
        return false;
    }
    if (const auto* name = std::get_if<name_reference>(&source->object->data);
        name && has_local_array(find_variable(name->name, item.position)))
    {
        return false;
    }
    if (const auto* member = std::get_if<member_expression>(
            &source->object->data);
        member && classes_.contains(member->object->type.name))
    {
        // 类字段读必须先执行尚未初始化检查，通用读路径会保留该错误。
        return false;
    }

    lvalue_indices indices;
    prepare_lvalue_indices(*assignment.target, indices);
    const auto source_array = lvalue_address(*source->object, indices);
    const auto source_index = expression_value(*source->index);
    const auto element = temporary();
    write_instruction(element +
        " = call ptr @txrt_array_element_read_ptr(ptr " + source_array +
        ", i64 " + source_index.text + ")");
    const auto destination = lvalue_address(*target->object, indices);
    const auto destination_index = indices.at(&*assignment.target);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_array_set_value(ptr " +
        destination + ", i64 " + destination_index.text + ", ptr " +
        element + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(source_index);
    release(destination_index);
    return true;
}

bool llvm_code_generator::emit_direct_array_assignment(
    const statement& item, const variable_assignment& assignment)
{
    const auto* index = std::get_if<index_expression>(&assignment.target->data);
    if (!index || index->object->type != value_type::array_type ||
        assignment.binding || assignment.operation != token_kind::equal)
    {
        return false;
    }
    const auto type = assignment.value->type;
    const auto* suffix = type == value_type::int_type ? "i64"
        : type == value_type::float_type ? "f64"
        : type == value_type::bool_type ? "bool"
        : type == value_type::str_type ? "str" : nullptr;
    if (!suffix)
    {
        return false;
    }

    lvalue_indices indices;
    prepare_lvalue_indices(*assignment.target, indices);
    const auto value = expression_value(*assignment.value);
    bool native = false;
    std::string array;
    if (const auto* name = std::get_if<name_reference>(&index->object->data))
    {
        const auto variable = find_variable(name->name, item.position);
        native = !variable.array_reference.empty();
        if (native)
        {
            array = load_array_reference(variable);
        }
    }
    if (!native)
    {
        array = lvalue_address(*index->object, indices);
    }
    const auto position = indices.at(&*assignment.target);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_array_" +
        (native ? "ref_set_" : "set_") + suffix + "(ptr " + array +
        ", i64 " + position.text + ", " + llvm_type(type, item.position) +
        " " + value.text + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(position);
    release(value);
    return true;
}

} // namespace tx
