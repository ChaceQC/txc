#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::emit_array_elements(const array_literal& literal,
                                               const std::string& array,
                                               source_pos position)
{
    const auto reference = temporary();
    write_instruction(reference + " = call ptr @txrt_array_ref(ptr " +
                      array + ")");
    for (std::size_t index = 0; index < literal.elements.size(); ++index)
    {
        const auto element = expression_value(*literal.elements[index]);
        const auto* direct = element.type == value_type::int_type
            ? "txrt_array_ref_set_i64" : element.type == value_type::float_type
            ? "txrt_array_ref_set_f64" : element.type == value_type::bool_type
            ? "txrt_array_ref_set_bool" : nullptr;
        if (direct)
        {
            const auto status = temporary();
            write_instruction(status + " = call i32 @" + direct +
                "(ptr " + reference + ", i64 " + std::to_string(index) +
                ", " + llvm_type(element.type, position) + " " +
                element.text + ")");
            write_instruction("call void @txrt_require_success(i32 " +
                              status + ")");
            continue;
        }
        const auto target = temporary();
        write_instruction(target +
            " = call ptr @txrt_array_ref_element_ptr(ptr " + reference +
            ", i64 " + std::to_string(index) + ")");
        assign_any(target, element, position);
        release(element);
    }
}

llvm_code_generator::ir_value llvm_code_generator::box_any(
    const ir_value& value, source_pos position)
{
    const auto address = allocate(value_type::any_type, position);
    std::string name;
    if (value.type == value_type::int_type)
    {
        name = "txrt_value_box_i64";
    }
    else if (value.type == value_type::float_type)
    {
        name = "txrt_value_box_f64";
    }
    else if (value.type == value_type::bool_type)
    {
        name = "txrt_value_box_bool";
    }
    else if (value.type == value_type::str_type)
    {
        name = "txrt_value_box_str";
    }
    else if (is_value_handle(value.type))
    {
        name = "txrt_value_clone";
    }
    else
    {
        throw compile_error(position, "LLVM 后端不能装箱此类型");
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + name + "(" +
                      llvm_type(value.type, position) + " " + value.text +
                      ", ptr " + address + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + address);
    return {value_type::any_type, result};
}

