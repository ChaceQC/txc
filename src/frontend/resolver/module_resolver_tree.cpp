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
    if (auto* name = std::get_if<name_reference>(&item.data))
    {
        // 保留局部变量原名，语义分析先查变量，再采用这里记录的函数候选。
        if (const auto* own = find_export(module_key, name->name))
        {
            if (!own->is_type)
            {
                name->function_symbol = own->internal_name;
            }
        }
        else
        {
            for (const auto& dependency : scopes_.at(module_key)->imports)
            {
                if (!dependency.alias.empty())
                {
                    continue;
                }
                const auto* candidate = find_export(dependency.target, name->name);
                if (candidate == nullptr || candidate->is_type)
                {
                    continue;
                }
                if (!name->function_symbol.empty() &&
                    name->function_symbol != candidate->internal_name)
                {
                    name->ambiguous_function = true;
                }
                name->function_symbol = candidate->internal_name;
            }
        }
    }
    else if (auto* literal = std::get_if<array_literal>(&item.data))
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
        if (const auto* owner = std::get_if<name_reference>(&access->object->data))
        {
            for (const auto& dependency : scopes_.at(module_key)->imports)
            {
                if (!dependency.alias.empty() && dependency.alias == owner->name)
                {
                    const auto qualified = owner->name + "." + access->field;
                    const auto* symbol = find_symbol(module_key, qualified,
                                                     item.position, false);
                    if (symbol != nullptr && !symbol->is_type)
                    {
                        item.data = name_reference{qualified, symbol->internal_name};
                        return;
                    }
                }
            }
        }
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
        if (call->container_type)
        {
            call->container_type = resolve_type(module_key, *call->container_type,
                                             item.position);
            for (auto& argument : call->arguments)
            {
                resolve_expression(*argument.value, module_key);
            }
            return;
        }
        if (!call->receiver && call->name == "super")
        {
            if (call->arguments.size() != 1 ||
                call->arguments.front().kind != argument_kind::positional)
            {
                throw compile_error(item.position,
                                    "super(类型) 需要一个父类类型名");
            }
            const auto& type_value = *call->arguments.front().value;
            std::string type_text;
            if (const auto* name = std::get_if<name_reference>(&type_value.data))
            {
                type_text = name->name;
            }
            else if (const auto* member =
                         std::get_if<member_expression>(&type_value.data))
            {
                const auto* module =
                    std::get_if<name_reference>(&member->object->data);
                if (module != nullptr)
                {
                    type_text = module->name + "." + member->field;
                }
            }
            if (type_text.empty())
            {
                throw compile_error(item.position,
                                    "super(类型) 需要父类类型名");
            }
            call->is_super_view = true;
            call->super_type = resolve_type(module_key,
                value_type(type_text), item.position).name;
            return;
        }
        if (call->receiver)
        {
            const auto* name = std::get_if<name_reference>(&call->receiver->data);
            bool module_alias = false;
            if (name)
            {
                for (const auto& dependency : scopes_.at(module_key)->imports)
                {
                    if (!dependency.alias.empty() && dependency.alias == name->name)
                    {
                        module_alias = true;
                        break;
                    }
                }
            }
            if (module_alias)
            {
                call->name = name->name + "." + call->name;
                call->receiver.reset();
            }
            else
            {
                resolve_expression(*call->receiver, module_key);
            }
        }
        for (auto& argument : call->arguments)
        {
            resolve_expression(*argument.value, module_key);
        }
        call->source_name = call->name;
        if (!call->receiver)
        {
            call->name = resolve_call_name(module_key, call->name, item.position);
        }
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
    else if (auto* guarded = std::get_if<try_statement>(&item.data))
    {
        resolve_statements(guarded->body, module_key);
        for (auto& handler : guarded->handlers)
        {
            check_local_name(module_key, handler.name, handler.position);
            handler.type = resolve_type(module_key, handler.type, handler.position);
            resolve_statements(handler.body, module_key);
        }
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
