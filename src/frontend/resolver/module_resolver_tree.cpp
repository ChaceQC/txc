#include "frontend/resolver/module_resolver.hpp"

namespace tx
{

void module_resolver::check_local_name(
    const std::string& module_key, const std::string& name,
    source_pos position) const
{
    for (const auto& dependency : scopes_.at(module_key)->imports)
    {
        if (!dependency.alias.empty() && dependency.alias == name)
        {
            throw compile_error(position, "变量名不能占用模块别名：" + name);
        }
    }
}

void module_resolver::resolve_expression(
    expression& item, const std::string& module_key)
{
    if (auto* literal = std::get_if<array_literal>(&item.data))
    {
        for (auto& element : literal->elements)
        {
            resolve_expression(*element, module_key);
        }
    }
    else if (auto* literal = std::get_if<dictionary_literal>(&item.data))
    {
        for (auto& entry : literal->entries)
        {
            resolve_expression(*entry.key, module_key);
            resolve_expression(*entry.value, module_key);
        }
    }
    else if (auto* access = std::get_if<index_expression>(&item.data))
    {
        resolve_expression(*access->object, module_key);
        resolve_expression(*access->index, module_key);
    }
    else if (auto* access = std::get_if<member_expression>(&item.data))
    {
        resolve_expression(*access->object, module_key);
    }
    else if (auto* cast = std::get_if<cast_expression>(&item.data))
    {
        resolve_expression(*cast->value, module_key);
        cast->target = resolve_type(module_key, cast->target, item.position);
    }
    else if (auto* operation = std::get_if<unary_operation>(&item.data))
    {
        resolve_expression(*operation->operand, module_key);
    }
    else if (auto* operation = std::get_if<update_expression>(&item.data))
    {
        resolve_expression(*operation->target, module_key);
    }
    else if (auto* operation = std::get_if<binary_operation>(&item.data))
    {
        resolve_expression(*operation->left, module_key);
        resolve_expression(*operation->right, module_key);
    }
    else if (auto* call = std::get_if<call_expression>(&item.data))
    {
        for (auto& argument : call->arguments)
        {
            resolve_expression(*argument.value, module_key);
        }
        call->source_name = call->name;
        call->name = resolve_call_name(module_key, call->name, item.position);
    }
}

void module_resolver::resolve_statement(
    statement& item, const std::string& module_key)
{
    if (auto* declaration = std::get_if<variable_declaration>(&item.data))
    {
        check_local_name(module_key, declaration->name, item.position);
        if (declaration->declared_type)
        {
            declaration->declared_type = resolve_type(
                module_key, *declaration->declared_type, item.position);
        }
        if (declaration->initializer)
        {
            resolve_expression(*declaration->initializer, module_key);
        }
        if (declaration->array_length)
        {
            resolve_expression(*declaration->array_length, module_key);
        }
    }
    else if (auto* assignment = std::get_if<variable_assignment>(&item.data))
    {
        resolve_expression(*assignment->target, module_key);
        resolve_expression(*assignment->value, module_key);
    }
    else if (auto* assignment = std::get_if<unpack_assignment>(&item.data))
    {
        for (const auto& name : assignment->names)
        {
            check_local_name(module_key, name, item.position);
        }
        resolve_expression(*assignment->value, module_key);
    }
    else if (auto* loop = std::get_if<for_loop>(&item.data))
    {
        check_local_name(module_key, loop->name, item.position);
        resolve_expression(*loop->first, module_key);
        resolve_expression(*loop->last, module_key);
        resolve_statements(loop->body, module_key);
    }
    else if (auto* loop = std::get_if<for_each>(&item.data))
    {
        check_local_name(module_key, loop->name, item.position);
        resolve_expression(*loop->values, module_key);
        resolve_statements(loop->body, module_key);
    }
    else if (auto* branch = std::get_if<if_statement>(&item.data))
    {
        resolve_expression(*branch->condition, module_key);
        resolve_statements(branch->then_body, module_key);
        resolve_statements(branch->else_body, module_key);
    }
    else if (auto* loop = std::get_if<while_statement>(&item.data))
    {
        resolve_expression(*loop->condition, module_key);
        resolve_statements(loop->body, module_key);
    }
    else if (auto* result = std::get_if<return_statement>(&item.data))
    {
        if (result->value)
        {
            resolve_expression(*result->value, module_key);
        }
    }
    else if (auto* expression_only = std::get_if<expression_statement>(&item.data))
    {
        resolve_expression(*expression_only->value, module_key);
    }
}

void module_resolver::resolve_statements(
    std::vector<stmt_ptr>& statements, const std::string& module_key)
{
    for (auto& item : statements)
    {
        resolve_statement(*item, module_key);
    }
}

} // namespace tx
