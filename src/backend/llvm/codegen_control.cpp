#include "backend/llvm/codegen.hpp"

namespace tx
{
namespace
{

bool uses_name(const expression& item, std::string_view name)
{
    if (const auto* reference = std::get_if<name_reference>(&item.data))
    {
        return reference->name == name;
    }
    if (const auto* index = std::get_if<index_expression>(&item.data))
    {
        return uses_name(*index->object, name) ||
               uses_name(*index->index, name);
    }
    if (const auto* member = std::get_if<member_expression>(&item.data))
    {
        return uses_name(*member->object, name);
    }
    if (const auto* cast = std::get_if<cast_expression>(&item.data))
    {
        return uses_name(*cast->value, name);
    }
    if (const auto* unary = std::get_if<unary_operation>(&item.data))
    {
        return uses_name(*unary->operand, name);
    }
    if (const auto* update = std::get_if<update_expression>(&item.data))
    {
        return uses_name(*update->target, name);
    }
    if (const auto* binary = std::get_if<binary_operation>(&item.data))
    {
        return uses_name(*binary->left, name) ||
               uses_name(*binary->right, name);
    }
    if (const auto* call = std::get_if<call_expression>(&item.data))
    {
        if (call->receiver && uses_name(*call->receiver, name))
        {
            return true;
        }
        for (const auto& argument : call->arguments)
        {
            if (uses_name(*argument.value, name))
            {
                return true;
            }
        }
    }
    if (const auto* array = std::get_if<array_literal>(&item.data))
    {
        for (const auto& element : array->elements)
        {
            if (uses_name(*element, name))
            {
                return true;
            }
        }
    }
    if (const auto* dict = std::get_if<dictionary_literal>(&item.data))
    {
        for (const auto& entry : dict->entries)
        {
            if (uses_name(*entry.key, name) || uses_name(*entry.value, name))
            {
                return true;
            }
        }
    }
    return false;
}

bool uses_name(const std::vector<stmt_ptr>& body, std::string_view name);

bool uses_name(const statement& item, std::string_view name)
{
    if (const auto* declaration =
            std::get_if<variable_declaration>(&item.data))
    {
        return (declaration->initializer &&
                uses_name(*declaration->initializer, name)) ||
               (declaration->array_length &&
                uses_name(*declaration->array_length, name));
    }
    if (const auto* assignment =
            std::get_if<variable_assignment>(&item.data))
    {
        return uses_name(*assignment->target, name) ||
               uses_name(*assignment->value, name);
    }
    if (const auto* unpack = std::get_if<unpack_assignment>(&item.data))
    {
        return uses_name(*unpack->value, name);
    }
    if (const auto* branch = std::get_if<if_statement>(&item.data))
    {
        return uses_name(*branch->condition, name) ||
               uses_name(branch->then_body, name) ||
               uses_name(branch->else_body, name);
    }
    if (const auto* loop = std::get_if<while_statement>(&item.data))
    {
        return uses_name(*loop->condition, name) ||
               uses_name(loop->body, name);
    }
    if (const auto* loop = std::get_if<for_loop>(&item.data))
    {
        return uses_name(*loop->first, name) ||
               uses_name(*loop->last, name) || uses_name(loop->body, name);
    }
    if (const auto* loop = std::get_if<for_each>(&item.data))
    {
        return uses_name(*loop->values, name) || uses_name(loop->body, name);
    }
    if (const auto* guarded = std::get_if<try_statement>(&item.data))
    {
        if (uses_name(guarded->body, name))
        {
            return true;
        }
        for (const auto& handler : guarded->handlers)
        {
            if (uses_name(handler.body, name))
            {
                return true;
            }
        }
        return false;
    }
    if (const auto* result = std::get_if<return_statement>(&item.data))
    {
        return result->value && uses_name(*result->value, name);
    }
    return uses_name(*std::get<expression_statement>(item.data).value, name);
}

bool uses_name(const std::vector<stmt_ptr>& body, std::string_view name)
{
    for (const auto& item : body)
    {
        if (uses_name(*item, name))
        {
            return true;
        }
    }
    return false;
}

} // namespace

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
    refine_integer_condition(*branch.condition, branch.then_body, true);
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
        refine_integer_condition(*branch.condition, branch.else_body, false);
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
        if (loop.body.empty() || !gc_neutral_expression(*loop.condition))
        {
            emit_gc_safepoint();
        }
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
    variable_slot iteration{value_type::int_type, cursor};
    const bool unchanged_cursor = scalar_local_unchanged(loop.name, loop.body);
    if (unchanged_cursor)
    {
        const auto lower = integer_range_of(first).first;
        const auto upper = integer_range_of(last).second;
        if (lower <= upper)
        {
            iteration.integer_range = integer_interval{lower, upper};
        }
    }
    scopes_.back().emplace(loop.name, iteration);
    emit_statements(loop.body);
    pop_scope();
    if (!terminated_)
    {
        if (loop.body.empty())
        {
            emit_gc_safepoint();
        }
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
    ir_value next{value_type::int_type, {}};
    if (unchanged_cursor)
    {
        // 到达此处时 first <= cursor < last，且用户代码不能修改 cursor。
        next.text = temporary();
        write_instruction(next.text + " = add nsw i64 " + before_step + ", 1");
    }
    else
    {
        next = checked_binary("txrt_add_i64",
                                     {value_type::int_type, before_step},
                                     {value_type::int_type, "1"},
                                     value_type::int_type, loop.first->position);
    }
    write_instruction("store i64 " + next.text + ", ptr " + cursor);
    write_instruction("br label %" + check_label);
    start_block(end_label);
}

void llvm_code_generator::emit_for_each(const for_each& loop)
{
    if (loop.values->type.is_vector() || loop.values->type.is_typed_container())
    {
        emit_vector_for_each(loop);
        return;
    }
    if (const auto* name = std::get_if<name_reference>(&loop.values->data))
    {
        const auto array = find_variable(name->name, loop.values->position);
        if (has_local_array(array))
        {
            emit_local_array_for_each(loop, array);
            return;
        }
    }
    push_scope();
    bool borrowed_values = false;
    ir_value values{value_type::void_type, {}};
    if (const auto* name = std::get_if<name_reference>(&loop.values->data);
        name && !uses_name(loop.body, name->name))
    {
        values = expression_value_or_borrow(*loop.values, borrowed_values);
    }
    else
    {
        values = expression_value(*loop.values);
    }
    std::string array_reference;
    if (borrowed_values && values.type == value_type::array_type)
    {
        const auto& name = std::get<name_reference>(loop.values->data);
        const auto variable = find_variable(name.name, loop.values->position);
        if (!variable.array_reference.empty())
        {
            array_reference = load_array_reference(variable);
        }
    }
    const auto values_address = allocate(values.type,
                                         loop.values->position, !borrowed_values);
    write_instruction("store ptr " + values.text + ", ptr " + values_address);
    scopes_.back().emplace("$foreach", variable_slot{
        values.type, values_address, borrowed_values});
    std::string iteration_values = values.text;
    const bool count_only_dictionary = values.type == value_type::dict_type &&
        !uses_name(loop.body, loop.name) && stable_vector_loop(loop.body);
    if (values.type == value_type::dict_type && !count_only_dictionary)
    {
        // 字典无序；进入循环时取得键快照，后续增删不影响本次遍历。
        const auto keys_address = allocate(value_type::array_type,
                                           loop.values->position);
        const auto keys_status = temporary();
        write_instruction(keys_status + " = call i32 @txrt_dictionary_keys(ptr " +
                          values.text + ", ptr " + keys_address + ")");
        write_instruction("call void @txrt_require_success(i32 " +
                          keys_status + ")");
        iteration_values = temporary();
        write_instruction(iteration_values + " = load ptr, ptr " + keys_address);
        scopes_.back().emplace("$dict_keys", variable_slot{
            value_type::array_type, keys_address});
    }
    const auto length_address = allocate(value_type::int_type,
                                         loop.values->position);
    if (!array_reference.empty())
    {
        const auto length = temporary();
        write_instruction(length + " = call i64 @txrt_array_ref_len(ptr " +
                          array_reference + ")");
        write_instruction("store i64 " + length + ", ptr " + length_address);
    }
    else
    {
        const auto length_status = temporary();
        write_instruction(length_status + " = call i32 @" +
                          std::string(count_only_dictionary ? "txrt_dict_len" : "txrt_array_len") + "(ptr " +
                          iteration_values + ", ptr " + length_address + ")");
        write_instruction("call void @txrt_require_success(i32 " +
                          length_status + ")");
    }
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
    const bool needs_element = uses_name(loop.body, loop.name);
    push_scope();
    if (needs_element)
    {
        std::string borrowed;
        if (values.type == value_type::array_type)
        {
            borrowed = temporary();
            write_instruction(borrowed +
                " = call ptr @" + std::string(array_reference.empty()
                    ? "txrt_array_element_read_ptr"
                    : "txrt_array_ref_element_read_ptr") + "(ptr " +
                (array_reference.empty() ? values.text : array_reference) +
                ", i64 " + index.text + ")");
        }
        else
        {
            borrowed = temporary();
            write_instruction(borrowed +
                " = call ptr @txrt_array_element_read_ptr(ptr " +
                iteration_values + ", i64 " + index.text + ")");
        }
        if (!rebinds_name(loop.body, loop.name))
        {
            scopes_.back().emplace(loop.name,
                make_scalar_snapshot(borrowed, loop.values->position));
        }
        else
        {
            const auto element = from_any({value_type::any_type, borrowed},
                                          value_type::any_type,
                                          loop.values->position);
            const auto element_address = allocate(value_type::any_type,
                                                  loop.values->position);
            write_instruction("store ptr " + element.text + ", ptr " +
                              element_address);
            scopes_.back().emplace(loop.name, variable_slot{
                value_type::any_type, element_address});
        }
    }
    emit_statements(loop.body);
    pop_scope();
    if (!terminated_)
    {
        if (loop.body.empty())
        {
            emit_gc_safepoint();
        }
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
