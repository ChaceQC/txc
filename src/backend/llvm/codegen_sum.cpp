#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::write_sum_declarations()
{
    module_ << R"txabi(
declare i32 @txrt_option_new(ptr, i1, ptr, ptr)
declare i32 @txrt_result_new(ptr, i1, ptr, ptr)
declare i32 @txrt_result_from_legacy(ptr, ptr, ptr)
declare i1 @txrt_sum_state(ptr)
declare i32 @txrt_option_value(ptr, ptr)
declare i32 @txrt_option_value_or(ptr, ptr, ptr)
declare i32 @txrt_result_value(ptr, ptr)
declare i32 @txrt_result_error(ptr, ptr)
)txabi";
}

llvm_code_generator::ir_value llvm_code_generator::emit_sum_call(
    const expression& item, const call_expression& call)
{
    const auto& type = call.container_type ? *call.container_type : call.receiver->type;
    const auto& element = type.parameters.front();
    if (call.container_type)
    {
        std::vector<ir_value> arguments;
        for (const auto& argument : call.arguments)
        {
            arguments.push_back(expression_value(*argument.value));
        }
        const bool legacy = type.is_result() && arguments.size() == 1 &&
            arguments.front().type != element;
        ir_value boxed{value_type::void_type, {}};
        std::string invocation;
        const auto name = global_bytes(type.name);
        if (legacy)
        {
            invocation = "@txrt_result_from_legacy(ptr " + name +
                ", ptr " + arguments.front().text;
        }
        else
        {
            const bool success = arguments.size() != 2;
            const bool present = !arguments.empty();
            const auto* input = arguments.empty() ? nullptr :
                &arguments[type.is_result() && !success ? 1 : 0];
            std::string value = "null";
            if (input)
            {
                if (is_value_handle(input->type))
                {
                    value = input->text;
                }
                else
                {
                    boxed = box_any(*input, item.position);
                    value = boxed.text;
                }
            }
            invocation = "@txrt_" + type.container_name() + "_new(ptr " +
                name + ", i1 " + (type.is_option() ?
                    (present ? "true" : "false") :
                    (success ? "true" : "false")) + ", ptr " + value;
        }
        const auto output = allocate(type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 " + invocation +
                          ", ptr " + output + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        release(boxed);
        for (const auto& argument : arguments)
        {
            release(argument);
        }
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + output);
        return {type, result};
    }
    const auto receiver = expression_value(*call.receiver);
    if (call.name == "is_some" || call.name == "is_none" ||
        call.name == "is_ok" || call.name == "is_err")
    {
        const auto state = temporary();
        write_instruction(state + " = call i1 @txrt_sum_state(ptr " +
                          receiver.text + ")");
        release(receiver);
        if (call.name == "is_none" || call.name == "is_err")
        {
            const auto inverted = temporary();
            write_instruction(inverted + " = xor i1 " + state + ", true");
            return {value_type::bool_type, inverted};
        }
        return {value_type::bool_type, state};
    }
    ir_value fallback{value_type::void_type, {}};
    ir_value boxed{value_type::void_type, {}};
    if (call.name == "value_or")
    {
        fallback = expression_value(*call.arguments.front().value);
        if (!is_value_handle(fallback.type))
        {
            boxed = box_any(fallback, item.position);
        }
    }
    const std::string function = type.is_option()
        ? (call.name == "value_or" ? "txrt_option_value_or" : "txrt_option_value")
        : (call.name == "error" ? "txrt_result_error" : "txrt_result_value");
    const bool returns_value = item.type != value_type::void_type;
    const auto output = returns_value ? allocate(value_type::any_type,
        item.position) : "";
    std::string parameters = "ptr " + receiver.text;
    if (call.name == "value_or")
    {
        parameters += ", ptr " + (boxed.type == value_type::void_type
            ? fallback.text : boxed.text);
    }
    parameters += ", ptr " + (returns_value ? output : "null");
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + function + "(" +
                      parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(boxed);
    release(fallback);
    release(receiver);
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    const auto handle = temporary();
    write_instruction(handle + " = load ptr, ptr " + output);
    const ir_value dynamic{value_type::any_type, handle};
    const auto result = from_any(dynamic,
        call.name == "error" ? item.type : element, item.position);
    release(dynamic);
    return result;
}

} // namespace tx
