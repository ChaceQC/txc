#include "backend/llvm/codegen.hpp"

namespace tx
{
namespace
{

std::string suffix(const value_type& type)
{
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

} // namespace

void llvm_code_generator::write_channel_declarations()
{
    for (const auto* kind : {"i64", "f64", "bool", "str", "value"})
    {
        const std::string scalar = std::string(kind) == "i64" ? "i64" :
            std::string(kind) == "f64" ? "double" :
            std::string(kind) == "bool" ? "i1" : "ptr";
        const std::string tail = "_" + std::string(kind);
        module_ << "declare i32 @txrt_channel_bounded" << tail
                << (scalar == "ptr" ? "(i64, ptr, ptr)\n" :
                    "(i64, ptr)\n")
                << "declare i32 @txrt_channel_send" << tail
                << "(ptr, " << scalar << ", i64, ptr, ptr)\n"
                << "declare i32 @txrt_channel_recv" << tail
                << "(ptr, i64, ptr, ptr, ptr)\n"
                << "declare i32 @txrt_channel_close" << tail
                << "(ptr, ptr)\n"
                << "declare i32 @txrt_channel_select" << tail
                << "(ptr, i64, ptr, ptr, ptr, ptr)\n";
    }
}

llvm_code_generator::ir_value llvm_code_generator::emit_channel_intrinsic(
    const expression& item, const function_decl& target,
    const std::vector<ir_value>& arguments)
{
    const auto operation = target.external_name.substr(8);
    const value_type element = operation == "bounded" ? item.type.parameters.front() :
        operation == "select"
            ? item.type.parameters.front()
            : arguments.front().type.parameters.front();
    const auto symbol = "txrt_channel_" + operation + "_" + suffix(element);
    std::string parameters;
    for (const auto& argument : arguments)
    {
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        parameters += llvm_type(argument.type, item.position) + " " +
            argument.text;
    }
    if (operation == "recv")
    {
        parameters += ", ptr " + global_bytes(item.type.name);
    }
    if (operation == "bounded" &&
        element != value_type::int_type &&
        element != value_type::float_type &&
        element != value_type::bool_type)
    {
        parameters += ", ptr " + global_bytes(item.type.name);
    }
    if (operation == "select")
    {
        parameters += ", ptr " + global_bytes(item.type.name) +
            ", ptr " + global_bytes("option<" + element.name + ">");
    }
    const auto output = allocate(item.type, item.position);
    parameters += ", ptr " + output;
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + symbol + "(" +
                      parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    for (const auto& argument : arguments)
    {
        release(argument);
    }
    if (is_value_handle(item.type))
    {
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + output);
        return {item.type, result};
    }
    return load({item.type, output});
}

} // namespace tx
