#include "backend/llvm/codegen.hpp"
#include "common/text_encoding.hpp"

#include <stdexcept>

namespace tx
{

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_constant_encoding(
    const expression& item, const call_expression& call, const function_decl& target, bool length_only)
{
    if ((target.external_name != "encoding.encode" && target.external_name != "encoding.decode") ||
        call.arguments.size() != 2 || call.arguments[0].kind != argument_kind::positional ||
        call.arguments[1].kind != argument_kind::positional)
    {
        return std::nullopt;
    }
    const auto encoding_name = constant_format_text(*call.arguments[1].value);
    if (!encoding_name)
    {
        return std::nullopt;
    }
    text_encoding encoding;
    try
    {
        encoding = parse_encoding(*encoding_name);
    }
    catch (const std::runtime_error&)
    {
        return std::nullopt;
    }
    bool borrowed = false;
    const auto& input = *call.arguments[0].value;
    const auto result_type = length_only ? value_type::int_type : item.type;
    const std::string suffix = length_only ? "_length" : "";
    if (target.external_name == "encoding.encode")
    {
        if (const auto* text = std::get_if<string_literal>(&input.data))
        {
            const auto bytes = decode_string_literal(text->text);
            const auto output = allocate(result_type, item.position);
            const auto status = temporary();
            write_instruction(status + " = call i32 @txrt_encoding_encode_literal" + suffix + "(ptr " +
                global_bytes(bytes) + ", i64 " + std::to_string(bytes.size()) + ", i64 " +
                std::to_string(static_cast<std::int64_t>(encoding)) + ", ptr " + output + ")");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
            const auto result = temporary();
            write_instruction(result + " = load " + llvm_type(result_type, item.position) + ", ptr " + output);
            return ir_value{result_type, result};
        }
    }
    const auto source = input.type == value_type::str_type
        ? read_only_string_value(input, borrowed) : expression_value_or_borrow(input, borrowed);
    const auto output = allocate(result_type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_encoding_" +
        (target.external_name == "encoding.encode" ? "encode" : "decode") +
        "_known" + suffix + "(ptr " + source.text + ", i64 " + std::to_string(static_cast<std::int64_t>(encoding)) +
        ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    if (!borrowed)
    {
        release(source);
    }
    const auto result = temporary();
    write_instruction(result + " = load " + llvm_type(result_type, item.position) + ", ptr " + output);
    return ir_value{result_type, result};
}

} // namespace tx
