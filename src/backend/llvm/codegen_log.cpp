#include "backend/llvm/codegen.hpp"

namespace tx
{

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_lazy_log(
    const expression& item, const call_expression& call, const function_decl& target)
{
    if (target.external_name != "log.event_lazy" || call.arguments.size() != 3)
    {
        return std::nullopt;
    }
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            return std::nullopt;
        }
    }
    const auto* function = std::get_if<name_reference>(&call.arguments[2].value->data);
    if (!function || !function->function_value)
    {
        return std::nullopt;
    }
    // 仅省略静态函数引用的闭包物化；级别和消息仍先求值，bind、调用等
    // 可能产生副作用的闭包实参仍使用完整原生路径。
    const auto* level_literal = std::get_if<string_literal>(&call.arguments[0].value->data);
    const auto* message_literal = std::get_if<string_literal>(&call.arguments[1].value->data);
    const bool literals = level_literal && message_literal;
    auto level = literals ? ir_value{value_type::void_type, {}}
        : expression_value(*call.arguments[0].value);
    auto message = literals ? ir_value{value_type::void_type, {}}
        : expression_value(*call.arguments[1].value);
    const auto enabled_slot = allocate(value_type::bool_type, item.position);
    const auto status = temporary();
    if (literals)
    {
        const auto text = decode_string_literal(level_literal->text);
        write_instruction(status + " = call i32 @txrt_log_enabled_literal(ptr " +
            global_bytes(text) + ", i64 " + std::to_string(text.size()) +
            ", ptr " + enabled_slot + ")");
    }
    else
    {
        write_instruction(status + " = call i32 @txrt_log_enabled(ptr " + level.text +
            ", ptr " + enabled_slot + ")");
    }
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto enabled = load({value_type::bool_type, enabled_slot});
    const auto emit = label();
    const auto done = label();
    write_instruction("br i1 " + enabled.text + ", label %" + emit + ", label %" + done);
    start_block(emit);
    if (literals)
    {
        level = expression_value(*call.arguments[0].value);
        message = expression_value(*call.arguments[1].value);
    }
    const auto fields = expression_value(*call.arguments[2].value);
    const auto logged = temporary();
    write_instruction(logged + " = call i32 @txrt_log_event_lazy(ptr " + level.text +
        ", ptr " + message.text + ", ptr " + fields.text + ")");
    write_instruction("call void @txrt_require_success(i32 " + logged + ")");
    release(fields);
    if (literals)
    {
        release(message);
        release(level);
    }
    write_instruction("br label %" + done);
    start_block(done);
    if (!literals)
    {
        release(message);
        release(level);
    }
    return ir_value{value_type::void_type, {}};
}

} // namespace tx
