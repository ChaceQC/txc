#include "backend/llvm/codegen.hpp"

#include <string>

namespace tx
{

llvm_code_generator::ir_value llvm_code_generator::emit_algorithm_intrinsic(
    const expression& item, const function_decl& target,
    const std::vector<ir_value>& arguments)
{
    const auto operation = target.external_name.substr(10);
    const auto& input_type = arguments.front().type;
    const auto& element = input_type.parameters.front();
    const auto suffix = vector_suffix(input_type);
    std::string symbol = "@txrt_algorithm_" + operation + "_" + suffix;
    const auto argument_address = [&](const ir_value& value)
    {
        if (value.type == value_type::str_type || is_value_handle(value.type))
        {
            return value.text;
        }
        const auto address = allocate(value.type, item.position, false);
        write_instruction("store " + llvm_type(value.type, item.position) +
            " " + value.text + ", ptr " + address);
        return address;
    };
    const auto callback = [&]() -> std::string
    {
        if (operation == "stable_sort" || operation == "stable_sorted" ||
            operation == "unique")
        {
            return arguments.size() == 2 ? arguments[1].text : "null";
        }
        if (operation == "binary_search" || operation == "equal_range")
        {
            return arguments.size() == 3 ? arguments[2].text : "null";
        }
        return "null";
    }();
    const auto less = callback == "null" && structs_.contains(element.name)
        ? key_less_symbol(element) : std::string("null");
    std::string parameters = "ptr " + arguments.front().text;
    if (operation == "stable_sort" || operation == "stable_sorted" ||
        operation == "unique")
    {
        parameters += ", ptr " + callback + ", ptr " + less;
    }
    else if (operation == "binary_search" || operation == "equal_range")
    {
        parameters += ", ptr " + argument_address(arguments[1]) +
            ", ptr " + callback + ", ptr " + less;
    }
    else if (operation == "rotate")
    {
        parameters += ", i64 " + arguments[1].text;
    }
    else if (operation == "partition" || operation == "filter" ||
             operation == "all" || operation == "any")
    {
        parameters += ", ptr " + arguments[1].text;
    }
    else if (operation == "map")
    {
        symbol += "_" + vector_suffix(item.type);
        parameters += ", ptr " + arguments[1].text + ", ptr " +
            global_bytes(item.type.parameters.front().name);
    }
    else if (operation == "fold")
    {
        symbol += "_" + vector_suffix(value_type::vector_of(item.type));
        parameters += ", ptr " + argument_address(arguments[1]) +
            ", ptr " + arguments[2].text;
    }
    const bool binary = operation == "binary_search";
    const bool returns_value = item.type != value_type::void_type;
    const auto output_type = binary ? value_type::int_type : item.type;
    const auto output = returns_value ? allocate(output_type, item.position) : "";
    if (returns_value)
    {
        parameters += ", ptr " + output;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 " + symbol + "(" + parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    if (binary)
    {
        const auto index = temporary();
        write_instruction(index + " = load i64, ptr " + output);
        const auto present = temporary();
        write_instruction(present + " = icmp sge i64 " + index + ", 0");
        const auto boxed = box_any({value_type::int_type, index}, item.position);
        const auto option_output = allocate(item.type, item.position);
        const auto option_status = temporary();
        write_instruction(option_status + " = call i32 @txrt_option_new(ptr " +
            global_bytes(item.type.name) + ", i1 " + present + ", ptr " +
            boxed.text + ", ptr " + option_output + ")");
        write_instruction("call void @txrt_require_success(i32 " +
            option_status + ")");
        release(boxed);
        for (const auto& argument : arguments)
        {
            release(argument);
        }
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + option_output);
        return {item.type, result};
    }
    for (const auto& argument : arguments)
    {
        release(argument);
    }
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
