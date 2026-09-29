#include "backend/llvm/codegen.hpp"
#include "backend/llvm/codegen_format_plan.hpp"

#include <algorithm>

namespace tx
{

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_static_format(
    const expression& item, const call_expression& call, const function_decl& target)
{
    if (target.external_name != "format.format" || call.arguments.empty() ||
        call.arguments.front().kind != argument_kind::positional)
    {
        return std::nullopt;
    }
    const auto* literal = std::get_if<string_literal>(&call.arguments.front().value->data);
    if (!literal)
    {
        return std::nullopt;
    }
    for (std::size_t index = 1; index < call.arguments.size(); ++index)
    {
        const auto& type = call.arguments[index].value->type;
        if (type != value_type::int_type && type != value_type::float_type &&
            type != value_type::bool_type && type != value_type::str_type)
        {
            return std::nullopt;
        }
    }
    const auto plan = plan_static_format(decode_string_literal(literal->text), call);
    if (!plan)
    {
        return std::nullopt;
    }
    // 在任何格式化动作之前按源码顺序求值，未引用的实参也保留副作用与生命周期。
    std::vector<ir_value> values{{value_type::void_type, {}}};
    for (std::size_t index = 1; index < call.arguments.size(); ++index)
    {
        const auto& argument = *call.arguments[index].value;
        const bool stable_suffix = std::all_of(
            call.arguments.begin() + index + 1, call.arguments.end(),
            [&](const call_argument& following)
            {
                return stable_value_expression(*following.value);
            });
        bool borrowed = false;
        auto value = argument.type == value_type::str_type && stable_suffix
            ? read_only_string_value(argument, borrowed)
            : expression_value(argument);
        value.borrowed = borrowed;
        values.push_back(value);
    }
    const auto output = allocate(value_type::str_type, item.position);
    const auto created = temporary();
    write_instruction(created + " = call i32 @txrt_format_begin(ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + created + ")");
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    for (const auto& part : *plan)
    {
        const auto status = temporary();
        if (!part.argument)
        {
            write_instruction(status + " = call i32 @txrt_format_literal(ptr " + result +
                ", ptr " + global_bytes(part.literal) + ", i64 " + std::to_string(part.literal.size()) + ")");
        }
        else
        {
            const auto spec = "@.format_spec." + std::to_string(next_string_++);
            const auto& value = part.spec;
            globals_ << spec << " = private constant [7 x i64] [i64 " << value.fill
                << ", i64 " << value.align << ", i64 " << value.sign << ", i64 " << value.type
                << ", i64 " << value.zero << ", i64 " << value.width << ", i64 " << value.precision << "]\n";
            const auto& argument = values[*part.argument];
            const auto suffix = argument.type == value_type::int_type ? "i64" :
                argument.type == value_type::float_type ? "f64" :
                argument.type == value_type::bool_type ? "bool" : "str";
            write_instruction(status + " = call i32 @txrt_format_append_" + suffix +
                "(ptr " + result + ", " + llvm_type(argument.type, item.position) + " " + argument.text +
                ", ptr " + spec + ", i8 " + std::to_string(part.conversion) + ")");
        }
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
    }
    write_instruction("call void @txrt_format_finish(ptr " + result + ")");
    for (const auto& value : values)
    {
        release(value);
    }
    return ir_value{value_type::str_type, result};
}

} // namespace tx
