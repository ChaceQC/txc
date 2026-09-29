#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

void llvm_code_generator::cache_record_view(variable_slot& slot, const std::string& value)
{
    if (!static_record_type(slot.type))
    {
        return;
    }
    slot.record_view = allocate(value_type::any_type, {}, false);
    refresh_record_view(slot, value);
}

void llvm_code_generator::refresh_record_view(const variable_slot& slot, const std::string& value)
{
    if (slot.record_view.empty())
    {
        return;
    }
    const auto view = record_view_value({slot.type, value});
    write_instruction("store ptr " + view + ", ptr " + slot.record_view);
}

std::string llvm_code_generator::record_view_value(const ir_value& object)
{
    const auto view = temporary();
    write_instruction(view + " = call ptr @" + std::string(classes_.contains(object.type.name)
        ? "txrt_record_class_view" : "txrt_record_struct_view") + "(ptr " + object.text + ")");
    return view;
}

std::string llvm_code_generator::record_field_slot(const ir_value& object, std::size_t index)
{
    const auto view = record_view_value(object);
    const auto data = temporary();
    write_instruction(data + " = load ptr, ptr " + view);
    const auto address = temporary();
    write_instruction(address + " = getelementptr inbounds i64, ptr " + data +
                      ", i64 " + std::to_string(index));
    return address;
}

llvm_code_generator::ir_value llvm_code_generator::read_record_field(
    const ir_value& object, const expression& item, const member_expression& member)
{
    const auto index = member.field_slot.value_or(
        classes_.contains(object.type.name) ? 0 : field_index(object.type, member.field, item.position));
    const auto address = record_field_slot(object, index);
    if (item.type == value_type::int_type || item.type == value_type::float_type ||
        item.type == value_type::bool_type)
    {
        return load({item.type, address});
    }
    const auto reference = temporary();
    write_instruction(reference + " = load ptr, ptr " + address);
    if (classes_.contains(object.type.name))
    {
        write_instruction("call void @txrt_record_require_initialized(ptr " + reference + ")");
    }
    if (item.type == value_type::str_type)
    {
        return from_any({value_type::any_type, reference}, item.type, item.position);
    }
    // 引用字段类型已由字段描述和赋值检查确定，只复制根句柄，不重复 require_type。
    const auto output = allocate(item.type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_value_clone(ptr " + reference + ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    return {item.type, result};
}

llvm_code_generator::ir_value llvm_code_generator::emit_record_constructor(
    const expression& item, const std::vector<ir_value>& arguments)
{
    const auto output = allocate(item.type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_record_struct_new(ptr @tx_record_" +
                      item.type.name + ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto object = temporary();
    write_instruction(object + " = load ptr, ptr " + output);
    const auto view = record_view_value({item.type, object});
    const auto data = temporary();
    write_instruction(data + " = load ptr, ptr " + view);
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        const auto& argument = arguments[index];
        const auto address = temporary();
        write_instruction(address + " = getelementptr inbounds i64, ptr " + data +
                          ", i64 " + std::to_string(index));
        if (argument.type == value_type::int_type || argument.type == value_type::float_type ||
            argument.type == value_type::bool_type)
        {
            write_instruction("store " + llvm_type(argument.type, item.position) + " " +
                              argument.text + ", ptr " + address);
        }
        else
        {
            const auto reference = temporary();
            write_instruction(reference + " = load ptr, ptr " + address);
            assign_any(reference, argument, item.position);
        }
        release(argument);
    }
    return {item.type, object};
}

bool llvm_code_generator::emit_stack_record(const statement& item,
    const variable_declaration& declaration)
{
    const auto* call = declaration.initializer
        ? std::get_if<call_expression>(&declaration.initializer->data) : nullptr;
    if (!call || !call->is_constructor || !structs_.contains(call->name) ||
        !static_record_type(value_type(call->name)) || !confined_local(declaration, true))
    {
        return false;
    }
    const auto& fields = structs_.at(call->name)->fields;
    if (call->arguments.size() != fields.size() || !std::all_of(
        call->arguments.begin(), call->arguments.end(), [](const call_argument& argument)
        {
            return argument.kind == argument_kind::positional;
        }) || !std::all_of(fields.begin(), fields.end(),
        [](const struct_field& field)
        {
            return field.type == value_type::int_type || field.type == value_type::float_type ||
                field.type == value_type::bool_type;
        }))
    {
        return false;
    }
    const auto data = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << data << " = alloca [" << fields.size() << " x i64], align 8\n";
    for (std::size_t index = 0; index < call->arguments.size(); ++index)
    {
        const auto value = expression_value(*call->arguments[index].value);
        const auto address = temporary();
        write_instruction(address + " = getelementptr inbounds i64, ptr " + data +
                          ", i64 " + std::to_string(index));
        write_instruction("store " + llvm_type(value.type, item.position) + " " + value.text +
                          ", ptr " + address);
    }
    variable_slot slot{value_type(call->name), data, true};
    slot.stack_record = data;
    scopes_.back().emplace(declaration.name, std::move(slot));
    return true;
}

} // namespace tx
