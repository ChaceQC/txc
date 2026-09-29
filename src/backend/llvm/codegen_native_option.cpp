#include "backend/llvm/codegen.hpp"

namespace tx
{

const char* llvm_code_generator::scalar_option_suffix(const value_type& type)
{
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
    return nullptr;
}

bool llvm_code_generator::emit_native_option_declaration(
    const statement& item, const variable_declaration& declaration)
{
    if (declaration.array_length || !declaration.initializer)
    {
        return false;
    }
    const auto& type = declaration.declared_type
        ? *declaration.declared_type : declaration.initializer->type;
    if (!type.is_option() || !scalar_option_suffix(type.parameters.front()))
    {
        return false;
    }

    const auto& element = type.parameters.front();
    const auto present = allocate(value_type::bool_type, item.position);
    const auto scalar = allocate(element, item.position);
    variable_slot slot{type, present};
    slot.native_option_value = scalar;
    const auto* call = std::get_if<call_expression>(&declaration.initializer->data);
    if (call && call->container_type && call->container_type->is_option())
    {
        if (call->arguments.empty())
        {
            const auto zero = element == value_type::float_type ? "0.0"
                : element == value_type::bool_type ? "false" : "0";
            write_instruction("store i1 false, ptr " + present);
            write_instruction("store " + llvm_type(element, item.position) +
                              " " + zero + ", ptr " + scalar);
        }
        else
        {
            const auto value = expression_value(*call->arguments.front().value);
            write_instruction("store " + llvm_type(element, item.position) +
                              " " + value.text + ", ptr " + scalar);
            write_instruction("store i1 true, ptr " + present);
            release(value);
        }
    }
    else if (call && call->receiver && call->receiver->type.is_iterator() &&
             call->name == "next")
    {
        if (emit_cursor_next(*call, present, scalar))
        {
            scopes_.back().emplace(declaration.name, std::move(slot));
            return true;
        }
        bool borrowed = false;
        const auto receiver = expression_value_or_borrow(*call->receiver, borrowed);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_iterator_next_scalar_" +
            scalar_option_suffix(element) + "(ptr " + receiver.text +
            ", ptr " + present + ", ptr " + scalar + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        if (!borrowed)
        {
            release(receiver);
        }
    }
    else
    {
        store_native_option(slot, expression_value(*declaration.initializer),
                            item.position);
    }
    scopes_.back().emplace(declaration.name, std::move(slot));
    return true;
}

llvm_code_generator::ir_value llvm_code_generator::load_native_option(
    const variable_slot& variable)
{
    const auto& element = variable.type.parameters.front();
    const auto present = temporary();
    const auto scalar = temporary();
    write_instruction(present + " = load i1, ptr " + variable.address);
    write_instruction(scalar + " = load " + llvm_type(element, {}) +
                      ", ptr " + variable.native_option_value);
    const auto output = allocate(variable.type, {});
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_option_new_" +
        scalar_option_suffix(element) + "(ptr " + global_bytes(variable.type.name) +
        ", i1 " + present + ", " + llvm_type(element, {}) + " " + scalar +
        ", ptr " + output + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + output);
    return {variable.type, result};
}

void llvm_code_generator::store_native_option(const variable_slot& variable,
    const ir_value& value, source_pos position)
{
    const auto& element = variable.type.parameters.front();
    const auto present_slot = allocate(value_type::bool_type, position);
    const auto scalar_slot = allocate(element, position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_option_unpack_" +
        scalar_option_suffix(element) + "(ptr " + value.text + ", ptr " +
        present_slot + ", ptr " + scalar_slot + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto present = temporary();
    const auto scalar = temporary();
    write_instruction(present + " = load i1, ptr " + present_slot);
    write_instruction(scalar + " = load " + llvm_type(element, position) +
                      ", ptr " + scalar_slot);
    write_instruction("store " + llvm_type(element, position) + " " +
                      scalar + ", ptr " + variable.native_option_value);
    write_instruction("store i1 " + present + ", ptr " + variable.address);
    release(value);
}

std::optional<llvm_code_generator::ir_value>
llvm_code_generator::emit_native_option_method(
    const expression& item, const call_expression& call)
{
    if (!call.receiver || !call.receiver->type.is_option() ||
        !scalar_option_suffix(call.receiver->type.parameters.front()))
    {
        return std::nullopt;
    }
    const auto* name = std::get_if<name_reference>(&call.receiver->data);
    if (!name || name->function_value)
    {
        return std::nullopt;
    }
    const auto variable = find_variable(name->name, item.position);
    if (variable.native_option_value.empty())
    {
        return std::nullopt;
    }
    const auto present = temporary();
    write_instruction(present + " = load i1, ptr " + variable.address);
    if (call.name == "is_some")
    {
        return ir_value{value_type::bool_type, present};
    }
    if (call.name == "is_none")
    {
        const auto missing = temporary();
        write_instruction(missing + " = xor i1 " + present + ", true");
        return ir_value{value_type::bool_type, missing};
    }
    const auto& element = variable.type.parameters.front();
    const auto type = llvm_type(element, item.position);
    if (call.name == "value_or")
    {
        // fallback 仍按普通实参顺序求值，先固定调用目标的旧值。
        const auto current = temporary();
        write_instruction(current + " = load " + type + ", ptr " +
                          variable.native_option_value);
        const auto fallback = expression_value(*call.arguments.front().value);
        const auto selected = temporary();
        write_instruction(selected + " = select i1 " + present + ", " +
            type + " " + current + ", " + type + " " + fallback.text);
        release(fallback);
        return ir_value{element, selected};
    }
    if (call.name != "value")
    {
        return std::nullopt;
    }
    const auto ready = label();
    const auto missing = label();
    write_instruction("br i1 " + present + ", label %" + ready +
                      ", label %" + missing);
    start_block(missing);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_option_empty_error()");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    write_instruction("br label %" + ready);
    start_block(ready);
    const auto result = temporary();
    write_instruction(result + " = load " + type + ", ptr " +
                      variable.native_option_value);
    return ir_value{element, result};
}

} // namespace tx
