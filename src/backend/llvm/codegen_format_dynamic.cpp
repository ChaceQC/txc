#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_dynamic_format(
    const expression& item, const call_expression& call, const function_decl& target)
{
    if (target.external_name != "format.format" || call.arguments.empty() ||
        call.arguments.front().kind != argument_kind::positional)
    {
        return std::nullopt;
    }
    for (std::size_t index = 1; index < call.arguments.size(); ++index)
    {
        const auto& argument = call.arguments[index];
        const auto& type = argument.value->type;
        if ((argument.kind != argument_kind::positional && argument.kind != argument_kind::keyword) ||
            (type != value_type::int_type && type != value_type::float_type &&
             type != value_type::bool_type && type != value_type::str_type))
        {
            return std::nullopt;
        }
    }
    std::vector<ir_value> values;
    for (std::size_t index = 0; index < call.arguments.size(); ++index)
    {
        // 按源码顺序先求值；可能重绑定早先文本的后续表达式会阻止借用。
        values.push_back(call_argument_value(call, index));
    }
    const auto count = call.arguments.size() - 1;
    const auto positional = static_cast<std::size_t>(std::count_if(
        call.arguments.begin() + 1, call.arguments.end(), [](const call_argument& argument)
        {
            return argument.kind == argument_kind::positional;
        }));
    const auto storage = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << storage << " = alloca [" << count << " x { ptr, ptr, i64 }]\n";
    std::size_t next_positional = 0;
    std::size_t next_keyword = positional;
    for (std::size_t index = 1; index < values.size(); ++index)
    {
        const auto& argument = call.arguments[index];
        const auto& value = values[index];
        const bool named = argument.kind == argument_kind::keyword;
        const auto destination = named ? next_keyword++ : next_positional++;
        const auto slot = temporary();
        write_instruction(slot + " = getelementptr { ptr, ptr, i64 }, ptr " + storage +
            ", i64 " + std::to_string(destination));
        write_instruction("store ptr " + (named ? global_bytes(argument.name) : "null") + ", ptr " + slot);
        const auto callback = temporary();
        const auto bits_slot = temporary();
        write_instruction(callback + " = getelementptr { ptr, ptr, i64 }, ptr " + slot + ", i32 0, i32 1");
        write_instruction(bits_slot + " = getelementptr { ptr, ptr, i64 }, ptr " + slot + ", i32 0, i32 2");
        const auto suffix = value.type == value_type::int_type ? "i64" :
            value.type == value_type::float_type ? "f64" : value.type == value_type::bool_type ? "bool" : "str";
        write_instruction("store ptr @tx_format_argument_" + std::string(suffix) + ", ptr " + callback);
        auto bits = value.text;
        if (value.type != value_type::int_type)
        {
            bits = temporary();
            const auto operation = value.type == value_type::float_type ? "bitcast double" :
                value.type == value_type::bool_type ? "zext i1" : "ptrtoint ptr";
            write_instruction(bits + " = " + operation + " " + value.text + " to i64");
        }
        write_instruction("store i64 " + bits + ", ptr " + bits_slot);
    }
    const auto output = allocate(value_type::str_type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_format_arguments_context(ptr %tx_context, ptr " +
        values.front().text + ", ptr " + storage + ", i64 " + std::to_string(positional) +
        ", i64 " + std::to_string(count) + ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    for (const auto& value : values)
    {
        release(value);
    }
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    return ir_value{value_type::str_type, result};
}

} // namespace tx
