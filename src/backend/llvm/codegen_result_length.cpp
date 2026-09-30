#include "backend/llvm/codegen.hpp"
#include "backend/llvm/codegen_format_plan.hpp"
#include "common/utf8.hpp"
#include "common/slot_layout.hpp"

#include <algorithm>

namespace tx
{
namespace
{

std::size_t literal_length(std::string_view text)
{
    std::size_t length = 0;
    for (std::size_t offset = 0; offset < text.size(); ++length)
    {
        offset += utf8_width(text, offset);
    }
    return length;
}

} // namespace

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_result_length(const expression& item)
{
    if (const auto* name = std::get_if<name_reference>(&item.data))
    {
        const auto slot = find_variable(name->name, item.position);
        if (!slot.result_length.empty())
        {
            return load({value_type::int_type, slot.result_length});
        }
        return std::nullopt;
    }
    const auto* call = std::get_if<call_expression>(&item.data);
    if (!call || call->receiver || call->indirect || !call->overload_index)
    {
        return std::nullopt;
    }
    const auto found = functions_.find(call->name);
    if (found == functions_.end() || *call->overload_index >= found->second.size())
    {
        return std::nullopt;
    }
    const auto& target = *found->second[*call->overload_index];
    if (auto formatted = emit_format_length(item, *call, target))
    {
        return formatted;
    }
    return emit_constant_encoding(item, *call, target, true);
}

bool llvm_code_generator::emit_length_local(const statement& item, const variable_declaration& declaration)
{
    if (!declaration.initializer || declaration.array_length ||
        (declaration.initializer->type != value_type::str_type &&
         declaration.initializer->type != value_type::bytes_type) || !length_only_local(declaration))
    {
        return false;
    }
    const auto length = emit_result_length(*declaration.initializer);
    if (!length)
    {
        return false;
    }
    const auto address = allocate(value_type::int_type, item.position);
    write_instruction("store i64 " + length->text + ", ptr " + address);
    variable_slot slot{declaration.initializer->type, address};
    slot.result_length = address;
    scopes_.back().emplace(declaration.name, std::move(slot));
    return true;
}

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_format_length(
    const expression& item, const call_expression& call, const function_decl& target)
{
    if (target.external_name != "format.format" || call.arguments.empty() ||
        call.arguments.front().kind != argument_kind::positional)
    {
        return std::nullopt;
    }
    const auto text = constant_format_text(*call.arguments.front().value);
    const auto plan = text ? plan_static_format(*text, call) : std::nullopt;
    if (!plan || plan->size() > static_format_budget ||
        !std::all_of(call.arguments.begin() + 1, call.arguments.end(), [](const call_argument& argument)
        {
            const auto& type = argument.value->type;
            return type == value_type::int_type || type == value_type::bool_type || type == value_type::str_type;
        }) || !std::all_of(plan->begin(), plan->end(), [&](const static_format_part& part)
        {
            return !part.argument || plain_format_part(part, call.arguments[*part.argument].value->type);
        }))
    {
        return std::nullopt;
    }
    std::vector<std::optional<std::string>> bytes(call.arguments.size());
    const auto values = emit_format_arguments(call, *plan, bytes);
    ir_value total{value_type::int_type, "0"};
    for (const auto& part : *plan)
    {
        ir_value length{value_type::int_type, "0"};
        if (part.argument)
        {
            const auto index = *part.argument;
            const auto& value = values[index];
            if (bytes[index] || value.type == value_type::str_type)
            {
                const auto storage = allocate(value_type::int_type, item.position);
                const auto status = temporary();
                // 字面量仍验证 UTF-8，不能假定转义字节合法。
                const auto invocation = bytes[index]
                    ? "@txrt_format_literal_length(ptr " + global_bytes(*bytes[index]) +
                        ", i64 " + std::to_string(bytes[index]->size())
                    : "@txrt_str_len(ptr " + value.text;
                write_instruction(status + " = call i32 " + invocation + ", ptr " + storage + ")");
                write_instruction("call void @txrt_require_success(i32 " + status + ")");
                length = load({value_type::int_type, storage});
            }
            else
            {
                const auto result = temporary();
                write_instruction(result + (value.type == value_type::int_type
                    ? " = call i64 @txrt_format_integer_length(i64 " + value.text + ")"
                    : " = select i1 " + value.text + ", i64 4, i64 5"));
                length.text = result;
            }
        }
        const ir_value literal{value_type::int_type,
            std::to_string(literal_length(part.literal) + literal_length(part.tail))};
        total = checked_binary("txrt_add_i64", total, length, value_type::int_type, item.position);
        total = checked_binary("txrt_add_i64", total, literal, value_type::int_type, item.position);
    }
    for (std::size_t index = 1; index < values.size(); ++index)
    {
        if (!bytes[index])
        {
            release(values[index]);
        }
    }
    return total;
}

} // namespace tx
