#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::emit_vector_assignment(
    const statement& item, const variable_assignment& assignment)
{
    const auto& access = std::get<index_expression>(assignment.target->data);
    bool borrowed = false;
    const auto vector = container_value(*access.object,
        stable_value_expression(*access.index) &&
        stable_value_expression(*assignment.value), borrowed);
    const auto index = expression_value(*access.index);
    if (const auto* source = std::get_if<index_expression>(&assignment.value->data);
        assignment.operation == token_kind::equal && source &&
        source->object->type == value_type::vector_of(value_type::str_type))
    {
        // 两边容器保持存活；取出文本后不再运行用户代码，直接复制内部引用。
        bool source_borrowed = false;
        const auto source_vector = container_value(*source->object,
            stable_value_expression(*source->index), source_borrowed);
        const auto source_index = expression_value(*source->index);
        const auto source_slot = vector_slot(source_vector, source_index, item.position);
        const auto text = temporary();
        write_instruction(text + " = load ptr, ptr " + source_slot);
        vector_write(vector, index, {value_type::str_type, text}, item.position);
        if (!source_borrowed)
        {
            release(source_vector);
        }
        if (!borrowed)
        {
            release(vector);
        }
        return;
    }
    auto value = expression_value(*assignment.value);
    if (assignment.operation != token_kind::equal)
    {
        const auto current = vector_read(vector, index, item.position);
        const bool add = assignment.operation == token_kind::plus_equal;
        ir_value combined{current.type, {}};
        if (current.type == value_type::float_type)
        {
            combined.text = temporary();
            write_instruction(combined.text + (add ? " = fadd double " : " = fsub double ") +
                              current.text + ", " + value.text);
        }
        else
        {
            combined = checked_binary(current.type == value_type::str_type
                ? "txrt_str_concat" : add ? "txrt_add_i64" : "txrt_sub_i64",
                current, value, current.type, item.position);
        }
        release(current);
        release(value);
        value = combined;
    }
    vector_write(vector, index, value, item.position);
    release(value);
    if (!borrowed)
    {
        release(vector);
    }
}

llvm_code_generator::ir_value llvm_code_generator::emit_vector_update(
    const expression& item, const update_expression& update)
{
    const auto& access = std::get<index_expression>(update.target->data);
    bool borrowed = false;
    const auto vector = container_value(*access.object,
        stable_value_expression(*access.index), borrowed);
    const auto index = expression_value(*access.index);
    const auto current = vector_read(vector, index, item.position);
    const bool add = update.operation == token_kind::plus_plus;
    ir_value result{item.type, {}};
    if (item.type == value_type::float_type)
    {
        result.text = temporary();
        write_instruction(result.text + (add ? " = fadd double " : " = fsub double ") +
                          current.text + ", 1.0");
    }
    else
    {
        result = checked_binary(add ? "txrt_add_i64" : "txrt_sub_i64", current,
            {item.type, "1"}, item.type, item.position);
    }
    vector_write(vector, index, result, item.position);
    if (!borrowed)
    {
        release(vector);
    }
    return result;
}

void llvm_code_generator::emit_vector_for_each(const for_each& loop)
{
    push_scope();
    auto vector = expression_value(*loop.values);
    if (vector.type.is_typed_container())
    {
        const auto source = vector;
        vector = container_operation(source.type, source.type.is_map() ? "keys" : "to_vector",
            {source}, value_type::vector_of(source.type.parameters.front()), loop.values->position);
        release(source);
    }
    const auto owner = allocate(vector.type, loop.values->position);
    write_instruction("store ptr " + vector.text + ", ptr " + owner);
    scopes_.back().emplace("$vector", variable_slot{vector.type, owner});
    const auto length = vector_length(vector, false);
    const auto counter = allocate(value_type::int_type, loop.values->position);
    write_instruction("store i64 0, ptr " + counter);
    const auto check = label();
    const auto body = label();
    const auto end = label();
    write_instruction("br label %" + check);
    start_block(check);
    const auto index = load({value_type::int_type, counter});
    const auto valid = temporary();
    write_instruction(valid + " = icmp slt i64 " + index.text + ", " + length.text);
    write_instruction("br i1 " + valid + ", label %" + body + ", label %" + end);
    start_block(body);
    push_scope();
    const auto element = vector_read(vector, index, loop.values->position);
    const auto local = allocate(element.type, loop.values->position);
    write_instruction("store " + llvm_type(element.type, loop.values->position) + " " +
                      element.text + ", ptr " + local);
    scopes_.back().emplace(loop.name, variable_slot{element.type, local});
    emit_statements(loop.body);
    pop_scope();
    if (!terminated_)
    {
        const auto next = temporary();
        write_instruction(next + " = add i64 " + index.text + ", 1");
        write_instruction("store i64 " + next + ", ptr " + counter);
        write_instruction("br label %" + check);
    }
    start_block(end);
    pop_scope();
}

} // namespace tx
