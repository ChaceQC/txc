#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::emit_if(const if_statement& branch)
{
    const auto condition = expression_value(*branch.condition);
    const auto then_label = label();
    const auto end_label = label();
    const auto else_label = branch.has_else ? label() : end_label;
    write_instruction("br i1 " + condition.text + ", label %" + then_label +
                      ", label %" + else_label);
    terminated_ = true;

    start_block(then_label);
    push_scope();
    emit_statements(branch.then_body);
    pop_scope();
    if (!terminated_)
    {
        write_instruction("br label %" + end_label);
    }

    if (branch.has_else)
    {
        start_block(else_label);
        push_scope();
        emit_statements(branch.else_body);
        pop_scope();
        if (!terminated_)
        {
            write_instruction("br label %" + end_label);
        }
    }
    start_block(end_label);
}

void llvm_code_generator::emit_while(const while_statement& loop)
{
    const auto check_label = label();
    const auto body_label = label();
    const auto end_label = label();
    write_instruction("br label %" + check_label);
    start_block(check_label);
    const auto condition = expression_value(*loop.condition);
    write_instruction("br i1 " + condition.text + ", label %" + body_label +
                      ", label %" + end_label);
    start_block(body_label);
    push_scope();
    emit_statements(loop.body);
    pop_scope();
    if (!terminated_)
    {
        write_instruction("br label %" + check_label);
    }
    start_block(end_label);
}

void llvm_code_generator::emit_for(const for_loop& loop)
{
    const auto first = expression_value(*loop.first);
    const auto last = expression_value(*loop.last);
    const auto cursor = allocate(value_type::int_type, loop.first->position);
    write_instruction("store i64 " + first.text + ", ptr " + cursor);

    const auto check_label = label();
    const auto body_label = label();
    const auto increment_label = label();
    const auto step_label = label();
    const auto end_label = label();
    write_instruction("br label %" + check_label);
    start_block(check_label);
    const auto current = temporary();
    write_instruction(current + " = load i64, ptr " + cursor);
    const auto in_range = temporary();
    write_instruction(in_range + " = icmp sle i64 " + current + ", " + last.text);
    write_instruction("br i1 " + in_range + ", label %" + body_label +
                      ", label %" + end_label);

    start_block(body_label);
    push_scope();
    scopes_.back().emplace(loop.name, variable_slot{value_type::int_type, cursor});
    emit_statements(loop.body);
    pop_scope();
    if (!terminated_)
    {
        write_instruction("br label %" + increment_label);
    }

    start_block(increment_label);
    const auto before_step = temporary();
    write_instruction(before_step + " = load i64, ptr " + cursor);
    const auto at_end = temporary();
    write_instruction(at_end + " = icmp eq i64 " + before_step + ", " + last.text);
    write_instruction("br i1 " + at_end + ", label %" + end_label +
                      ", label %" + step_label);
    start_block(step_label);
    const auto next = checked_binary("txrt_add_i64",
                                     {value_type::int_type, before_step},
                                     {value_type::int_type, "1"},
                                     value_type::int_type, loop.first->position);
    write_instruction("store i64 " + next.text + ", ptr " + cursor);
    write_instruction("br label %" + check_label);
    start_block(end_label);
}

void llvm_code_generator::emit_for_each(const for_each& loop)
{
    push_scope();
    const auto values = expression_value(*loop.values);
    const auto values_address = allocate(value_type::array_type,
                                         loop.values->position);
    write_instruction("store ptr " + values.text + ", ptr " + values_address);
    scopes_.back().emplace("$foreach", variable_slot{
        value_type::array_type, values_address});
    const auto length_address = allocate(value_type::int_type,
                                         loop.values->position);
    const auto length_status = temporary();
    write_instruction(length_status + " = call i32 @txrt_array_len(ptr " +
                      values.text + ", ptr " + length_address + ")");
    write_instruction("call void @txrt_require_success(i32 " +
                      length_status + ")");
    const auto length = load({value_type::int_type, length_address});
    const auto index_address = allocate(value_type::int_type,
                                        loop.values->position);
    write_instruction("store i64 0, ptr " + index_address);

    const auto check_label = label();
    const auto body_label = label();
    const auto end_label = label();
    write_instruction("br label %" + check_label);
    start_block(check_label);
    const auto index = load({value_type::int_type, index_address});
    const auto in_range = temporary();
    write_instruction(in_range + " = icmp slt i64 " + index.text +
                      ", " + length.text);
    write_instruction("br i1 " + in_range + ", label %" + body_label +
                      ", label %" + end_label);

    start_block(body_label);
    const auto field = allocate(value_type::any_type, loop.values->position);
    const auto field_status = temporary();
    write_instruction(field_status +
        " = call i32 @txrt_array_element_address(ptr " + values.text +
        ", i64 " + index.text + ", ptr " + field + ")");
    write_instruction("call void @txrt_require_success(i32 " + field_status + ")");
    const auto borrowed = temporary();
    write_instruction(borrowed + " = load ptr, ptr " + field);
    const auto element = from_any({value_type::any_type, borrowed},
                                  value_type::any_type, loop.values->position);
    const auto element_address = allocate(value_type::any_type,
                                          loop.values->position);
    write_instruction("store ptr " + element.text + ", ptr " + element_address);
    push_scope();
    scopes_.back().emplace(loop.name, variable_slot{
        value_type::any_type, element_address});
    emit_statements(loop.body);
    pop_scope();
    if (!terminated_)
    {
        const auto next = checked_binary("txrt_add_i64", index,
            {value_type::int_type, "1"}, value_type::int_type,
            loop.values->position);
        write_instruction("store i64 " + next.text + ", ptr " + index_address);
        write_instruction("br label %" + check_label);
    }
    start_block(end_label);
    pop_scope();
}

} // namespace tx
