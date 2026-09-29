#include "backend/llvm/codegen.hpp"

namespace tx
{

llvm_code_generator::ir_value llvm_code_generator::emit_cast(
    const expression& item, const cast_expression& cast)
{
    if (cast.value->type == value_type::any_type &&
        (cast.target == value_type::int_type ||
         cast.target == value_type::float_type ||
         cast.target == value_type::bool_type ||
         cast.target == value_type::str_type))
    {
        if (const auto* name = std::get_if<name_reference>(&cast.value->data))
        {
            const auto variable = find_variable(name->name,
                                                cast.value->position);
            if (!variable.snapshot_kind.empty())
            {
                if (cast.target == value_type::int_type ||
                    cast.target == value_type::float_type ||
                    cast.target == value_type::bool_type)
                {
                    return cast_scalar_snapshot(variable, cast.target,
                                                item.position);
                }
                const auto boxed = load(variable);
                const auto result = from_any(boxed, cast.target, item.position);
                release(boxed);
                return result;
            }
            const auto borrowed = temporary();
            write_instruction(borrowed + " = load ptr, ptr " +
                              variable.address);
            return from_any({value_type::any_type, borrowed}, cast.target,
                            item.position);
        }
        if (const auto* index = std::get_if<index_expression>(&cast.value->data);
            index && index->object->type == value_type::array_type)
        {
            const auto* member =
                std::get_if<member_expression>(&index->object->data);
            if (std::holds_alternative<name_reference>(index->object->data) ||
                (member && std::holds_alternative<name_reference>(
                    member->object->data) &&
                 (std::holds_alternative<integer_literal>(index->index->data) ||
                  std::holds_alternative<name_reference>(index->index->data))))
            {
                return cast_array_element(*index, cast.target, item.position);
            }
        }
        if (const auto* index = std::get_if<index_expression>(&cast.value->data);
            index && index->object->type == value_type::dict_type &&
            (cast.target == value_type::int_type ||
             cast.target == value_type::float_type ||
             cast.target == value_type::str_type))
        {
            if (const auto* name = std::get_if<name_reference>(
                    &index->object->data))
            {
                const auto variable = find_variable(name->name,
                                                    index->object->position);
                const auto& key = index->index->data;
                const bool simple_key =
                    std::holds_alternative<name_reference>(key) ||
                    std::holds_alternative<string_literal>(key) ||
                    std::holds_alternative<integer_literal>(key) ||
                    std::holds_alternative<floating_literal>(key) ||
                    std::holds_alternative<boolean_literal>(key) ||
                    std::holds_alternative<none_literal>(key);
                if (!variable.dict_reference.empty() && simple_key)
                {
                    return cast_dict_element(*index, cast.target, item.position);
                }
            }
        }
    }
    const auto value = expression_value(*cast.value);
    if (classes_.contains(cast.target.name))
    {
        if (!class_is_assignable(value.type, cast.target))
        {
            write_instruction(std::string("call void @") +
                              (static_record_type(cast.target) ? "txrt_record_require_type" :
                               "txrt_class_require_type_fast") + "(ptr " +
                              value.text + ", ptr " +
                              (static_record_type(cast.target) ? "@tx_record_" + cast.target.name :
                               global_bytes(cast.target.name)) + ")");
        }
        return {cast.target, value.text};
    }
    if (value.type == cast.target)
    {
        return value;
    }
    if (value.type == value_type::any_type)
    {
        const auto result = from_any(value, cast.target, item.position);
        release(value);
        return result;
    }
    if (value.type == value_type::int_type &&
        cast.target == value_type::float_type)
    {
        const auto result = temporary();
        write_instruction(result + " = sitofp i64 " + value.text + " to double");
        return {item.type, result};
    }
    if (value.type == value_type::float_type &&
        cast.target == value_type::int_type)
    {
        // fptosi 仅在有界有限值上有定义；错误路径沿用运行时原有诊断。
        const auto lower = temporary();
        write_instruction(lower + " = fcmp oge double " + value.text +
                          ", 0xc3e0000000000000");
        const auto upper = temporary();
        write_instruction(upper + " = fcmp olt double " + value.text +
                          ", 0x43e0000000000000");
        const auto valid = temporary();
        write_instruction(valid + " = and i1 " + lower + ", " + upper);
        const auto success_label = label();
        const auto error_label = label();
        write_instruction("br i1 " + valid + ", label %" + success_label +
                          ", label %" + error_label);
        start_block(error_label);
        const auto address = allocate(value_type::int_type, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_float_to_int(double " +
                          value.text + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        write_instruction("unreachable");
        start_block(success_label);
        const auto result = temporary();
        write_instruction(result + " = fptosi double " + value.text +
                          " to i64");
        return {value_type::int_type, result};
    }
    const auto* conversion = value.type == value_type::str_type &&
                              cast.target == value_type::int_type
        ? "txrt_parse_int" : value.type == value_type::str_type &&
                             cast.target == value_type::float_type
        ? "txrt_parse_float" : value.type == value_type::int_type &&
                               cast.target == value_type::str_type
        ? "txrt_int_to_str" : value.type == value_type::float_type &&
                               cast.target == value_type::str_type
        ? "txrt_float_to_str" : value.type == value_type::bool_type &&
                                cast.target == value_type::str_type
        ? "txrt_bool_to_str" : nullptr;
    if (conversion)
    {
        const auto address = allocate(cast.target, item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @" + conversion + "(" +
                          llvm_type(value.type, item.position) + " " +
                          value.text + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        release(value);
        if (cast.target == value_type::str_type)
        {
            const auto result = temporary();
            write_instruction(result + " = load ptr, ptr " + address);
            return {cast.target, result};
        }
        return load({cast.target, address});
    }
    throw compile_error(item.position, "LLVM 后端暂不支持此类型转换");
}

llvm_code_generator::ir_value llvm_code_generator::cast_array_element(
    const index_expression& index, const value_type& target,
    source_pos position)
{
    std::string array;
    bool native_array = false;
    if (const auto* name = std::get_if<name_reference>(&index.object->data))
    {
        const auto variable = find_variable(name->name, index.object->position);
        if (has_local_array(variable))
        {
            return cast_local_array_element(index, target, position);
        }
        native_array = !variable.array_reference.empty();
        array = native_array ? load_array_reference(variable) : temporary();
        if (!native_array)
        {
            write_instruction(array + " = load ptr, ptr " + variable.address);
        }
    }
    else
    {
        const auto& member = std::get<member_expression>(index.object->data);
        const auto& member_name =
            std::get<name_reference>(member.object->data);
        const auto variable = find_variable(member_name.name,
                                            member.object->position);
        const auto object = temporary();
        write_instruction(object + " = load ptr, ptr " + variable.address);
        const bool class_field = classes_.contains(member.object->type.name);
        if (class_field && !member.field_slot)
        {
            throw compile_error(position, "类字段缺少静态槽位");
        }
        const auto field = allocate(value_type::any_type, position, false);
        const auto status = temporary();
        write_instruction(status + " = call i32 @" +
            std::string(class_field ? "txrt_class_field_address_index"
                                    : "txrt_struct_field_address_index") +
            "(ptr " + object + ", i64 " +
            std::to_string(class_field ? *member.field_slot
                : field_index(member.object->type, member.field, position)) +
            (class_field ? ", i1 false" : "") + ", ptr " + field + ")");
        write_instruction("call void @txrt_require_success(i32 " + status +
                          ")");
        array = temporary();
        write_instruction(array + " = load ptr, ptr " + field);
    }
    const auto element_index = expression_value(*index.index);
    if (native_array &&
        (target == value_type::int_type ||
         target == value_type::float_type ||
         target == value_type::str_type))
    {
        const auto* function = target == value_type::int_type
            ? "txrt_array_ref_get_i64"
            : target == value_type::float_type
            ? "txrt_array_ref_get_f64" : "txrt_array_ref_get_str";
        const auto result = temporary();
        write_instruction(result + " = call " +
            llvm_type(target, position) + " @" + function +
            "(ptr " + array + ", i64 " + element_index.text + ")");
        const auto owned_result = own_direct_value({target, result});
        release(element_index);
        return owned_result;
    }
    const auto borrowed = temporary();
    write_instruction(borrowed +
        " = call ptr @" + std::string(native_array
            ? "txrt_array_ref_element_read_ptr"
            : "txrt_array_element_read_ptr") + "(ptr " + array + ", i64 " +
        element_index.text + ")");
    const auto result = from_any({value_type::any_type, borrowed},
                                 target, position);
    release(element_index);
    return result;
}

} // namespace tx
