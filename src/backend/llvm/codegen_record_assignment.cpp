#include "backend/llvm/codegen.hpp"

namespace tx
{

bool llvm_code_generator::emit_record_assignment(const statement& item,
    const variable_assignment& assignment)
{
    const auto* member = std::get_if<member_expression>(&assignment.target->data);
    if (!member || !static_record_type(member->object->type) ||
        (!assignment.binding && direct_scalar_field(*assignment.target)))
    {
        return false;
    }
    lvalue_indices indices;
    prepare_lvalue_indices(*assignment.target, indices);
    ir_value value{value_type::void_type, {}};
    std::string object;
    if (assignment.binding)
    {
        object = lvalue_address(*member->object, indices);
        const auto current = read_record_field({member->object->type, object},
                                               *assignment.target, *member);
        const auto right = expression_value(*assignment.value);
        value = emit_operator_call(assignment.target->type, item.position,
                                    *assignment.binding, current, right);
    }
    else
    {
        value = expression_value(*assignment.value);
        object = lvalue_address(*member->object, indices);
        if (assignment.operation == token_kind::plus_equal ||
            assignment.operation == token_kind::minus_equal)
        {
            const auto current = read_record_field({member->object->type, object},
                                                   *assignment.target, *member);
            const bool add = assignment.operation == token_kind::plus_equal;
            ir_value combined{current.type, {}};
            if (current.type == value_type::int_type)
            {
                combined = checked_binary(add ? "txrt_add_i64" : "txrt_sub_i64",
                                          current, value, current.type, item.position);
            }
            else if (current.type == value_type::float_type)
            {
                combined.text = temporary();
                write_instruction(combined.text + (add ? " = fadd double " : " = fsub double ") +
                                  current.text + ", " + value.text);
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
    const auto index = member->field_slot.value_or(classes_.contains(member->object->type.name)
        ? 0 : field_index(member->object->type, member->field, item.position));
    const auto slot = record_field_slot({member->object->type, object}, index);
    if (value.type == value_type::int_type || value.type == value_type::float_type ||
        value.type == value_type::bool_type)
    {
        write_instruction("store " + llvm_type(value.type, item.position) + " " + value.text +
                          ", ptr " + slot);
    }
    else
    {
        const auto reference = temporary();
        write_instruction(reference + " = load ptr, ptr " + slot);
        assign_any(reference, value, item.position);
    }
    release(value);
    return true;
}

} // namespace tx