llvm_code_generator::ir_value llvm_code_generator::from_any(
    const ir_value& value, const value_type& target, source_pos position)
{
    if (is_value_handle(target))
    {
        const auto address = allocate(target, position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_value_clone(ptr " +
                          value.text + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        return {target, result};
    }
    const auto* name = target == value_type::int_type ? "txrt_value_to_i64"
        : target == value_type::float_type ? "txrt_value_to_f64"
        : target == value_type::bool_type ? "txrt_value_to_bool"
        : target == value_type::str_type ? "txrt_value_to_str" : nullptr;
    if (!name)
    {
        throw compile_error(position, "LLVM 后端暂不支持此动态类型转换");
    }
    if (target == value_type::int_type ||
        target == value_type::float_type ||
        target == value_type::bool_type)
    {
        const auto* direct = target == value_type::int_type
            ? "txrt_value_to_i64_fast" : target == value_type::float_type
            ? "txrt_value_to_f64_fast" : "txrt_value_to_bool_fast";
        const auto result = temporary();
        write_instruction(result + " = call " + llvm_type(target, position) +
                          " @" + direct + "(ptr " + value.text + ")");
        return {target, result};
    }
    const auto address = allocate(target, position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + name + "(ptr " +
                      value.text + ", ptr " + address + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto result = temporary();
    write_instruction(result + " = load " + llvm_type(target, position) +
                      ", ptr " + address);
    return {target, result};
}

void llvm_code_generator::assign_any(const std::string& target,
                                     const ir_value& value,
                                     source_pos position)
{
    const auto* direct = value.type == value_type::int_type
        ? "txrt_value_set_i64" : value.type == value_type::float_type
        ? "txrt_value_set_f64" : value.type == value_type::bool_type
        ? "txrt_value_set_bool" : nullptr;
    ir_value boxed{value_type::void_type, {}};
    if (!direct && !is_value_handle(value.type))
    {
        boxed = box_any(value, position);
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @" +
        std::string(direct ? direct : "txrt_value_assign") + "(ptr " +
        target + ", " + (direct ? llvm_type(value.type, position) + " " +
        value.text : "ptr " + (is_value_handle(value.type)
            ? value.text : boxed.text)) + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(boxed);
}

void llvm_code_generator::prepare_lvalue_indices(
    const expression& item, lvalue_indices& indices)
{
    if (const auto* member = std::get_if<member_expression>(&item.data))
    {
        prepare_lvalue_indices(*member->object, indices);
    }
    else if (const auto* index = std::get_if<index_expression>(&item.data))
    {
        prepare_lvalue_indices(*index->object, indices);
        // 所有可能产生副作用的索引先各求值一次，再取得容器内部地址。
        if (index->object->type != value_type::dict_type ||
            !std::holds_alternative<string_literal>(index->index->data))
        {
            indices.emplace(&item, expression_value(*index->index));
        }
    }
}

std::string llvm_code_generator::lvalue_address(
    const expression& item, const lvalue_indices& indices)
{
    if (const auto* name = std::get_if<name_reference>(&item.data))
    {
        const auto variable = find_variable(name->name, item.position);
        if (!is_value_handle(variable.type))
        {
            throw compile_error(item.position, "字段或索引需要复合类型变量");
        }
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + variable.address);
        return result;
    }
    std::string base;
    std::string invocation;
    if (const auto* member = std::get_if<member_expression>(&item.data))
    {
        base = lvalue_address(*member->object, indices);
        if (member->object->type == value_type::any_type)
        {
            invocation = "@txrt_struct_field_address(ptr " + base + ", ptr " +
                         global_bytes(member->field);
        }
        else
        {
            if (classes_.contains(member->object->type.name))
            {
                if (!member->field_slot)
                {
                    throw compile_error(item.position, "类字段缺少静态槽位");
                }
                invocation = "@txrt_class_field_address_index(ptr " + base +
                    ", i64 " + std::to_string(*member->field_slot) +
                    ", i1 true";
            }
            else
            {
                invocation = "@txrt_struct_field_address_index(ptr " + base +
                    ", i64 " + std::to_string(field_index(
                        member->object->type, member->field, item.position));
            }
        }
    }
    else if (const auto* index = std::get_if<index_expression>(&item.data))
    {
        bool native_array = false;
        if (const auto* name = std::get_if<name_reference>(&index->object->data);
            name && index->object->type == value_type::array_type)
        {
            const auto variable = find_variable(name->name, item.position);
            native_array = !variable.array_reference.empty();
            if (native_array)
            {
                base = load_array_reference(variable);
            }
        }
        if (!native_array)
        {
            base = lvalue_address(*index->object, indices);
        }
        const auto* literal_key = index->object->type == value_type::dict_type
            ? std::get_if<string_literal>(&index->index->data) : nullptr;
        ir_value position{value_type::void_type, {}};
        if (!literal_key)
        {
            position = indices.at(&item);
        }
        const bool static_array = index->object->type == value_type::array_type;
        const bool string_key = index->object->type == value_type::dict_type &&
                                position.type == value_type::str_type;
        if (static_array)
        {
            const auto result = temporary();
            write_instruction(result +
                " = call ptr @" + std::string(native_array
                    ? "txrt_array_ref_element_ptr"
                    : "txrt_array_element_ptr") + "(ptr " + base +
                ", i64 " + position.text + ")");
            release(position);
            return result;
        }
        ir_value boxed{value_type::void_type, {}};
        if (literal_key)
        {
            const auto decoded = decode_string_literal(literal_key->text);
            invocation = "@txrt_dict_element_address_literal(ptr " + base +
                ", ptr " + global_bytes(decoded) + ", i64 " +
                std::to_string(decoded.size()) + ", i1 true";
        }
        else if (string_key)
        {
            invocation = "@txrt_dict_element_address_str(ptr " + base +
                         ", ptr " + position.text + ", i1 true";
        }
        else
        {
            boxed = box_any(position, item.position);
            const auto* function = index->object->type == value_type::dict_type
                ? "txrt_dict_element_address" : "txrt_value_element_address";
            invocation = "@" + std::string(function) + "(ptr " + base +
                         ", ptr " + boxed.text + ", i1 true";
        }
        const auto address = allocate(value_type::any_type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 " + invocation +
                          ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        if (!string_key && !literal_key)
        {
            release(boxed);
        }
        if (!literal_key)
        {
            release(position);
        }
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        return result;
    }
    else
    {
        throw compile_error(item.position, "赋值左侧必须是变量、字段或数组元素");
    }
    const auto address = allocate(value_type::any_type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 " + invocation +
                      ", ptr " + address + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + address);
    return result;
}

llvm_code_generator::ir_value llvm_code_generator::emit_update(
    const expression& item, const update_expression& operation)
{
    if (direct_scalar_field(*operation.target))
    {
        const auto address = scalar_field_address(*operation.target);
        const auto current = load({item.type, address});
        const bool increment = operation.operation == token_kind::plus_plus;
        ir_value updated{item.type, {}};
        if (item.type == value_type::int_type)
        {
            updated = checked_binary(increment ? "txrt_add_i64" : "txrt_sub_i64",
                                     current, {item.type, "1"}, item.type,
                                     item.position);
        }
        else
        {
            updated.text = temporary();
            write_instruction(updated.text +
                (increment ? " = fadd double " : " = fsub double ") +
                current.text + ", 0x3ff0000000000000");
        }
        write_instruction("store " + llvm_type(item.type, item.position) +
                          " " + updated.text + ", ptr " + address);
        return updated;
    }
    const auto* name = std::get_if<name_reference>(&operation.target->data);
    lvalue_indices indices;
    if (!name)
    {
        prepare_lvalue_indices(*operation.target, indices);
    }
    const auto address = name
        ? find_variable(name->name, item.position).address
        : lvalue_address(*operation.target, indices);
    const auto current = name
        ? load({item.type, address})
        : from_any({value_type::any_type, address}, item.type, item.position);
    const ir_value one{item.type, item.type == value_type::int_type
        ? "1" : "0x3ff0000000000000"};
    const bool increment = operation.operation == token_kind::plus_plus;
    ir_value updated{item.type, {}};
    if (item.type == value_type::int_type)
    {
        updated = checked_binary(increment ? "txrt_add_i64" : "txrt_sub_i64",
                                 current, one, item.type, item.position);
    }
    else
    {
        updated.text = temporary();
        write_instruction(updated.text +
            (increment ? " = fadd double " : " = fsub double ") +
            current.text + ", " + one.text);
    }
    release(current);

    // 后置写法按先更新、再返回新值处理；字段目标的地址仅计算一次。
    if (name)
    {
        write_instruction("store " + llvm_type(item.type, item.position) +
                          " " + updated.text + ", ptr " + address);
    }
    else
    {
        assign_any(address, updated, item.position);
    }
    return updated;
}

} // namespace tx
