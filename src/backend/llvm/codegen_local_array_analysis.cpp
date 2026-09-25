#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <charconv>
#include <limits>

namespace tx
{
namespace
{

bool is_name(const expression& item, std::string_view name)
{
    const auto* reference = std::get_if<name_reference>(&item.data);
    return reference && reference->name == name;
}

const index_expression* array_index(const expression& item,
                                    std::string_view name)
{
    const auto* index = std::get_if<index_expression>(&item.data);
    return index && is_name(*index->object, name) ? index : nullptr;
}

bool scalar_type(const value_type& type)
{
    return type == value_type::int_type ||
           type == value_type::float_type ||
           type == value_type::bool_type ||
           type == value_type::none_type;
}

bool safe_expression(const expression& item, std::string_view name)
{
    if (is_name(item, name))
    {
        return false;
    }
    if (const auto* literal = std::get_if<array_literal>(&item.data))
    {
        return std::all_of(literal->elements.begin(), literal->elements.end(),
            [&](const expr_ptr& value) { return safe_expression(*value, name); });
    }
    if (const auto* literal = std::get_if<dictionary_literal>(&item.data))
    {
        return std::all_of(literal->entries.begin(), literal->entries.end(),
            [&](const dictionary_entry& entry)
            {
                return safe_expression(*entry.key, name) &&
                       safe_expression(*entry.value, name);
            });
    }
    if (const auto* index = std::get_if<index_expression>(&item.data))
    {
        return safe_expression(*index->object, name) &&
               safe_expression(*index->index, name);
    }
    if (const auto* member = std::get_if<member_expression>(&item.data))
    {
        return safe_expression(*member->object, name);
    }
    if (const auto* cast = std::get_if<cast_expression>(&item.data))
    {
        if (const auto* index = array_index(*cast->value, name);
            index && (cast->target == value_type::int_type ||
                      cast->target == value_type::float_type ||
                      cast->target == value_type::bool_type))
        {
            return safe_expression(*index->index, name);
        }
        return safe_expression(*cast->value, name);
    }
    if (const auto* unary = std::get_if<unary_operation>(&item.data))
    {
        return safe_expression(*unary->operand, name);
    }
    if (const auto* update = std::get_if<update_expression>(&item.data))
    {
        return safe_expression(*update->target, name);
    }
    if (const auto* binary = std::get_if<binary_operation>(&item.data))
    {
        return safe_expression(*binary->left, name) &&
               safe_expression(*binary->right, name);
    }
    if (const auto* call = std::get_if<call_expression>(&item.data))
    {
        if (!call->receiver && call->arguments.size() == 1)
        {
            const auto& argument = *call->arguments.front().value;
            if (call->name == "len" && is_name(argument, name))
            {
                return true;
            }
            if (call->name == "is_none")
            {
                if (const auto* index = array_index(argument, name))
                {
                    return safe_expression(*index->index, name);
                }
            }
        }
        if (call->receiver && !safe_expression(*call->receiver, name))
        {
            return false;
        }
        return std::all_of(call->arguments.begin(), call->arguments.end(),
            [&](const call_argument& argument)
            { return safe_expression(*argument.value, name); });
    }
    return true;
}

bool safe_body(const std::vector<stmt_ptr>& body, std::string_view name,
               const variable_declaration& candidate, std::size_t length);

bool safe_statement(const statement& item, std::string_view name,
                    const variable_declaration& candidate,
                    std::size_t length)
{
    if (const auto* declaration =
            std::get_if<variable_declaration>(&item.data))
    {
        if (declaration != &candidate && declaration->name == name)
        {
            return false;
        }
        return (!declaration->initializer ||
                safe_expression(*declaration->initializer, name)) &&
               (!declaration->array_length ||
                safe_expression(*declaration->array_length, name));
    }
    if (const auto* assignment =
            std::get_if<variable_assignment>(&item.data))
    {
        if (const auto* index = array_index(*assignment->target, name))
        {
            return assignment->operation == token_kind::equal &&
                   !assignment->binding &&
                   scalar_type(assignment->value->type) &&
                   safe_expression(*index->index, name) &&
                   safe_expression(*assignment->value, name);
        }
        return safe_expression(*assignment->target, name) &&
               safe_expression(*assignment->value, name);
    }
    if (const auto* unpack = std::get_if<unpack_assignment>(&item.data))
    {
        if (std::find(unpack->names.begin(), unpack->names.end(), name) !=
            unpack->names.end())
        {
            return false;
        }
        if (is_name(*unpack->value, name))
        {
            return unpack->names.size() == length &&
                   std::all_of(unpack->declares.begin(), unpack->declares.end(),
                               [](bool declares) { return declares; });
        }
        return safe_expression(*unpack->value, name);
    }
    if (const auto* branch = std::get_if<if_statement>(&item.data))
    {
        return safe_expression(*branch->condition, name) &&
               safe_body(branch->then_body, name, candidate, length) &&
               safe_body(branch->else_body, name, candidate, length);
    }
    if (const auto* loop = std::get_if<while_statement>(&item.data))
    {
        return safe_expression(*loop->condition, name) &&
               safe_body(loop->body, name, candidate, length);
    }
    if (const auto* loop = std::get_if<for_loop>(&item.data))
    {
        return loop->name != name &&
               safe_expression(*loop->first, name) &&
               safe_expression(*loop->last, name) &&
               safe_body(loop->body, name, candidate, length);
    }
    if (const auto* loop = std::get_if<for_each>(&item.data))
    {
        return loop->name != name &&
               (is_name(*loop->values, name) ||
                safe_expression(*loop->values, name)) &&
               safe_body(loop->body, name, candidate, length);
    }
    if (const auto* result = std::get_if<return_statement>(&item.data))
    {
        return !result->value || safe_expression(*result->value, name);
    }
    return safe_expression(*std::get<expression_statement>(item.data).value,
                           name);
}

bool safe_body(const std::vector<stmt_ptr>& body, std::string_view name,
               const variable_declaration& candidate, std::size_t length)
{
    return std::all_of(body.begin(), body.end(), [&](const stmt_ptr& item)
    { return safe_statement(*item, name, candidate, length); });
}

} // namespace

std::optional<std::size_t> llvm_code_generator::eligible_local_array_length(
    const variable_declaration& declaration) const
{
    if (!current_function_body_ ||
        declaration.declared_type.value_or(value_type::array_type) !=
            value_type::array_type)
    {
        return std::nullopt;
    }
    const auto* literal = declaration.initializer
        ? std::get_if<array_literal>(&declaration.initializer->data) : nullptr;
    if (declaration.initializer && !literal)
    {
        return std::nullopt;
    }
    std::size_t length = literal ? literal->elements.size() : 0;
    if (declaration.array_length)
    {
        const auto* constant =
            std::get_if<integer_literal>(&declaration.array_length->data);
        if (!constant)
        {
            return std::nullopt;
        }
        const auto [end, error] = std::from_chars(
            constant->digits.data(),
            constant->digits.data() + constant->digits.size(), length);
        if (error != std::errc{} ||
            end != constant->digits.data() + constant->digits.size())
        {
            return std::nullopt;
        }
    }
    if (length > 32 || (literal && literal->elements.size() > length) ||
        (literal && !std::all_of(literal->elements.begin(),
            literal->elements.end(), [](const expr_ptr& element)
            { return scalar_type(element->type); })) ||
        !safe_body(*current_function_body_, declaration.name,
                   declaration, length))
    {
        return std::nullopt;
    }
    return length;
}

bool llvm_code_generator::eligible_dynamic_local_array(
    const variable_declaration& declaration) const
{
    if (!current_function_body_ || !declaration.array_length ||
        declaration.initializer ||
        declaration.declared_type.value_or(value_type::array_type) !=
            value_type::array_type ||
        std::holds_alternative<integer_literal>(declaration.array_length->data))
    {
        return false;
    }
    // 第一版仅处理函数体顶层声明，保证动态存储只创建一次并由该函数释放。
    const bool top_level = std::any_of(current_function_body_->begin(),
        current_function_body_->end(), [&](const stmt_ptr& item)
        {
            const auto* current =
                std::get_if<variable_declaration>(&item->data);
            return current == &declaration;
        });
    return top_level && safe_body(*current_function_body_, declaration.name,
        declaration, std::numeric_limits<std::size_t>::max());
}

} // namespace tx
