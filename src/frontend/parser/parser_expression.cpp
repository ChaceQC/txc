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
            binary_operation{operation.kind, std::move(left),
                             std::move(right), std::nullopt});
    }
    return left;
}

expr_ptr parser::parse_unary()
{
    if (check(token_kind::plus_plus) || check(token_kind::minus_minus))
    {
        throw compile_error(current().position, "++ 和 -- 只支持后置写法");
    }
    if (match(token_kind::minus) || match(token_kind::bang))
    {
        const auto operation = previous();
        return std::make_unique<expression>(
            operation.position,
            unary_operation{operation.kind, parse_unary(), false,
                            std::nullopt});
    }
    return parse_postfix();
}

std::vector<call_argument> parser::parse_arguments()
{
    std::vector<call_argument> arguments;
    bool seen_keyword = false;
    if (!check(token_kind::right_paren))
    {
        do
        {
            const auto position = current().position;
            argument_kind kind = argument_kind::positional;
            std::string name;
            if (match(token_kind::double_star))
            {
                kind = argument_kind::spread_dict;
                seen_keyword = true;
            }
            else if (match(token_kind::star))
            {
                kind = argument_kind::spread_array;
            }
            else if (check(token_kind::identifier) &&
                     tokens_[index_ + 1].kind == token_kind::equal)
            {
                kind = argument_kind::keyword;
                name = advance().text;
                (void)advance();
                seen_keyword = true;
            }
            if (seen_keyword && (kind == argument_kind::positional ||
                                 kind == argument_kind::spread_array))
            {
                throw compile_error(position, "位置实参必须写在命名实参之前");
            }
            arguments.push_back({kind, std::move(name), parse_expression(), position});
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
            expr_ptr receiver;
            if (name != nullptr)
            {
                callable = name->name;
            }
            else if (auto* member = std::get_if<member_expression>(&value->data))
            {
                callable = member->field;
                receiver = std::move(member->object);
            }
            if (callable.empty())
            {
                throw compile_error(previous().position, "只能调用函数、构造函数或方法");
            }
            auto arguments = parse_arguments();
            value = std::make_unique<expression>(
                position, call_expression{callable, std::move(arguments),
                                          false, callable, std::nullopt,
                                          std::move(receiver), false, 0, {}, 0,
                                          false, {}});
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
                position, member_expression{std::move(value), field, std::nullopt});
        }
        else if (match(token_kind::keyword_as))
        {
            const auto position = value->position;
            const auto target = parse_type();
            value = std::make_unique<expression>(
                position, cast_expression{std::move(value), target});
        }
        else if (match(token_kind::plus_plus) || match(token_kind::minus_minus))
        {
            const auto operation = previous();
            value = std::make_unique<expression>(
                operation.position,
                update_expression{operation.kind, std::move(value)});
        }
        else
        {
            return value;
        }
    }
}

expr_ptr parser::parse_atom()
{
    if (check(token_kind::identifier) && value_type::is_container_name(current().text) &&
        tokens_[index_ + 1].kind == token_kind::less)
    {
        const auto position = current().position;
        auto type = parse_type();
        (void)consume(token_kind::left_paren, "容器构造需要左括号");
        call_expression call;
        call.name = type.container_name();
        call.source_name = type.name;
        call.container_type = std::move(type);
        call.arguments = parse_arguments();
        return std::make_unique<expression>(position, std::move(call));
    }
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
    if (match(token_kind::left_brace))
    {
        const auto position = previous().position;
        std::vector<dictionary_entry> entries;
        if (!check(token_kind::right_brace))
        {
            do
            {
                auto key = parse_expression();
                (void)consume(token_kind::colon, "字典键后需要冒号");
                entries.push_back({std::move(key), parse_expression()});
            } while (match(token_kind::comma));
        }
        (void)consume(token_kind::right_brace, "字典缺少右大括号");
        return std::make_unique<expression>(
            position, dictionary_literal{std::move(entries)});
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
