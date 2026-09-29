#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{
namespace
{

// 只允许指定局部的字段操作或容器方法；作为普通实参、别名或返回值均视为逃逸。
class local_use_checker
{
public:
    const variable_declaration& candidate;
    bool fields_only;
    bool scalar_reads = false;
    bool read_only_fields = false;
    const variable_declaration* allowed_alias = nullptr;

    bool field_owner(const expression& item) const
    {
        if (const auto* member = std::get_if<member_expression>(&item.data))
        {
            return field_owner(*member->object);
        }
        return named(item);
    }

    bool named(const expression& item) const
    {
        const auto* name = std::get_if<name_reference>(&item.data);
        return name && name->name == candidate.name;
    }

    bool expression_safe(const expression& item) const
    {
        if (named(item))
        {
            return scalar_reads;
        }
        if (const auto* member = std::get_if<member_expression>(&item.data))
        {
            if (read_only_fields && field_owner(*member->object))
            {
                return item.type == value_type::int_type || item.type == value_type::float_type ||
                    item.type == value_type::bool_type || item.type == value_type::str_type;
            }
            return (fields_only && named(*member->object)) ||
                expression_safe(*member->object);
        }
        if (const auto* call = std::get_if<call_expression>(&item.data))
        {
            if (call->receiver && !(!fields_only && named(*call->receiver)) &&
                !expression_safe(*call->receiver))
            {
                return false;
            }
            return std::all_of(call->arguments.begin(), call->arguments.end(),
                [&](const call_argument& argument)
                {
                    return expression_safe(*argument.value);
                });
        }
        if (const auto* binary = std::get_if<binary_operation>(&item.data))
        {
            return expression_safe(*binary->left) && expression_safe(*binary->right);
        }
        if (const auto* unary = std::get_if<unary_operation>(&item.data))
        {
            return expression_safe(*unary->operand);
        }
        if (const auto* update = std::get_if<update_expression>(&item.data))
        {
            if (read_only_fields && field_owner(*update->target))
            {
                return false;
            }
            return !(scalar_reads && named(*update->target)) && expression_safe(*update->target);
        }
        if (const auto* cast = std::get_if<cast_expression>(&item.data))
        {
            return expression_safe(*cast->value);
        }
        if (const auto* index = std::get_if<index_expression>(&item.data))
        {
            return expression_safe(*index->object) && expression_safe(*index->index);
        }
        if (const auto* array = std::get_if<array_literal>(&item.data))
        {
            return std::all_of(array->elements.begin(), array->elements.end(),
                [&](const expr_ptr& value)
                {
                    return expression_safe(*value);
                });
        }
        if (const auto* dictionary = std::get_if<dictionary_literal>(&item.data))
        {
            return std::all_of(dictionary->entries.begin(), dictionary->entries.end(),
                [&](const dictionary_entry& entry)
                {
                    return expression_safe(*entry.key) && expression_safe(*entry.value);
                });
        }
        return true;
    }

    bool body_safe(const std::vector<stmt_ptr>& body) const
    {
        return std::all_of(body.begin(), body.end(), [&](const stmt_ptr& item)
        {
            return statement_safe(*item);
        });
    }

    bool statement_safe(const statement& item) const
    {
        if (const auto* declaration = std::get_if<variable_declaration>(&item.data))
        {
            return (declaration == &candidate || declaration->name != candidate.name) &&
                (declaration == allowed_alias || !declaration->initializer ||
                 expression_safe(*declaration->initializer)) &&
                (!declaration->array_length || expression_safe(*declaration->array_length));
        }
        if (const auto* assignment = std::get_if<variable_assignment>(&item.data))
        {
            if (read_only_fields && field_owner(*assignment->target))
            {
                return false;
            }
            return !(scalar_reads && named(*assignment->target)) &&
                expression_safe(*assignment->target) && expression_safe(*assignment->value);
        }
        if (const auto* unpack = std::get_if<unpack_assignment>(&item.data))
        {
            return std::find(unpack->names.begin(), unpack->names.end(), candidate.name) ==
                unpack->names.end() && expression_safe(*unpack->value);
        }
        if (const auto* branch = std::get_if<if_statement>(&item.data))
        {
            return expression_safe(*branch->condition) &&
                body_safe(branch->then_body) && body_safe(branch->else_body);
        }
        if (const auto* loop = std::get_if<while_statement>(&item.data))
        {
            return expression_safe(*loop->condition) && body_safe(loop->body);
        }
        if (const auto* loop = std::get_if<for_loop>(&item.data))
        {
            return loop->name != candidate.name && expression_safe(*loop->first) &&
                expression_safe(*loop->last) && body_safe(loop->body);
        }
        if (const auto* loop = std::get_if<for_each>(&item.data))
        {
            return loop->name != candidate.name && expression_safe(*loop->values) &&
                body_safe(loop->body);
        }
        if (const auto* guarded = std::get_if<try_statement>(&item.data))
        {
            return body_safe(guarded->body) && std::all_of(
                guarded->handlers.begin(), guarded->handlers.end(),
                [&](const exception_clause& handler)
                {
                    return handler.name != candidate.name && body_safe(handler.body);
                });
        }
        if (const auto* result = std::get_if<return_statement>(&item.data))
        {
            return !result->value || expression_safe(*result->value);
        }
        return expression_safe(*std::get<expression_statement>(item.data).value);
    }
};

} // namespace

bool llvm_code_generator::confined_local(
    const variable_declaration& declaration, bool fields_only) const
{
    return current_function_body_ &&
        local_use_checker{declaration, fields_only}.body_safe(*current_function_body_);
}

bool llvm_code_generator::scalar_local_unchanged(const std::string& name,
    const std::vector<stmt_ptr>& body, const variable_declaration* allowed)
{
    variable_declaration candidate;
    candidate.name = name;
    return local_use_checker{allowed ? *allowed : candidate, false, true}.body_safe(body);
}

bool llvm_code_generator::readonly_local_fields(const variable_declaration& declaration,
    const variable_declaration* allowed_alias) const
{
    return current_function_body_ && local_use_checker{
        declaration, true, false, true, allowed_alias}.body_safe(*current_function_body_);
}

bool llvm_code_generator::default_heap_local(const variable_declaration& declaration) const
{
    const auto* call = declaration.initializer
        ? std::get_if<call_expression>(&declaration.initializer->data) : nullptr;
    if (!call || !call->container_type || call->container_type->container_name() != "heap")
    {
        return false;
    }
    const auto& element = call->container_type->parameters.front();
    return (element == value_type::int_type || element == value_type::float_type ||
            element == value_type::bool_type || element == value_type::str_type) &&
        std::none_of(call->arguments.begin(), call->arguments.end(),
            [](const call_argument& argument)
            {
                return argument.value->type.is_function();
            }) && confined_local(declaration, false);
}

bool llvm_code_generator::default_heap_receiver(const call_expression& call) const
{
    const auto* name = call.receiver
        ? std::get_if<name_reference>(&call.receiver->data) : nullptr;
    return name && find_variable(name->name, call.receiver->position).default_heap &&
        (call.name == "push" || call.name == "top" || call.name == "pop");
}

} // namespace tx
