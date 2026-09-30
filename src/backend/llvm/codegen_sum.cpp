#include "backend/llvm/codegen.hpp"
#include <algorithm>

namespace tx
{

void llvm_code_generator::write_sum_declarations()
{
    module_ << R"txabi(
declare i32 @txrt_option_new(ptr, i1, ptr, ptr)
declare i32 @txrt_option_new_i64(ptr, i1, i64, ptr)
declare i32 @txrt_option_new_f64(ptr, i1, double, ptr)
declare i32 @txrt_option_new_bool(ptr, i1, i1, ptr)
declare i32 @txrt_result_new(ptr, i1, ptr, ptr)
declare i32 @txrt_result_from_legacy(ptr, ptr, ptr)
declare i1 @txrt_sum_state(ptr)
declare i32 @txrt_option_value(ptr, ptr)
declare i32 @txrt_option_value_i64(ptr, ptr)
declare i32 @txrt_option_value_f64(ptr, ptr)
declare i32 @txrt_option_value_bool(ptr, ptr)
declare i32 @txrt_option_unpack_i64(ptr, ptr, ptr)
declare i32 @txrt_option_unpack_f64(ptr, ptr, ptr)
declare i32 @txrt_option_unpack_bool(ptr, ptr, ptr)
declare i32 @txrt_option_empty_error()
declare i32 @txrt_db_get_int_required(ptr, i64, ptr)
declare i32 @txrt_db_get_float_required(ptr, i64, ptr)
declare i32 @txrt_db_get_bool_required(ptr, i64, ptr)
declare i32 @txrt_db_get_str_required(ptr, i64, ptr)
declare i32 @txrt_channel_recv_required_i64(ptr, i64, ptr, ptr)
declare i32 @txrt_channel_recv_required_f64(ptr, i64, ptr, ptr)
declare i32 @txrt_channel_recv_required_bool(ptr, i64, ptr, ptr)
declare i32 @txrt_option_value_or(ptr, ptr, ptr)
declare i32 @txrt_result_value(ptr, ptr)
declare i32 @txrt_result_error(ptr, ptr)
)txabi";
}

llvm_code_generator::ir_value llvm_code_generator::emit_sum_call(
    const expression& item, const call_expression& call)
{
    if (const auto native = emit_native_option_method(item, call))
    {
        return *native;
    }
    // 临时 option 没有身份或别名观察点，直接读取列并执行原有空值检查。
    // 实参仍按源码顺序求值，错误位置保持为外层 value() 调用位置。
    if (call.receiver && call.receiver->type.is_option() && call.name == "value")
    {
        const auto* inner = std::get_if<call_expression>(&call.receiver->data);
        const bool positional = inner && std::all_of(inner->arguments.begin(),
            inner->arguments.end(), [](const call_argument& argument)
            {
                return argument.kind == argument_kind::positional;
            });
        if (positional && inner->overload_index && !inner->receiver &&
            inner->arguments.size() == 3 && functions_.contains(inner->name) &&
            scalar_option_suffix(item.type))
        {
            const auto& target = *functions_.at(inner->name).at(*inner->overload_index);
            if (target.external && target.external_name == "channel.recv")
            {
                const auto channel = call_argument_value(*inner, 0);
                const auto timeout = call_argument_value(*inner, 1);
                const auto token = call_argument_value(*inner, 2);
                const auto output = allocate(item.type, item.position);
                const auto status = temporary();
                write_instruction(status + " = call i32 @txrt_channel_recv_required_" +
                    scalar_option_suffix(item.type) + "(ptr " + channel.text +
                    ", i64 " + timeout.text + ", ptr " + token.text + ", ptr " + output + ")");
                write_instruction("call void @txrt_require_success(i32 " + status + ")");
                release(channel);
                release(timeout);
                release(token);
                return load({item.type, output});
            }
        }
        if (positional && inner->overload_index && !inner->receiver &&
            inner->arguments.size() == 2 && functions_.contains(inner->name))
        {
            const auto& target = *functions_.at(inner->name).at(*inner->overload_index);
            const auto& name = target.external_name;
            if (target.external && (name == "db.get_int" || name == "db.get_float" ||
                name == "db.get_bool" || name == "db.get_str"))
            {
                const auto row = call_argument_value(*inner, 0);
                const auto index = call_argument_value(*inner, 1);
                const auto output = allocate(item.type, item.position);
                const auto status = temporary();
                write_instruction(status + " = call i32 @txrt_db_" + name.substr(3) +
                    "_required(ptr " + row.text + ", i64 " + index.text +
                    ", ptr " + output + ")");
                write_instruction("call void @txrt_require_success(i32 " + status + ")");
                release(row);
                release(index);
                if (item.type == value_type::str_type)
                {
                    const auto value = temporary();
                    write_instruction(value + " = load ptr, ptr " + output);
                    return {item.type, value};
                }
                return load({item.type, output});
            }
        }
    }
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
            const auto* scalar_suffix = type.is_option() && input
                ? scalar_option_suffix(input->type) : nullptr;
            std::string value = "null";
            if (input)
            {
                if (is_value_handle(input->type))
                {
                    value = input->text;
                }
                else if (!scalar_suffix)
                {
                    boxed = box_any(*input, item.position);
                    value = boxed.text;
                }
            }
            if (scalar_suffix)
            {
                invocation = "@txrt_option_new_" + std::string(scalar_suffix) +
                    "(ptr " + name + ", i1 true, " +
                    llvm_type(input->type, item.position) + " " + input->text;
            }
            else
            {
                invocation = "@txrt_" + type.container_name() + "_new(ptr " +
                    name + ", i1 " + (type.is_option() ?
                        (present ? "true" : "false") :
                        (success ? "true" : "false")) + ", ptr " + value;
            }
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
    bool borrowed_receiver = false;
    const auto receiver = call.name == "value_or"
        ? expression_value(*call.receiver)
        : expression_value_or_borrow(*call.receiver, borrowed_receiver);
    if (type.is_option() && call.name == "value")
    {
        if (const auto* suffix = scalar_option_suffix(element))
        {
            const auto output = allocate(element, item.position);
            const auto status = temporary();
            write_instruction(status + " = call i32 @txrt_option_value_" +
                suffix + "(ptr " + receiver.text + ", ptr " + output + ")");
            write_instruction("call void @txrt_require_success(i32 " +
                              status + ")");
            if (!borrowed_receiver)
            {
                release(receiver);
            }
            return load({element, output});
        }
    }
    if (call.name == "is_some" || call.name == "is_none" ||
        call.name == "is_ok" || call.name == "is_err")
    {
        const auto state = temporary();
        write_instruction(state + " = call i1 @txrt_sum_state(ptr " +
                          receiver.text + ")");
        if (!borrowed_receiver)
        {
            release(receiver);
        }
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
    if (!borrowed_receiver)
    {
        release(receiver);
    }
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    const auto handle = temporary();
    write_instruction(handle + " = load ptr, ptr " + output);
    if (is_value_handle(item.type))
    {
        // option/result 的元素类型已由语义分析确定，取值 ABI 已产生独立根。
        // 直接交付该根，避免将它当作动态 cast 再检查并复制一次。
        return {item.type, handle};
    }
    const ir_value dynamic{value_type::any_type, handle};
    const auto result = from_any(dynamic,
        call.name == "error" ? item.type : element, item.position);
    release(dynamic);
    return result;
}

} // namespace tx
