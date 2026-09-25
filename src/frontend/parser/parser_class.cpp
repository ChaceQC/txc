#include "frontend/parser/parser.hpp"

#include <utility>

namespace tx
{

class_decl parser::parse_class(bool is_abstract, bool is_interface)
{
    const auto position = consume(is_interface ? token_kind::keyword_interface
                                               : token_kind::keyword_class,
                                  is_interface ? "需要 interface" : "需要 class").position;
    const auto name = consume(token_kind::identifier, "需要类名").text;
    std::vector<std::string> bases;
    if (match(token_kind::colon))
    {
        do
        {
            bases.push_back(parse_type().name);
        } while (match(token_kind::comma));
    }
    skip_newlines();
    (void)consume(token_kind::left_brace, "类需要左大括号");
    skip_newlines();
    class_decl result{name, std::move(bases), {}, {}, position, {}, {},
                      is_interface, is_abstract, name};
    member_access access = is_interface ? member_access::public_access
                                        : member_access::private_access;
    while (!check(token_kind::right_brace))
    {
        if (check(token_kind::end_of_file))
        {
            throw compile_error(current().position, "类缺少右大括号");
        }
        if (check(token_kind::keyword_public) ||
            check(token_kind::keyword_protected) ||
            check(token_kind::keyword_private))
        {
            if (is_interface)
            {
                throw compile_error(current().position,
                                    "interface 成员隐式为 public，不能使用访问修饰符");
            }
            const auto kind = advance().kind;
            access = kind == token_kind::keyword_public
                ? member_access::public_access
                : kind == token_kind::keyword_protected
                ? member_access::protected_access
                : member_access::private_access;
            (void)consume(token_kind::colon, "访问修饰符后需要冒号");
        }
        else if (check(token_kind::keyword_def) ||
                 check(token_kind::keyword_virtual) ||
                 check(token_kind::keyword_override) ||
                 check(token_kind::keyword_abstract))
        {
            const bool is_virtual = match(token_kind::keyword_virtual);
            const bool is_override = !is_virtual && match(token_kind::keyword_override);
            const bool is_abstract_method = !is_virtual && !is_override &&
                                            match(token_kind::keyword_abstract);
            if (is_interface && (is_virtual || is_override || is_abstract_method))
            {
                throw compile_error(current().position,
                                    "interface 方法直接使用 def 声明");
            }
            auto method = parse_function(is_interface || is_abstract_method, true);
            if (is_interface && method.operator_kind)
            {
                throw compile_error(method.position,
                                    "interface 暂不支持运算符成员");
            }
            method.owner_class = name;
            method.access = access;
            method.is_virtual = is_virtual || is_interface || is_abstract_method;
            method.is_override = is_override;
            method.is_abstract = is_interface || is_abstract_method;
            result.methods.push_back(std::move(method));
        }
        else
        {
            if (is_interface)
            {
                throw compile_error(current().position, "interface 不能声明字段");
            }
            const auto field = consume(token_kind::identifier, "需要类字段或方法");
            (void)consume(token_kind::colon, "字段名后需要冒号");
            result.fields.push_back({field.text, parse_type(), field.position,
                                     access, 0});
        }
        if (!check(token_kind::right_brace) && !match(token_kind::newline))
        {
            throw compile_error(current().position, "类成员结束处需要换行");
        }
        skip_newlines();
    }
    (void)advance();
    return result;
}

} // namespace tx
