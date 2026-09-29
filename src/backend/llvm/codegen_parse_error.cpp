#include "backend/llvm/codegen.hpp"

namespace tx
{

bool llvm_code_generator::emit_parse_error_declaration(
    const statement& item, const variable_declaration& declaration)
{
    const auto* field = declaration.initializer
        ? std::get_if<member_expression>(&declaration.initializer->data) : nullptr;
    const auto* name = field ? std::get_if<name_reference>(&field->object->data) : nullptr;
    if (!name || field->field != "error" || declaration.array_length ||
        !readonly_local_fields(declaration))
    {
        return false;
    }
    const auto source = find_variable(name->name, item.position);
    if (!source.parse_declaration || source.native_parse_error.empty() ||
        !readonly_local_fields(*source.parse_declaration, &declaration))
    {
        return false;
    }
    // 两个局部均不修改、暴露对象身份或传出引用，错误枚举足以表示此次观察。
    variable_slot slot{declaration.initializer->type, {}, true};
    slot.readonly_parse_error = allocate(value_type::int_type, item.position);
    const auto error = load({value_type::int_type, source.native_parse_error});
    write_instruction("store i64 " + error.text + ", ptr " + slot.readonly_parse_error);
    scopes_.back().emplace(declaration.name, std::move(slot));
    return true;
}

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_parse_error_field(
    const expression& item, bool length)
{
    const auto* field = std::get_if<member_expression>(&item.data);
    const auto* name = field ? std::get_if<name_reference>(&field->object->data) : nullptr;
    if (!name || item.type != value_type::str_type)
    {
        return std::nullopt;
    }
    const auto source = find_variable(name->name, item.position);
    if (source.readonly_parse_error.empty())
    {
        return std::nullopt;
    }
    const auto index = field->field == "kind" ? "0" : field->field == "code" ? "1" : "2";
    const auto error = load({value_type::int_type, source.readonly_parse_error});
    if (length)
    {
        const auto result = temporary();
        write_instruction(result + " = call i64 @txrt_parse_error_field_length(i64 " +
            error.text + ", i64 " + index + ")");
        return ir_value{value_type::int_type, result};
    }
    const auto output = allocate(value_type::str_type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_parse_error_field_context(ptr %tx_context, i64 " +
        error.text + ", i64 " + index + ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    return ir_value{value_type::str_type, result};
}

} // namespace tx
