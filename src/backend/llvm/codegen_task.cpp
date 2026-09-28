#include "backend/llvm/codegen.hpp"

namespace tx
{
namespace
{

const char* result_suffix(const value_type& type)
{
    if (type == value_type::void_type)
    {
        return "void";
    }
    if (type == value_type::int_type)
    {
        return "i64";
    }
    if (type == value_type::float_type)
    {
        return "f64";
    }
    if (type == value_type::bool_type)
    {
        return "bool";
    }
    return type == value_type::str_type ? "str" : "value";
}

int result_kind(const value_type& type)
{
    if (type == value_type::void_type)
    {
        return 0;
    }
    if (type == value_type::int_type)
    {
        return 1;
    }
    if (type == value_type::float_type)
    {
        return 2;
    }
    if (type == value_type::bool_type)
    {
        return 3;
    }
    return type == value_type::str_type ? 4 : 5;
}

} // namespace

llvm_code_generator::ir_value llvm_code_generator::emit_task_wait(
    const expression& item, const ir_value& argument)
{
    const bool returns_value = item.type != value_type::void_type;
    const auto output = returns_value ? allocate(item.type, item.position) : "";
    const auto status = temporary();
    std::string parameters = "ptr " + argument.text;
    if (returns_value)
    {
        parameters += ", ptr " + output;
    }
    write_instruction(status + " = call i32 @txrt_task_wait_" +
        result_suffix(item.type) + "(" + parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(argument);
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    if (item.type == value_type::str_type || is_value_handle(item.type))
    {
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + output);
        return {item.type, result};
    }
    return load({item.type, output});
}

llvm_code_generator::ir_value llvm_code_generator::emit_task_intrinsic(
    const expression& item, const function_decl& target,
    const std::vector<ir_value>& arguments)
{
    const auto operation = target.external_name.substr(5);
    if (operation == "wait")
    {
        return emit_task_wait(item, arguments.front());
    }
    const bool scope = operation == "scope";
    const auto kind = scope ? result_kind(item.type) :
        result_kind(arguments[1].type.parameters.back());
    const auto output = item.type == value_type::void_type
        ? std::string{} : allocate(item.type, item.position);
    std::string symbol;
    std::string parameters;
    if (scope)
    {
        symbol = "txrt_task_scope_" + std::string(result_suffix(item.type));
        parameters = "ptr " + arguments[0].text + ", i64 " +
            arguments[1].text + ", i64 " + arguments[2].text;
    }
    else
    {
        symbol = "txrt_task_spawn";
        parameters = "ptr " + arguments[0].text + ", ptr " +
            arguments[1].text + ", i64 " + std::to_string(kind) +
            ", ptr " + global_bytes(item.type.name);
    }
    if (!scope || item.type != value_type::void_type)
    {
        parameters += ", ptr " + output;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + symbol + "(" +
        parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    for (const auto& argument : arguments)
    {
        release(argument);
    }
    if (item.type == value_type::void_type)
    {
        return {value_type::void_type, {}};
    }
    if (item.type == value_type::str_type || is_value_handle(item.type))
    {
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + output);
        return {item.type, result};
    }
    return load({item.type, output});
}

llvm_code_generator::ir_value llvm_code_generator::emit_async_call(
    const expression& item, const call_expression& call,
    const function_decl& target)
{
    std::vector<value_type> parameter_types;
    parameter_types.reserve(target.parameters.size());
    for (const auto& parameter : target.parameters)
    {
        parameter_types.push_back(parameter.type);
    }
    const auto parent_type = value_type::function_of(
        std::move(parameter_types), target.return_type);
    const auto parent_slot = allocate(parent_type, item.position);
    const auto parent_status = temporary();
    write_instruction(parent_status + " = call i32 @txrt_closure_new(ptr " +
        callback_name(call.name, *call.overload_index) + ", ptr " +
        global_bytes(parent_type.name) + ", ptr " + parent_slot + ")");
    write_instruction("call void @txrt_require_success(i32 " + parent_status + ")");
    const auto parent = temporary();
    write_instruction(parent + " = load ptr, ptr " + parent_slot);
    std::string callback = parent;
    if (!call.arguments.empty())
    {
        const auto count = call.arguments.size();
        const auto wrapper = "@tx_bind_" + std::to_string(next_bind_++);
        emit_bind_wrapper(wrapper, parent_type, count);
        const auto array = "%slot" + std::to_string(next_slot_++);
        allocations_ << "  " << array << " = alloca [" << count << " x ptr]\n";
        std::vector<ir_value> values;
        std::vector<ir_value> boxes;
        for (std::size_t index = 0; index < count; ++index)
        {
            auto value = expression_value(*call.arguments[index].value);
            auto box = box_any(value, call.arguments[index].position);
            const auto address = temporary();
            write_instruction(address + " = getelementptr inbounds [" +
                std::to_string(count) + " x ptr], ptr " + array +
                ", i64 0, i64 " + std::to_string(index));
            write_instruction("store ptr " + box.text + ", ptr " + address);
            values.push_back(std::move(value));
            boxes.push_back(std::move(box));
        }
        const auto bound_type = value_type::function_of({}, target.return_type);
        const auto bound_slot = allocate(bound_type, item.position);
        const auto bind_status = temporary();
        write_instruction(bind_status + " = call i32 @txrt_closure_bind(ptr " +
            parent + ", ptr " + wrapper + ", ptr " +
            global_bytes(bound_type.name) + ", ptr " + array + ", i64 " +
            std::to_string(count) + ", ptr " + bound_slot + ")");
        write_instruction("call void @txrt_require_success(i32 " + bind_status + ")");
        for (std::size_t index = 0; index < count; ++index)
        {
            release(boxes[index]);
            release(values[index]);
        }
        callback = temporary();
        write_instruction(callback + " = load ptr, ptr " + bound_slot);
    }
    const auto output = allocate(item.type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_task_spawn_current(ptr " +
        callback + ", i64 " + std::to_string(result_kind(target.return_type)) +
        ", ptr " + global_bytes(item.type.name) +
        ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    if (callback != parent)
    {
        release({value_type::function_of({}, target.return_type), callback});
    }
    release({parent_type, parent});
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    return {item.type, result};
}

} // namespace tx
