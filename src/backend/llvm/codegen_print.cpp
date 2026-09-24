#include "backend/llvm/codegen.hpp"

#include <optional>

namespace tx
{

void llvm_code_generator::emit_print_char(int value)
{
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_print_char(i8 " +
                      std::to_string(value) + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
}

void llvm_code_generator::emit_print_value(const ir_value& value,
                                           bool newline, source_pos position)
{
    const auto* name = value.type == value_type::int_type
        ? "txrt_print_i64" : value.type == value_type::float_type
        ? "txrt_print_f64" : value.type == value_type::bool_type
        ? "txrt_print_bool" : value.type == value_type::str_type
        ? "txrt_print_str" : "txrt_value_print";
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + name + "(" +
                      llvm_type(value.type, position) + " " + value.text +
                      ", i1 " + (newline ? "true" : "false") + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
}

void llvm_code_generator::emit_print_call(
    const expression& item, const call_expression& call,
    const std::vector<ir_value>& arguments)
{
    std::vector<std::size_t> value_indices;
    std::optional<std::size_t> sep_index;
    std::optional<std::size_t> end_index;
    for (std::size_t index = 0; index < call.arguments.size(); ++index)
    {
        const auto& argument = call.arguments[index];
        if (argument.kind == argument_kind::positional)
        {
            value_indices.push_back(index);
        }
        else if (argument.name == "sep")
        {
            sep_index = index;
        }
        else
        {
            end_index = index;
        }
    }
    for (std::size_t index = 0; index < value_indices.size(); ++index)
    {
        if (index != 0)
        {
            if (sep_index)
            {
                emit_print_value(arguments[*sep_index], false, item.position);
            }
            else
            {
                emit_print_char(' ');
            }
        }
        const bool newline = index + 1 == value_indices.size() && !end_index;
        emit_print_value(arguments[value_indices[index]], newline, item.position);
    }
    if (end_index)
    {
        emit_print_value(arguments[*end_index], false, item.position);
    }
    else if (value_indices.empty())
    {
        emit_print_char('\n');
    }
    for (const auto& argument : arguments)
    {
        release(argument);
    }
}

} // namespace tx
