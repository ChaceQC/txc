#include "backend/llvm/codegen.hpp"

namespace tx
{

llvm_code_generator::ir_value llvm_code_generator::emit_map_index(
    const index_expression& access, source_pos position)
{
    const auto map = expression_value(*access.object);
    const auto key = expression_value(*access.index);
    const auto result = container_operation(map.type, "read", {map, key},
        map.type.parameters[map.type.is_map() ? 1 : 0], position);
    release(key);
    release(map);
    return result;
}

llvm_code_generator::ir_value llvm_code_generator::map_update_value(
    const ir_value& current, const ir_value& value, bool add, source_pos position)
{
    if (current.type == value_type::float_type)
    {
        const auto result = temporary();
        write_instruction(result + (add ? " = fadd double " : " = fsub double ") +
                          current.text + ", " + value.text);
        return {current.type, result};
    }
    return checked_binary(current.type == value_type::str_type
        ? "txrt_str_concat" : add ? "txrt_add_i64" : "txrt_sub_i64",
        current, value, current.type, position);
}

void llvm_code_generator::emit_map_assignment(
    const statement& item, const variable_assignment& assignment)
{
    const auto& access = std::get<index_expression>(assignment.target->data);
    const auto map = expression_value(*access.object);
    const auto key = expression_value(*access.index);
    auto value = expression_value(*assignment.value);
    if (assignment.operation != token_kind::equal)
    {
        // 不保留跨右侧求值的哈希槽位地址；右侧可能插入、清空或触发 rehash。
        const auto current = container_operation(map.type, "read", {map, key},
                                                 assignment.target->type, item.position);
        const auto combined = map_update_value(current, value,
            assignment.operation == token_kind::plus_equal, item.position);
        release(current);
        release(value);
        value = combined;
    }
    (void)container_operation(map.type, "set", {map, key, value},
                              value_type::void_type, item.position);
    release(value);
    release(key);
    release(map);
}

llvm_code_generator::ir_value llvm_code_generator::emit_map_update(
    const expression& item, const update_expression& update)
{
    const auto& access = std::get<index_expression>(update.target->data);
    const auto map = expression_value(*access.object);
    const auto key = expression_value(*access.index);
    const auto current = container_operation(map.type, "read", {map, key}, item.type, item.position);
    const auto result = map_update_value(current,
        {item.type, item.type == value_type::float_type ? "1.0" : "1"},
        update.operation == token_kind::plus_plus, item.position);
    (void)container_operation(map.type, "set", {map, key, result},
                              value_type::void_type, item.position);
    release(current);
    release(key);
    release(map);
    return result;
}

} // namespace tx
