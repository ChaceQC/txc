#include "backend/llvm/codegen.hpp"

namespace tx
{

std::pair<std::string, std::string> llvm_code_generator::local_array_element(
    const variable_slot& array, const std::string& index, bool known_valid)
{
    const auto length = array.local_array_length
        ? std::to_string(*array.local_array_length)
        : array.dynamic_array_length;
    if (!known_valid)
    {
        const auto lower = temporary();
        write_instruction(lower + " = icmp sge i64 " + index + ", 0");
        const auto upper = temporary();
        write_instruction(upper + " = icmp slt i64 " + index + ", " + length);
        const auto valid = temporary();
        write_instruction(valid + " = and i1 " + lower + ", " + upper);
        const auto error_label = label();
        const auto ready_label = label();
        write_instruction("br i1 " + valid + ", label %" + ready_label +
                          ", label %" + error_label);
        start_block(error_label);
        write_instruction("call void @txrt_array_index_error()");
        write_instruction("unreachable");
        start_block(ready_label);
    }
    const auto element = temporary();
    if (array.local_array_length)
    {
        write_instruction(element + " = getelementptr [" + length +
            " x { i8, i64 }], ptr " + array.address +
            ", i64 0, i64 " + index);
    }
    else
    {
        write_instruction(element + " = getelementptr { i8, i64 }, ptr " +
                          array.address + ", i64 " + index);
    }
    const auto kind = temporary();
    write_instruction(kind + " = getelementptr { i8, i64 }, ptr " +
                      element + ", i32 0, i32 0");
    const auto bits = temporary();
    write_instruction(bits + " = getelementptr { i8, i64 }, ptr " +
                      element + ", i32 0, i32 1");
    return {kind, bits};
}

void llvm_code_generator::store_local_scalar(
    const std::string& kind_address, const std::string& bits_address,
    const ir_value& value)
{
    const int kind = value.type == value_type::none_type ? 1
        : value.type == value_type::int_type ? 2
        : value.type == value_type::float_type ? 3 : 4;
    std::string bits = value.text;
    if (value.type == value_type::none_type)
    {
        bits = "0";
    }
    else if (value.type == value_type::float_type)
    {
        bits = temporary();
        write_instruction(bits + " = bitcast double " + value.text + " to i64");
    }
    else if (value.type == value_type::bool_type)
    {
        bits = temporary();
        write_instruction(bits + " = zext i1 " + value.text + " to i64");
    }
    write_instruction("store i8 " + std::to_string(kind) + ", ptr " +
                      kind_address);
    write_instruction("store i64 " + bits + ", ptr " + bits_address);
}

void llvm_code_generator::emit_local_array_declaration(
    const statement& item, const variable_declaration& declaration,
    std::size_t length)
{
    const auto address = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << address << " = alloca [" << length
                 << " x { i8, i64 }]\n";
    variable_slot array{value_type::array_type, address, false,
                        {}, {}, {}, length};
    // 每次执行声明都重置槽位；循环中的上次值不能泄漏到本次。
    for (std::size_t index = 0; index < length; ++index)
    {
        const auto [kind, bits] = local_array_element(
            array, std::to_string(index), true);
        store_local_scalar(kind, bits, {value_type::none_type, "0"});
    }
    if (declaration.initializer)
    {
        const auto& literal = std::get<array_literal>(
            declaration.initializer->data);
        for (std::size_t index = 0; index < literal.elements.size(); ++index)
        {
            const auto& element = *literal.elements[index];
            const bool literal_none =
                std::holds_alternative<none_literal>(element.data);
            const auto value = literal_none
                ? ir_value{value_type::none_type, "0"}
                : expression_value(element);
            const auto [kind, bits] = local_array_element(
                array, std::to_string(index), true);
            store_local_scalar(kind, bits, value);
            if (element.type == value_type::none_type && !literal_none)
            {
                release(value);
            }
        }
    }
    scopes_.back().emplace(declaration.name, std::move(array));
    (void)item;
}

bool llvm_code_generator::emit_local_array_assignment(
    const statement& item, const variable_assignment& assignment)
{
    const auto* index = std::get_if<index_expression>(&assignment.target->data);
    if (!index)
    {
        return false;
    }
    const auto* name = std::get_if<name_reference>(&index->object->data);
    if (!name)
    {
        return false;
    }
    const auto array = find_variable(name->name, item.position);
    if (!has_local_array(array))
    {
        return false;
    }
    const auto position = expression_value(*index->index);
    const auto& right = *assignment.value;
    const bool literal_none = std::holds_alternative<none_literal>(right.data);
    const auto value = literal_none
        ? ir_value{value_type::none_type, "0"} : expression_value(right);
    const auto [kind, bits] = local_array_element(array, position.text, false);
    store_local_scalar(kind, bits, value);
    release(position);
    if (right.type == value_type::none_type && !literal_none)
    {
        release(value);
    }
    return true;
}

llvm_code_generator::variable_slot
llvm_code_generator::make_local_scalar_snapshot(
    const std::string& kind_address, const std::string& bits_address,
    source_pos position)
{
    const auto fallback = allocate(value_type::any_type, position);
    const auto bits = allocate(value_type::int_type, position);
    const auto kind = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << kind << " = alloca i8\n";
    const auto kind_value = temporary();
    write_instruction(kind_value + " = load i8, ptr " + kind_address);
    const auto bits_value = temporary();
    write_instruction(bits_value + " = load i64, ptr " + bits_address);
    write_instruction("store i8 " + kind_value + ", ptr " + kind);
    write_instruction("store i64 " + bits_value + ", ptr " + bits);
    write_instruction("store ptr null, ptr " + fallback);
    return {value_type::any_type, fallback, false, {}, kind, bits};
}

void llvm_code_generator::emit_local_array_unpack(
    const statement& item, const unpack_assignment& assignment,
    const variable_slot& array)
{
    for (std::size_t index = 0; index < assignment.names.size(); ++index)
    {
        const auto [kind, bits] = local_array_element(
            array, std::to_string(index), true);
        auto snapshot = make_local_scalar_snapshot(kind, bits, item.position);
        if (rebinds_name(*current_function_body_, assignment.names[index]))
        {
            const auto boxed = load(snapshot);
            const auto address = allocate(value_type::any_type, item.position);
            write_instruction("store ptr " + boxed.text + ", ptr " + address);
            snapshot = {value_type::any_type, address};
        }
        scopes_.back().emplace(assignment.names[index], std::move(snapshot));
    }
}

llvm_code_generator::ir_value llvm_code_generator::cast_local_array_element(
    const index_expression& index, const value_type& target,
    source_pos position)
{
    const auto& name = std::get<name_reference>(index.object->data);
    const auto array = find_variable(name.name, position);
    const auto element_index = expression_value(*index.index);
    const auto [kind, bits] = local_array_element(
        array, element_index.text, false);
    release(element_index);
    const auto fallback = allocate(value_type::any_type, position);
    write_instruction("store ptr null, ptr " + fallback);
    return cast_scalar_snapshot({value_type::any_type, fallback, false,
                                 {}, kind, bits}, target, position);
}

void llvm_code_generator::emit_local_array_for_each(
    const for_each& loop, const variable_slot& array)
{
    push_scope();
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
    write_instruction(in_range + " = icmp slt i64 " + index.text + ", " +
                      (array.local_array_length
                          ? std::to_string(*array.local_array_length)
                          : array.dynamic_array_length));
    write_instruction("br i1 " + in_range + ", label %" + body_label +
                      ", label %" + end_label);
    start_block(body_label);
    const auto [kind, bits] = local_array_element(array, index.text, true);
    push_scope();
    auto snapshot = make_local_scalar_snapshot(kind, bits,
                                               loop.values->position);
    if (rebinds_name(loop.body, loop.name))
    {
        const auto boxed = load(snapshot);
        const auto address = allocate(value_type::any_type,
                                      loop.values->position);
        write_instruction("store ptr " + boxed.text + ", ptr " + address);
        snapshot = {value_type::any_type, address};
    }
    scopes_.back().emplace(loop.name, std::move(snapshot));
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
