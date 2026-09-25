#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

llvm_code_generator::ir_value llvm_code_generator::emit_vector_call(
    const expression& item, const call_expression& call)
{
    const auto& type = call.container_type ? *call.container_type : call.receiver->type;
    bool borrowed = false;
    ir_value vector{value_type::void_type, {}};
    if (call.receiver)
    {
        const bool stable = std::all_of(call.arguments.begin(), call.arguments.end(),
            [](const call_argument& arg)
            {
                return stable_value_expression(*arg.value);
            });
        vector = container_value(*call.receiver, stable, borrowed);
    }
    std::vector<ir_value> arguments;
    for (const auto& argument : call.arguments)
    {
        arguments.push_back(expression_value(*argument.value));
    }
    if (call.receiver && (call.name == "size" || call.name == "capacity" ||
                          call.name == "empty"))
    {
        const auto result = vector_length(vector, call.name == "capacity");
        if (!borrowed)
        {
            release(vector);
        }
        if (call.name == "empty")
        {
            const auto empty = temporary();
            write_instruction(empty + " = icmp eq i64 " + result.text + ", 0");
            return {value_type::bool_type, empty};
        }
        return result;
    }
    std::string operation = call.name;
    if (call.container_type)
    {
        operation = arguments.size() == 1 ? "from_array" : "new";
        if (arguments.empty())
        {
            const auto& element = type.parameters.front();
            arguments.push_back({value_type::int_type, "0"});
            arguments.push_back({element, element == value_type::str_type ? "null"
                : element == value_type::float_type ? "0.0"
                : element == value_type::bool_type ? "false" : "0"});
        }
    }
    std::string parameters = call.receiver ? "ptr " + vector.text : "";
    for (const auto& argument : arguments)
    {
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        parameters += llvm_type(argument.type, item.position) + " " + argument.text;
    }
    const bool returns_value = item.type != value_type::void_type;
    const auto output = returns_value ? allocate(item.type, item.position) : "";
    if (returns_value)
    {
        parameters += ", ptr " + output;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_vector_" + operation + "_" +
                      vector_suffix(type) + "(" + parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    for (const auto& argument : arguments)
    {
        if (argument.text != "null")
        {
            release(argument);
        }
    }
    if (call.receiver && !borrowed)
    {
        release(vector);
    }
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    return {item.type, result};
}

} // namespace tx
