#include "backend/llvm/codegen.hpp"

namespace tx
{
namespace
{

bool scalar(const value_type& type)
{
    return type == value_type::int_type || type == value_type::float_type ||
        type == value_type::bool_type;
}

bool scalar_expression(const expression& item)
{
    if (!scalar(item.type))
    {
        return false;
    }
    if (std::holds_alternative<name_reference>(item.data) ||
        std::holds_alternative<integer_literal>(item.data) ||
        std::holds_alternative<floating_literal>(item.data) ||
        std::holds_alternative<boolean_literal>(item.data))
    {
        return true;
    }
    if (const auto* binary = std::get_if<binary_operation>(&item.data))
    {
        return !binary->binding && scalar_expression(*binary->left) &&
            scalar_expression(*binary->right);
    }
    if (const auto* unary = std::get_if<unary_operation>(&item.data))
    {
        return !unary->binding && scalar_expression(*unary->operand);
    }
    if (const auto* update = std::get_if<update_expression>(&item.data))
    {
        return std::holds_alternative<name_reference>(update->target->data);
    }
    if (const auto* index = std::get_if<index_expression>(&item.data))
    {
        return index->object->type.is_vector() &&
            std::holds_alternative<name_reference>(index->object->data) &&
            scalar_expression(*index->index);
    }
    return false;
}

} // namespace

bool llvm_code_generator::stable_vector_loop(const std::vector<stmt_ptr>& body)
{
    // 保守证明整个循环没有容器修改、未知调用和可析构临时对象，覆盖别名影响。
    for (const auto& item : body)
    {
        if (const auto* declaration = std::get_if<variable_declaration>(&item->data))
        {
            if (!declaration->initializer || !scalar_expression(*declaration->initializer))
            {
                return false;
            }
        }
        else if (const auto* assignment = std::get_if<variable_assignment>(&item->data))
        {
            if (!std::holds_alternative<name_reference>(assignment->target->data) ||
                !scalar(assignment->target->type) || !scalar_expression(*assignment->value))
            {
                return false;
            }
        }
        else if (const auto* expression = std::get_if<expression_statement>(&item->data))
        {
            if (!scalar_expression(*expression->value))
            {
                return false;
            }
        }
        else if (const auto* branch = std::get_if<if_statement>(&item->data))
        {
            if (!scalar_expression(*branch->condition) || !stable_vector_loop(branch->then_body) ||
                !stable_vector_loop(branch->else_body))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}

} // namespace tx
