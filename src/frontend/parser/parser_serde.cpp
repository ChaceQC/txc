#include "frontend/parser/parser.hpp"

#include <charconv>
#include <limits>
#include <string>

namespace tx
{

std::int64_t parser::parse_serde_number()
{
    const auto& item = consume(token_kind::integer, "serde 需要正整数字段编号或版本");
    std::int64_t result = 0;
    const auto [end, error] = std::from_chars(item.text.data(),
        item.text.data() + item.text.size(), result);
    if (error != std::errc{} || end != item.text.data() + item.text.size() ||
        result <= 0 || result > std::numeric_limits<std::int32_t>::max())
    {
        throw compile_error(item.position, "serde 编号和版本必须在 1 到 2147483647 之间");
    }
    return result;
}

serde_struct_metadata parser::parse_serde_struct_metadata()
{
    (void)advance();
    (void)consume(token_kind::left_paren, "serde 声明需要左括号");
    serde_struct_metadata metadata;
    bool seen_version = false;
    bool seen_unknown = false;
    bool seen_reserved = false;
    do
    {
        const auto& key = consume(token_kind::identifier, "serde 需要选项名称");
        (void)consume(token_kind::equal, "serde 选项需要 =");
        if (key.text == "version" && !seen_version)
        {
            metadata.version = parse_serde_number();
            seen_version = true;
        }
        else if (key.text == "unknown" && !seen_unknown)
        {
            const auto& value = consume(token_kind::string_literal,
                "serde unknown 需要字符串策略");
            if (value.text == "\"reject\"")
            {
                metadata.unknown = serde_unknown_policy::reject;
            }
            else if (value.text == "\"ignore\"")
            {
                metadata.unknown = serde_unknown_policy::ignore;
            }
            else if (value.text == "\"preserve\"")
            {
                metadata.unknown = serde_unknown_policy::preserve;
            }
            else
            {
                throw compile_error(value.position,
                    "serde unknown 只接受 reject、ignore 或 preserve");
            }
            seen_unknown = true;
        }
        else if (key.text == "reserved" && !seen_reserved)
        {
            (void)consume(token_kind::left_bracket, "serde reserved 需要 [");
            if (!check(token_kind::right_bracket))
            {
                do
                {
                    metadata.reserved.push_back(parse_serde_number());
                } while (match(token_kind::comma));
            }
            (void)consume(token_kind::right_bracket, "serde reserved 缺少 ]");
            seen_reserved = true;
        }
        else
        {
            throw compile_error(key.position, "重复或未知的 serde 结构体选项：" + key.text);
        }
    } while (match(token_kind::comma));
    (void)consume(token_kind::right_paren, "serde 声明缺少右括号");
    if (!seen_version)
    {
        throw compile_error(previous().position, "serde 结构体必须声明 version");
    }
    return metadata;
}

struct_field::serde_field_metadata parser::parse_serde_field_metadata()
{
    (void)advance();
    (void)consume(token_kind::left_paren, "serde 字段需要左括号");
    struct_field::serde_field_metadata metadata;
    if (check(token_kind::identifier) && current().text == "unknown")
    {
        (void)advance();
        metadata.unknown_capture = true;
    }
    else
    {
        metadata.number = parse_serde_number();
        if (match(token_kind::comma))
        {
            const auto& key = consume(token_kind::identifier,
                "serde 字段需要 default 选项");
            if (key.text != "default")
            {
                throw compile_error(key.position, "serde 字段只支持 default 选项");
            }
            (void)consume(token_kind::equal, "serde default 需要 =");
            std::string literal;
            if (match(token_kind::minus))
            {
                literal = "-";
            }
            if (!check(token_kind::integer) && !check(token_kind::floating) &&
                !check(token_kind::string_literal) &&
                !check(token_kind::keyword_true) &&
                !check(token_kind::keyword_false))
            {
                throw compile_error(current().position,
                    "serde default 只接受 int、float、bool、str 字面量");
            }
            literal += advance().text;
            metadata.default_literal = std::move(literal);
        }
    }
    (void)consume(token_kind::right_paren, "serde 字段缺少右括号");
    return metadata;
}

} // namespace tx
