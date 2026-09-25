#include "backend/llvm/codegen.hpp"

namespace tx
{

std::string llvm_code_generator::cache_dict_reference(
    const std::string& handle, source_pos position)
{
    const auto address = allocate(value_type::dict_type, position);
    const auto reference = temporary();
    write_instruction(reference + " = call ptr @txrt_dict_ref(ptr " +
                      handle + ")");
    write_instruction("store ptr " + reference + ", ptr " + address);
    return address;
}

void llvm_code_generator::refresh_dict_reference(
    const variable_slot& variable, const std::string& handle)
{
    if (variable.dict_reference.empty())
    {
        return;
    }
    const auto reference = temporary();
    write_instruction(reference + " = call ptr @txrt_dict_ref(ptr " +
                      handle + ")");
    write_instruction("store ptr " + reference + ", ptr " +
                      variable.dict_reference);
}

std::string llvm_code_generator::load_dict_reference(
    const variable_slot& variable)
{
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + variable.dict_reference);
    return result;
}

llvm_code_generator::ir_value llvm_code_generator::cast_dict_element(
    const index_expression& index, const value_type& target,
    source_pos position)
{
    const auto& name = std::get<name_reference>(index.object->data);
    const auto variable = find_variable(name.name, index.object->position);
    const auto dictionary = load_dict_reference(variable);
    const auto* literal = std::get_if<string_literal>(&index.index->data);
    ir_value key{value_type::void_type, {}};
    ir_value boxed{value_type::void_type, {}};
    std::string argument;
    std::string suffix;
    if (literal)
    {
        const auto decoded = decode_string_literal(literal->text);
        argument = "ptr " + global_bytes(decoded) + ", i64 " +
                   std::to_string(decoded.size());
        suffix = "_literal";
    }
    else
    {
        key = expression_value(*index.index);
        if (key.type == value_type::str_type)
        {
            argument = "ptr " + key.text;
            suffix = "_str";
        }
        else
        {
            boxed = box_any(key, index.index->position);
            argument = "ptr " + boxed.text;
        }
    }
    const auto* conversion = target == value_type::int_type
        ? "txrt_dict_ref_get_i64" : target == value_type::float_type
        ? "txrt_dict_ref_get_f64" : "txrt_dict_ref_get_str";
    const auto result = temporary();
    write_instruction(result + " = call " + llvm_type(target, position) +
                      " @" + conversion + suffix + "(ptr " + dictionary +
                      ", " + argument + ")");
    release(boxed);
    release(key);
    return {target, result};
}

bool llvm_code_generator::emit_direct_dict_assignment(
    const statement& item, const variable_assignment& assignment)
{
    const auto* index = std::get_if<index_expression>(&assignment.target->data);
    if (!index || index->object->type != value_type::dict_type ||
        index->index->type != value_type::str_type ||
        std::holds_alternative<string_literal>(index->index->data) ||
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
    const auto dictionary = lvalue_address(*index->object, indices);
    const auto key = indices.at(&*assignment.target);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_dict_set_" + suffix +
        "_str(ptr " + dictionary + ", ptr " + key.text + ", " +
        llvm_type(type, item.position) + " " + value.text + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(key);
    release(value);
    return true;
}

} // namespace tx
