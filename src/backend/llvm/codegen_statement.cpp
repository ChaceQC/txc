#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::emit_statements(const std::vector<stmt_ptr>& statements)
{
    for (const auto& item : statements)
    {
        if (terminated_)
        {
            break;
        }
        emit_statement(*item);
    }
}

void llvm_code_generator::emit_declaration(
    const statement& item, const variable_declaration& declaration)
{
    ir_value value{value_type::void_type, {}};
    if (declaration.array_length)
    {
        const auto length = expression_value(*declaration.array_length);
        ir_value initial{value_type::void_type, {}};
        if (declaration.initializer)
        {
            initial = expression_value(*declaration.initializer);
        }
        const auto result_address = allocate(value_type::array_type,
                                             item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @" +
            std::string(declaration.initializer
                ? "txrt_array_resize(i64 " + length.text +
                  ", ptr " + initial.text
                : "txrt_array_new(i64 " + length.text) +
            ", ptr " + result_address + ")");
        write_instruction("call void @txrt_require_success(i32 " +
                          status + ")");
        release(initial);
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + result_address);
        value = {value_type::array_type, result};
    }
    else
    {
        value = expression_value(*declaration.initializer);
    }
    const auto type = declaration.declared_type.value_or(value.type);
    const auto address = allocate(type, item.position);
    write_instruction("store " + llvm_type(type, item.position) + " " +
                      value.text + ", ptr " + address);
    scopes_.back().emplace(declaration.name, variable_slot{type, address});
}

