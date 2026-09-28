#include "backend/llvm/codegen.hpp"

namespace tx
{
namespace
{

const char* result_suffix(const value_type& type)
{
    if (type == value_type::void_type)
    {
        return "void";
    }
    if (type == value_type::int_type)
    {
        return "i64";
    }
    if (type == value_type::float_type)
    {
        return "f64";
    }
    if (type == value_type::bool_type)
    {
        return "bool";
    }
    return type == value_type::str_type ? "str" : "value";
}

int result_kind(const value_type& type)
{
    if (type == value_type::void_type)
    {
        return 0;
    }
    if (type == value_type::int_type)
    {
        return 1;
    }
    if (type == value_type::float_type)
    {
        return 2;
    }
    if (type == value_type::bool_type)
    {
        return 3;
    }
    return type == value_type::str_type ? 4 : 5;
}

} // namespace

llvm_code_generator::ir_value llvm_code_generator::emit_thread_intrinsic(
    const expression& item, const function_decl& target,
    const std::vector<ir_value>& arguments)
{
    const auto operation = target.external_name.substr(7);
    const auto& argument = arguments.front();
    const bool returns_value = item.type != value_type::void_type;
    const auto output = returns_value ? allocate(item.type, item.position) : "";
    std::string symbol = "txrt_thread_" + operation;
    std::string parameters = "ptr " + argument.text;
    if (operation == "spawn")
    {
        parameters += ", i64 " + std::to_string(
            result_kind(argument.type.parameters.back()));
    }
    if (operation == "join")
    {
        symbol += "_" + std::string(result_suffix(item.type));
    }
    if (returns_value)
    {
        parameters += ", ptr " + output;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + symbol + "(" +
                      parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(argument);
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    if (item.type == value_type::str_type || is_value_handle(item.type))
    {
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + output);
        return {item.type, result};
    }
    return load({item.type, output});
}

} // namespace tx
