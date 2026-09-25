#include "backend/llvm/codegen.hpp"

namespace tx
{

bool llvm_code_generator::has_local_array(const variable_slot& array)
{
    return array.local_array_length || !array.dynamic_array_length.empty();
}

void llvm_code_generator::emit_dynamic_local_array_declaration(
    const statement& item, const variable_declaration& declaration)
{
    const auto length = expression_value(*declaration.array_length);
    const auto output = allocate(value_type::array_type, item.position);
    const auto status = temporary();
    write_instruction(status +
        " = call i32 @txrt_local_scalar_array_new(i64 " + length.text +
        ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto array = temporary();
    write_instruction(array + " = load ptr, ptr " + output);
    scopes_.back().emplace(declaration.name,
        variable_slot{value_type::array_type, array, false, {}, {}, {},
                      std::nullopt, length.text});
}

} // namespace tx