void llvm_code_generator::emit_composite_assignment(
    const statement& item, const variable_assignment& assignment)
{
    lvalue_indices indices;
    prepare_lvalue_indices(*assignment.target, indices);
    ir_value value{value_type::void_type, {}};
    std::string target;
    if (assignment.binding)
    {
        target = lvalue_address(*assignment.target, indices);
        const auto current = from_any({value_type::any_type, target},
                                      assignment.target->type, item.position);
        const auto right = expression_value(*assignment.value);
        value = emit_operator_call(assignment.target->type, item.position,
                                   *assignment.binding, current, right);
    }
    else
    {
        value = expression_value(*assignment.value);
        target = lvalue_address(*assignment.target, indices);
        if (assignment.operation == token_kind::plus_equal ||
            assignment.operation == token_kind::minus_equal)
        {
            const bool add = assignment.operation == token_kind::plus_equal;
            const auto current = from_any({value_type::any_type, target},
                                          assignment.target->type, item.position);
            ir_value combined{value_type::void_type, {}};
            if (current.type == value_type::int_type)
            {
                combined = checked_binary(add ? "txrt_add_i64" : "txrt_sub_i64",
                                          current, value,
                                          current.type, item.position);
            }
            else if (current.type == value_type::float_type)
            {
                const auto result = temporary();
                write_instruction(result +
                    (add ? " = fadd double " : " = fsub double ") +
                    current.text + ", " + value.text);
                combined = {current.type, result};
            }
            else if (add && current.type == value_type::str_type)
            {
                combined = checked_binary("txrt_str_concat", current, value,
                                          current.type, item.position);
            }
            else
            {
                throw compile_error(item.position, "LLVM 后端暂不支持此复合赋值类型");
            }
            release(current);
            release(value);
            value = combined;
        }
    }
    const auto boxed = box_any(value, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_value_assign(ptr " +
                      target + ", ptr " + boxed.text + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(boxed);
    release(value);
}

void llvm_code_generator::emit_name_assignment(
    const statement& item, const variable_assignment& assignment,
    const name_reference& name)
{
    const auto variable = find_variable(name.name, item.position);
    ir_value old{value_type::void_type, {}};
    if (assignment.binding)
    {
        old = load(variable);
    }
    auto value = expression_value(*assignment.value);
    if (assignment.binding)
    {
        value = emit_operator_call(variable.type, item.position,
                                   *assignment.binding, old, value);
    }
    else if (assignment.operation == token_kind::plus_equal ||
        assignment.operation == token_kind::minus_equal)
    {
        const bool add = assignment.operation == token_kind::plus_equal;
        const auto old = load(variable);
        if (variable.type == value_type::int_type)
        {
            value = checked_binary(add ? "txrt_add_i64" : "txrt_sub_i64",
                                   old, value,
                                   variable.type, item.position);
        }
        else if (variable.type == value_type::float_type)
        {
            const auto result = temporary();
            write_instruction(result +
                (add ? " = fadd double " : " = fsub double ") +
                old.text + ", " + value.text);
            value = {variable.type, result};
        }
        else if (add && variable.type == value_type::str_type)
        {
            const auto combined = checked_binary("txrt_str_concat", old,
                value, variable.type, item.position);
            release(old);
            release(value);
            value = combined;
        }
        else
        {
            throw compile_error(item.position, "LLVM 后端暂不支持此复合赋值类型");
        }
    }
    release_slot(variable);
    write_instruction("store " + llvm_type(variable.type, item.position) +
                      " " + value.text + ", ptr " + variable.address);
}

void llvm_code_generator::emit_assignment(
    const statement& item, const variable_assignment& assignment)
{
    if (const auto* name = std::get_if<name_reference>(&assignment.target->data))
    {
        emit_name_assignment(item, assignment, *name);
    }
    else
    {
        emit_composite_assignment(item, assignment);
    }
}

void llvm_code_generator::emit_unpack(
    const statement& item, const unpack_assignment& assignment)
{
    const auto source = expression_value(*assignment.value);
    const auto length_status = temporary();
    write_instruction(length_status +
        " = call i32 @txrt_array_require_length(ptr " + source.text +
        ", i64 " + std::to_string(assignment.names.size()) + ")");
    write_instruction("call void @txrt_require_success(i32 " +
                      length_status + ")");
    std::vector<ir_value> values;
    values.reserve(assignment.names.size());
    for (std::size_t index = 0; index < assignment.names.size(); ++index)
    {
        const auto type = assignment.declares[index]
            ? value_type::any_type
            : find_variable(assignment.names[index], item.position).type;
        const auto address = allocate(value_type::any_type, item.position);
        const auto status = temporary();
        write_instruction(status +
            " = call i32 @txrt_array_element_address(ptr " + source.text +
            ", i64 " + std::to_string(index) + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto borrowed = temporary();
        write_instruction(borrowed + " = load ptr, ptr " + address);
        if (type != value_type::any_type)
        {
            const auto checked = temporary();
            write_instruction(checked + " = call i32 @txrt_value_require_type(ptr " +
                              borrowed + ", ptr " + global_bytes(type.name) + ")");
            write_instruction("call void @txrt_require_success(i32 " +
                              checked + ")");
        }
        values.push_back(from_any({value_type::any_type, borrowed},
                                  type, item.position));
    }
    for (std::size_t index = 0; index < assignment.names.size(); ++index)
    {
        if (assignment.declares[index])
        {
            const auto address = allocate(value_type::any_type, item.position);
            write_instruction("store ptr " + values[index].text + ", ptr " + address);
            scopes_.back().emplace(assignment.names[index],
                variable_slot{value_type::any_type, address});
        }
        else
        {
            const auto slot = find_variable(assignment.names[index], item.position);
            release_slot(slot);
            write_instruction("store " + llvm_type(slot.type, item.position) +
                              " " + values[index].text + ", ptr " + slot.address);
        }
    }
    release(source);
}

void llvm_code_generator::emit_return(const statement& item,
                                      const return_statement& result)
{
    ir_value value{value_type::void_type, {}};
    if (result.value)
    {
        value = expression_value(*result.value);
    }
    for (const auto& scope : scopes_)
    {
        for (const auto& [name, variable] : scope)
        {
            (void)name;
            release_slot(variable);
        }
    }
    write_instruction(result.value
        ? "ret " + llvm_type(return_type_, item.position) + " " + value.text
        : "ret void");
    terminated_ = true;
}

void llvm_code_generator::emit_statement(const statement& item)
{
    if (const auto* declaration = std::get_if<variable_declaration>(&item.data))
    {
        emit_declaration(item, *declaration);
    }
    else if (const auto* assignment = std::get_if<variable_assignment>(&item.data))
    {
        emit_assignment(item, *assignment);
    }
    else if (const auto* assignment = std::get_if<unpack_assignment>(&item.data))
    {
        emit_unpack(item, *assignment);
    }
    else if (const auto* branch = std::get_if<if_statement>(&item.data))
    {
        emit_if(*branch);
    }
    else if (const auto* loop = std::get_if<while_statement>(&item.data))
    {
        emit_while(*loop);
    }
    else if (const auto* loop = std::get_if<for_loop>(&item.data))
    {
        emit_for(*loop);
    }
    else if (const auto* loop = std::get_if<for_each>(&item.data))
    {
        emit_for_each(*loop);
    }
    else if (const auto* result = std::get_if<return_statement>(&item.data))
    {
        emit_return(item, *result);
    }
    else
    {
        const auto& expression_only = std::get<expression_statement>(item.data);
        release(expression_value(*expression_only.value));
    }
    if (!terminated_)
    {
        emit_gc_safepoint();
    }
}

} // namespace tx
