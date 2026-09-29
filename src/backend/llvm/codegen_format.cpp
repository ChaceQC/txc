#include "backend/llvm/codegen.hpp"
#include "backend/llvm/codegen_format_plan.hpp"

#include <algorithm>
#include <limits>

namespace tx
{
namespace
{

std::uint64_t format_capacity(const std::vector<static_format_part>& plan,
    const call_expression& call, const std::vector<std::optional<std::string>>& bytes,
    source_pos position)
{
    std::uint64_t capacity = 0;
    const auto add = [&](std::uint64_t extra)
    {
        if (extra > std::numeric_limits<std::uint64_t>::max() - capacity)
        {
            throw compile_error(position, "format 输出长度过大");
        }
        capacity += extra;
    };
    for (const auto& part : plan)
    {
        add(part.literal.size());
        add(part.tail.size());
        if (part.argument && plain_format_part(part, call.arguments[*part.argument].value->type))
        {
            const auto index = *part.argument;
            // 整数和运行时文本按需增长，避免按最大整数宽度使短结果失去小字符串存储。
            add(bytes[index] ? bytes[index]->size() :
                call.arguments[index].value->type == value_type::bool_type ? 5 : 0);
        }
    }
    return capacity;
}

} // namespace

std::vector<llvm_code_generator::ir_value> llvm_code_generator::emit_format_arguments(
    const call_expression& call, const std::vector<static_format_part>& plan,
    std::vector<std::optional<std::string>>& bytes)
{
    // 在任何格式化动作之前按源码顺序求值，未引用的实参也保留副作用与生命周期。
    std::vector<ir_value> values{{value_type::void_type, {}}};
    for (std::size_t index = 1; index < call.arguments.size(); ++index)
    {
        const auto& argument = *call.arguments[index].value;
        if (std::holds_alternative<string_literal>(argument.data) &&
            std::all_of(plan.begin(), plan.end(), [&](const static_format_part& part)
            {
                return part.argument != index || plain_format_part(part, argument.type);
            }))
        {
            bytes[index] = constant_format_text(argument);
            values.push_back({argument.type, {}, true});
            continue;
        }
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
    return values;
}

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_static_format(
    const expression& item, const call_expression& call, const function_decl& target)
{
    if (target.external_name != "format.format" || call.arguments.empty() ||
        call.arguments.front().kind != argument_kind::positional)
    {
        return std::nullopt;
    }
    const auto text = constant_format_text(*call.arguments.front().value);
    if (!text)
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
    const auto plan = plan_static_format(*text, call);
    if (!plan)
    {
        return std::nullopt;
    }
    std::vector<std::optional<std::string>> bytes(call.arguments.size());
    const auto values = emit_format_arguments(call, *plan, bytes);
    const auto output = allocate(value_type::str_type, item.position);
    const auto created = temporary();
    const auto capacity = format_capacity(*plan, call, bytes, item.position);
    const std::string prefix = !plan->empty() && !plan->front().argument ? plan->front().literal : "";
    const auto steps = static_format_steps(call, *plan, bytes);
    const auto arguments = static_format_arguments(values, bytes);
    const auto count = std::count_if(plan->begin(), plan->end(), [](const static_format_part& part)
    {
        return part.argument.has_value();
    });
    write_instruction(created + " = call i32 @txrt_format_execute(ptr " + steps +
        ", i64 " + std::to_string(count) + ", ptr " + arguments + ", ptr " + global_bytes(prefix) +
        ", i64 " + std::to_string(prefix.size()) + ", i64 " + std::to_string(capacity) +
        ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + created + ")");
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    for (std::size_t index = 1; index < values.size(); ++index)
    {
        if (!bytes[index])
        {
            release(values[index]);
        }
    }
    return ir_value{value_type::str_type, result};
}

} // namespace tx
