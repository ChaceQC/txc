#include "backend/llvm/codegen.hpp"

namespace tx
{

std::optional<llvm_code_generator::ir_value>
llvm_code_generator::emit_literal_string_call(
    const expression& item, const call_expression& call,
    const function_decl& target)
{
    const auto& name = target.external_name;
    const bool replacement = name == "string.replace";
    const bool joining = name == "string.join";
    const bool supported = replacement || joining || name == "string.contains" ||
        name == "string.starts_with" || name == "string.ends_with" ||
        name == "string.find" || name == "string.split" || name == "string.split_vector";
    if (!supported || call.arguments.size() != (replacement ? 3U : 2U))
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
    if (!joining && call.arguments[0].value->type != value_type::str_type)
    {
        return std::nullopt;
    }
    for (std::size_t index = 1; index < call.arguments.size(); ++index)
    {
        if (!std::holds_alternative<string_literal>(call.arguments[index].value->data))
        {
            return std::nullopt;
        }
    }

    // 后续实参都是无副作用字面量，借用第一个实参不会改变求值顺序。
    bool borrowed = false;
    const auto& input = *call.arguments[0].value;
    const auto source = input.type.is_vector()
        ? container_value(input, true, borrowed)
        : input.type == value_type::str_type ? read_only_string_value(input, borrowed)
        : expression_value_or_borrow(input, borrowed);
    std::string parameters = "ptr " + source.text;
    for (std::size_t index = 1; index < call.arguments.size(); ++index)
    {
        const auto& literal = std::get<string_literal>(
            call.arguments[index].value->data);
        const auto decoded = decode_string_literal(literal.text);
        parameters += ", ptr " + global_bytes(decoded) + ", i64 " +
                      std::to_string(decoded.size());
    }
    const auto address = allocate(item.type, item.position);
    parameters += ", ptr " + address;
    const std::string symbol = "txrt_string_" +
        (joining && input.type.is_vector() ? "join_vector" : name.substr(7)) + "_literal";
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + symbol + "(" +
                      parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    if (!borrowed)
    {
        release(source);
    }
    if (item.type == value_type::str_type || is_value_handle(item.type))
    {
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        return ir_value{item.type, result};
    }
    return load({item.type, address});
}

} // namespace tx
