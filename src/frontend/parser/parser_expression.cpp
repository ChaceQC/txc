#include "frontend/parser/parser.hpp"

#include <memory>
#include <utility>

namespace tx
{
namespace
{

int precedence(token_kind kind)
{
    switch (kind)
    {
    case token_kind::or_or:
        return 1;
    case token_kind::and_and:
        return 2;
    case token_kind::equal_equal:
    case token_kind::bang_equal:
        return 3;
    case token_kind::less:
    case token_kind::less_equal:
    case token_kind::greater:
    case token_kind::greater_equal:
        return 4;
    case token_kind::plus:
    case token_kind::minus:
        return 5;
    case token_kind::star:
    case token_kind::slash:
        return 6;
    default:
        return -1;
    }
}

} // namespace

expr_ptr parser::parse_expression(int min_precedence)
{
    auto left = parse_unary();
    while (precedence(current().kind) >= min_precedence)
    {
        const auto operation = advance();
        auto right = parse_expression(precedence(operation.kind) + 1);
        left = std::make_unique<expression>(
            operation.position,
            binary_operation{operation.kind, std::move(left), std::move(right)});
    }
    return left;
}

expr_ptr parser::parse_unary()
{
    if (match(token_kind::minus) || match(token_kind::bang))
    {
        const auto operation = previous();
        return std::make_unique<expression>(
            operation.position,
            unary_operation{operation.kind, parse_unary(), false});
    }
    return parse_postfix();
}

std::vector<expr_ptr> parser::parse_arguments()
{
    std::vector<expr_ptr> arguments;
    if (!check(token_kind::right_paren))
    {
        do
        {
            arguments.push_back(parse_expression());
        } while (match(token_kind::comma));
    }
    (void)consume(token_kind::right_paren, "调用缺少右括号");
    return arguments;
}

expr_ptr parser::parse_postfix()
{
    auto value = parse_atom();
    while (true)
    {
        if (match(token_kind::left_paren))
        {
            const auto position = value->position;
            const auto* name = std::get_if<name_reference>(&value->data);
            std::string callable;
            if (name != nullptr)
            {
                callable = name->name;
            }
            else if (const auto* member = std::get_if<member_expression>(&value->data))
            {
                const auto* module = std::get_if<name_reference>(&member->object->data);
                if (module != nullptr)
                {
                    callable = module->name + "." + member->field;
                }
            }
            if (callable.empty())
            {
                throw compile_error(previous().position, "只能调用函数或结构体");
            }
            auto arguments = parse_arguments();
            value = std::make_unique<expression>(
                position, call_expression{callable, std::move(arguments),
                                          false, callable});
        }
        else if (match(token_kind::left_bracket))
        {
            const auto position = value->position;
            auto index = parse_expression();
            (void)consume(token_kind::right_bracket, "索引缺少右方括号");
            value = std::make_unique<expression>(
                position, index_expression{std::move(value), std::move(index)});
        }
        else if (match(token_kind::dot))
        {
            const auto position = value->position;
            const auto field = consume(token_kind::identifier, "点号后需要字段名").text;
            value = std::make_unique<expression>(
                position, member_expression{std::move(value), field});
        }
        else if (match(token_kind::keyword_as))
        {
            const auto position = value->position;
            const auto target = parse_type();
            value = std::make_unique<expression>(
                position, cast_expression{std::move(value), target});
        }
        else
        {
            return value;
        }
    }
}

expr_ptr parser::parse_atom()
{
    if (match(token_kind::integer))
    {
        return std::make_unique<expression>(
            previous().position, integer_literal{previous().text});
    }
    if (match(token_kind::floating))
    {
        return std::make_unique<expression>(
            previous().position, floating_literal{previous().text});
    }
    if (match(token_kind::string_literal))
    {
        return std::make_unique<expression>(
            previous().position, string_literal{previous().text});
    }
    if (match(token_kind::keyword_true) || match(token_kind::keyword_false))
    {
        return std::make_unique<expression>(
            previous().position,
            boolean_literal{previous().kind == token_kind::keyword_true});
    }
    if (match(token_kind::keyword_none))
    {
        return std::make_unique<expression>(previous().position, none_literal{});
    }
    if (match(token_kind::identifier))
    {
        return std::make_unique<expression>(
            previous().position, name_reference{previous().text});
    }
    if (match(token_kind::left_bracket))
    {
        const auto position = previous().position;
        std::vector<expr_ptr> elements;
        if (!check(token_kind::right_bracket))
        {
            do
            {
                elements.push_back(parse_expression());
            } while (match(token_kind::comma));
        }
        (void)consume(token_kind::right_bracket, "数组缺少右方括号");
        return std::make_unique<expression>(
            position, array_literal{std::move(elements)});
    }
    if (match(token_kind::left_paren))
    {
        auto value = parse_expression();
        (void)consume(token_kind::right_paren, "表达式缺少右括号");
        return value;
    }
    throw compile_error(current().position, "需要表达式");
}

} // namespace tx
