#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <cstdint>

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
    if (eligible_dynamic_local_array(declaration))
    {
        emit_dynamic_local_array_declaration(item, declaration);
        return;
    }
    if (const auto length = eligible_local_array_length(declaration))
    {
        emit_local_array_declaration(item, declaration, *length);
        return;
    }
    ir_value value{value_type::void_type, {}};
    if (declaration.array_length)
    {
        const auto length = expression_value(*declaration.array_length);
        const auto* literal = declaration.initializer
            ? std::get_if<array_literal>(&declaration.initializer->data)
            : nullptr;
        const auto* constant =
            std::get_if<integer_literal>(&declaration.array_length->data);
        const bool direct = literal && constant &&
            std::stoll(constant->digits) >=
                static_cast<std::int64_t>(literal->elements.size());
        ir_value initial{value_type::void_type, {}};
        if (declaration.initializer && !direct)
        {
            initial = expression_value(*declaration.initializer);
        }
        const auto result_address = allocate(value_type::array_type,
                                             item.position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @" +
            std::string(declaration.initializer && !direct
                ? "txrt_array_resize(i64 " + length.text +
                  ", ptr " + initial.text
                : "txrt_array_new(i64 " + length.text) +
            ", ptr " + result_address + ")");
        write_instruction("call void @txrt_require_success(i32 " +
                          status + ")");
        release(initial);
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + result_address);
        if (direct)
        {
            emit_array_elements(*literal, result, item.position);
        }
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
    const auto array_reference = type == value_type::array_type
        ? cache_array_reference(value.text, item.position) : std::string{};
    variable_slot slot{type, address, false, array_reference};
    if (type == value_type::dict_type)
    {
        slot.dict_reference = cache_dict_reference(value.text, item.position);
    }
    scopes_.back().emplace(declaration.name, std::move(slot));
}

void llvm_code_generator::emit_composite_assignment(
    const statement& item, const variable_assignment& assignment)
{
    if (emit_local_array_assignment(item, assignment))
    {
        return;
    }
    if (!assignment.binding && direct_scalar_field(*assignment.target))
    {
        const auto value = expression_value(*assignment.value);
        const auto address = scalar_field_address(*assignment.target);
        ir_value assigned = value;
        if (assignment.operation == token_kind::plus_equal ||
            assignment.operation == token_kind::minus_equal)
        {
            const auto current = load({assignment.target->type, address});
            const bool add = assignment.operation == token_kind::plus_equal;
            if (current.type == value_type::int_type)
            {
                assigned = checked_binary(add ? "txrt_add_i64" : "txrt_sub_i64",
                                          current, value, current.type,
                                          item.position);
            }
            else if (current.type == value_type::float_type)
            {
                assigned.text = temporary();
                write_instruction(assigned.text +
                    (add ? " = fadd double " : " = fsub double ") +
                    current.text + ", " + value.text);
            }
            else
            {
                throw compile_error(item.position,
                                    "LLVM 后端暂不支持此复合赋值类型");
            }
        }
        write_instruction("store " + llvm_type(assigned.type, item.position) +
                          " " + assigned.text + ", ptr " + address);
        release(value);
        return;
    }
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
    assign_any(target, value, item.position);
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
    refresh_array_reference(variable, value.text);
    refresh_dict_reference(variable, value.text);
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
    bool local_scalar_only = false;
    if (const auto* declaration =
            std::get_if<variable_declaration>(&item.data))
    {
        const auto slot = find_variable(declaration->name, item.position);
        if (slot.local_array_length)
        {
            const auto* literal = declaration->initializer
                ? std::get_if<array_literal>(&declaration->initializer->data)
                : nullptr;
            local_scalar_only =
                (!declaration->array_length ||
                 gc_neutral_expression(*declaration->array_length)) &&
                (!literal || std::all_of(literal->elements.begin(),
                    literal->elements.end(), [&](const expr_ptr& element)
                    { return gc_neutral_expression(*element); }));
        }
    }
    if (const auto* assignment =
            std::get_if<variable_assignment>(&item.data))
    {
        if (const auto* index =
                std::get_if<index_expression>(&assignment->target->data))
        {
            if (const auto* name =
                    std::get_if<name_reference>(&index->object->data))
            {
                local_scalar_only =
                    find_variable(name->name, item.position).local_array_length &&
                    gc_neutral_expression(*index->index) &&
                    gc_neutral_expression(*assignment->value);
            }
        }
    }
    if (!terminated_ && !local_scalar_only && !gc_neutral_statement(item))
    {
        emit_gc_safepoint();
    }
}

} // namespace tx
