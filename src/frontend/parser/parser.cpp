#include "frontend/parser/parser.hpp"

#include <memory>
#include <utility>

namespace tx
{

parser::parser(std::vector<token> tokens, bool interface_mode)
    : tokens_(std::move(tokens)), interface_mode_(interface_mode)
{
}

const token& parser::current() const
{
    return tokens_[index_];
}

const token& parser::previous() const
{
    return tokens_[index_ - 1];
}

const token& parser::advance()
{
    if (!check(token_kind::end_of_file))
    {
        ++index_;
    }
    return previous();
}

bool parser::check(token_kind kind) const
{
    return current().kind == kind;
}

bool parser::match(token_kind kind)
{
    if (!check(kind))
    {
        return false;
    }
    (void)advance();
    return true;
}

const token& parser::consume(token_kind kind, const char* message)
{
    if (!check(kind))
    {
        throw compile_error(current().position, message);
    }
    return advance();
}

void parser::skip_newlines()
{
    while (match(token_kind::newline))
    {
    }
}

value_type parser::parse_type()
{
    if (match(token_kind::keyword_fn))
    {
        if (!check(token_kind::left_paren))
        {
            return value_type::fn_type;
        }
        (void)consume(token_kind::left_paren, "fn 类型需要左括号");
        std::vector<value_type> arguments;
        if (!check(token_kind::right_paren))
        {
            do
            {
                arguments.push_back(parse_type());
            } while (match(token_kind::comma));
        }
        (void)consume(token_kind::right_paren, "fn 类型缺少右括号");
        (void)consume(token_kind::arrow, "fn 类型需要 -> 返回类型");
        return value_type::function_of(std::move(arguments), parse_type());
    }
    if (match(token_kind::keyword_int))
    {
        return value_type::int_type;
    }
    if (match(token_kind::keyword_bool))
    {
        return value_type::bool_type;
    }
    if (match(token_kind::keyword_float))
    {
        return value_type::float_type;
    }
    if (match(token_kind::keyword_str))
    {
        return value_type::str_type;
    }
    if (match(token_kind::keyword_array))
    {
        return value_type::array_type;
    }
    if (match(token_kind::keyword_dict))
    {
        return value_type::dict_type;
    }
    if (match(token_kind::keyword_none))
    {
        return value_type::none_type;
    }
    if (match(token_kind::identifier))
    {
        std::string name = previous().text;
        if (value_type::is_container_name(name) && match(token_kind::less))
        {
            std::vector<value_type> arguments;
            arguments.push_back(parse_type());
            if (name == "map")
            {
                (void)consume(token_kind::comma, "map 的键和值类型之间需要逗号");
                arguments.push_back(parse_type());
            }
            (void)consume(token_kind::greater, "容器类型参数后需要 >");
            return value_type::container_of(std::move(name), std::move(arguments));
        }
        while (match(token_kind::dot))
        {
            name += "." + consume(token_kind::identifier, "点号后需要类型名").text;
        }
        return value_type(std::move(name));
    }
    throw compile_error(current().position, "需要类型名称");
}

import_decl parser::parse_import()
{
    const auto position = consume(token_kind::keyword_import, "需要 import").position;
    const auto& path = consume(token_kind::string_literal, "import 后需要路径字符串");
    const auto value = path.text.substr(1, path.text.size() - 2);
    if (value.empty() || value.find('\\') != std::string::npos)
    {
        throw compile_error(path.position, "导入路径不能为空，请使用正斜杠且不要转义");
    }
    std::string alias;
    if (match(token_kind::keyword_as))
    {
        alias = consume(token_kind::identifier, "as 后需要模块别名").text;
    }
    return {value, alias, position};
}

std::vector<stmt_ptr> parser::parse_block()
{
    // 只有等待代码块左大括号时才跨越换行，普通语句仍按行结束。
    skip_newlines();
    (void)consume(token_kind::left_brace, "需要左大括号");
    skip_newlines();
    std::vector<stmt_ptr> body;
    while (!check(token_kind::right_brace))
    {
        if (check(token_kind::end_of_file))
        {
            throw compile_error(current().position, "代码块缺少右大括号");
        }
        body.push_back(parse_statement());
        if (!check(token_kind::right_brace) && !match(token_kind::newline))
        {
            throw compile_error(current().position, "语句结束处需要换行");
        }
        skip_newlines();
    }
    (void)advance();
    return body;
}

bool parser::looks_like_declaration() const
{
    const auto kind = current().kind;
    if (kind == token_kind::keyword_auto || kind == token_kind::keyword_fn ||
        kind == token_kind::keyword_int ||
        kind == token_kind::keyword_bool || kind == token_kind::keyword_float ||
        kind == token_kind::keyword_str || kind == token_kind::keyword_dict)
    {
        return true;
    }
    if (kind != token_kind::identifier)
    {
        return false;
    }
    std::size_t next = index_ + 1;
    if (value_type::is_container_name(current().text) && next < tokens_.size() &&
        tokens_[next].kind == token_kind::less)
    {
        int depth = 0;
        do
        {
            if (tokens_[next].kind == token_kind::less)
            {
                ++depth;
            }
            else if (tokens_[next].kind == token_kind::greater)
            {
                --depth;
            }
            ++next;
        } while (next < tokens_.size() && depth > 0);
    }
    while (next + 1 < tokens_.size() &&
           tokens_[next].kind == token_kind::dot &&
           tokens_[next + 1].kind == token_kind::identifier)
    {
        next += 2;
    }
    return next + 1 < tokens_.size() &&
           tokens_[next].kind == token_kind::identifier &&
           tokens_[next + 1].kind == token_kind::equal;
}

stmt_ptr parser::parse_for(source_pos position)
{
    const auto name = consume(token_kind::identifier, "for 后需要循环变量").text;
    (void)consume(token_kind::keyword_in, "循环变量后需要 in");
    auto values = parse_expression();
    if (match(token_kind::range_inclusive))
    {
        auto last = parse_expression();
        auto body = parse_block();
        return std::make_unique<statement>(
            position, for_loop{name, std::move(values), std::move(last), std::move(body)});
    }
    auto body = parse_block();
    return std::make_unique<statement>(
        position, for_each{name, std::move(values), std::move(body)});
}

stmt_ptr parser::parse_if(source_pos position)
{
    auto condition = parse_expression();
    auto then_body = parse_block();
    const auto saved_index = index_;
    skip_newlines();
    const bool has_else = match(token_kind::keyword_else);
    std::vector<stmt_ptr> else_body;
    if (has_else)
    {
        skip_newlines();
        if (match(token_kind::keyword_if))
        {
            // 将 else if 表示为 else 中的嵌套分支，复用类型和控制流检查。
            else_body.push_back(parse_if(previous().position));
        }
        else
        {
            else_body = parse_block();
        }
    }
    else
    {
        index_ = saved_index;
    }
    return std::make_unique<statement>(
        position, if_statement{std::move(condition), std::move(then_body),
                               std::move(else_body), has_else});
}

stmt_ptr parser::parse_while(source_pos position)
{
    auto condition = parse_expression();
    auto body = parse_block();
    return std::make_unique<statement>(
        position, while_statement{std::move(condition), std::move(body)});
}

stmt_ptr parser::parse_statement()
{
    const auto position = current().position;
    if (match(token_kind::keyword_try))
    {
        return parse_try(position);
    }
    if (check(token_kind::keyword_exception))
    {
        throw compile_error(position, "exception 必须紧跟 try 或前一个 exception 分支");
    }
    if (match(token_kind::keyword_for))
    {
        return parse_for(position);
    }
    if (match(token_kind::keyword_if))
    {
        return parse_if(position);
    }
    if (match(token_kind::keyword_while))
    {
        return parse_while(position);
    }
    if (match(token_kind::keyword_return))
    {
        expr_ptr value;
        if (!check(token_kind::newline) && !check(token_kind::right_brace))
        {
            value = parse_expression();
        }
        return std::make_unique<statement>(
            position, return_statement{std::move(value)});
    }
    if (check(token_kind::identifier) &&
        tokens_[index_ + 1].kind == token_kind::comma)
    {
        std::vector<std::string> names;
        names.push_back(advance().text);
        (void)consume(token_kind::comma, "解包变量名之间需要逗号");
        do
        {
            names.push_back(consume(token_kind::identifier,
                                    "解包赋值需要变量名").text);
        } while (match(token_kind::comma));
        (void)consume(token_kind::equal, "解包赋值需要 =");
        return std::make_unique<statement>(
            position, unpack_assignment{std::move(names), {}, parse_expression()});
    }
    if (match(token_kind::keyword_array))
    {
        expr_ptr length;
        if (match(token_kind::left_bracket))
        {
            length = parse_expression();
            (void)consume(token_kind::right_bracket, "数组长度缺少右方括号");
        }
        const auto name = consume(token_kind::identifier, "数组声明需要变量名").text;
        expr_ptr initializer;
        if (match(token_kind::equal))
        {
            initializer = parse_expression();
        }
        else if (length == nullptr)
        {
            throw compile_error(current().position, "未指定长度的数组需要初值");
        }
        return std::make_unique<statement>(
            position, variable_declaration{name, value_type::array_type,
                                           std::move(initializer), std::move(length)});
    }
    if (looks_like_declaration())
    {
        const bool inferred = match(token_kind::keyword_auto);
        const auto declared_type = inferred
            ? std::optional<value_type>{}
            : std::optional<value_type>{parse_type()};
        const auto name = consume(token_kind::identifier, "声明需要变量名").text;
        (void)consume(token_kind::equal, "变量声明需要初值");
        return std::make_unique<statement>(
            position, variable_declaration{name, declared_type, parse_expression(), nullptr});
    }
    auto target = parse_expression();
    if (match(token_kind::equal) || match(token_kind::plus_equal) ||
        match(token_kind::minus_equal))
    {
        const auto operation = previous().kind;
        return std::make_unique<statement>(
            position, variable_assignment{std::move(target), operation,
                                          parse_expression(), std::nullopt});
    }
    return std::make_unique<statement>(
        position, expression_statement{std::move(target)});
}

program parser::parse_program()
{
    program result;
    skip_newlines();
    while (!check(token_kind::end_of_file))
    {
        if (check(token_kind::keyword_import))
        {
            result.imports.push_back(parse_import());
            if (!check(token_kind::newline) && !check(token_kind::end_of_file))
            {
                throw compile_error(current().position, "import 后需要换行");
            }
        }
        else if (check(token_kind::keyword_struct))
        {
            result.structs.push_back(parse_struct());
        }
        else if (check(token_kind::keyword_class))
        {
            result.classes.push_back(parse_class());
        }
        else if (check(token_kind::keyword_interface))
        {
            result.classes.push_back(parse_class(false, true));
        }
        else if (match(token_kind::keyword_abstract))
        {
            result.classes.push_back(parse_class(true, false));
        }
        else
        {
            result.functions.push_back(parse_function());
        }
        skip_newlines();
    }
    return result;
}

} // namespace tx
