#include "backend/llvm/codegen.hpp"

namespace tx
{

std::string llvm_code_generator::vector_reference(const ir_value& value)
{
    if (!value.vector_reference.empty())
    {
        return value.vector_reference;
    }
    const auto result = temporary();
    write_instruction(result + " = call ptr @txrt_vector_ref_" +
        vector_suffix(value.type) + "(ptr " + value.text + ")");
    return result;
}

void llvm_code_generator::cache_vector_reference(
    variable_slot& variable, const std::string& handle)
{
    if (!variable.type.is_vector())
    {
        return;
    }
    variable.vector_reference = allocate(variable.type, {}, false);
    refresh_vector_reference(variable, handle);
}

void llvm_code_generator::refresh_vector_reference(
    const variable_slot& variable, const std::string& handle)
{
    if (!variable.vector_reference.empty())
    {
        const auto reference = vector_reference({variable.type, handle});
        write_instruction("store ptr " + reference + ", ptr " + variable.vector_reference);
    }
}

std::string llvm_code_generator::load_vector_reference(const variable_slot& variable)
{
    if (variable.vector_reference.empty())
    {
        return {};
    }
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + variable.vector_reference);
    return result;
}

} // namespace tx
