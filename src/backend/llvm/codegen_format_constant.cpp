#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{
namespace
{

bool unchanged_body(const std::vector<stmt_ptr>& body, const variable_declaration& candidate);

bool unchanged_statement(const statement& item, const variable_declaration& candidate)
{
    const auto& name = candidate.name;
    if (const auto* declaration = std::get_if<variable_declaration>(&item.data))
    {
        return declaration == &candidate || declaration->name != name;
    }
    if (const auto* assignment = std::get_if<variable_assignment>(&item.data))
    {
        const auto* target = std::get_if<name_reference>(&assignment->target->data);
        return !target || target->name != name;
    }
    if (const auto* unpack = std::get_if<unpack_assignment>(&item.data))
    {
        return std::find(unpack->names.begin(), unpack->names.end(), name) == unpack->names.end();
    }
    if (const auto* branch = std::get_if<if_statement>(&item.data))
    {
        return unchanged_body(branch->then_body, candidate) &&
            unchanged_body(branch->else_body, candidate);
    }
    if (const auto* loop = std::get_if<while_statement>(&item.data))
    {
        return unchanged_body(loop->body, candidate);
    }
    if (const auto* loop = std::get_if<for_loop>(&item.data))
    {
        return loop->name != name && unchanged_body(loop->body, candidate);
    }
    if (const auto* loop = std::get_if<for_each>(&item.data))
    {
        return loop->name != name && unchanged_body(loop->body, candidate);
    }
    if (const auto* guarded = std::get_if<try_statement>(&item.data))
    {
        return unchanged_body(guarded->body, candidate) &&
            std::all_of(guarded->handlers.begin(), guarded->handlers.end(),
                [&](const exception_clause& handler)
                {
                    return handler.name != name && unchanged_body(handler.body, candidate);
                });
    }
    return true;
}

bool unchanged_body(const std::vector<stmt_ptr>& body, const variable_declaration& candidate)
{
    return std::all_of(body.begin(), body.end(), [&](const stmt_ptr& item)
    {
        return unchanged_statement(*item, candidate);
    });
}

} // namespace

bool llvm_code_generator::immutable_format_local(const variable_declaration& declaration) const
{
    // str 为不可变值，调用和别名不能改写局部槽；任意分支赋值、解包或同名遮蔽均放弃传播。
    return declaration.initializer && current_function_body_ &&
        unchanged_body(*current_function_body_, declaration);
}

std::optional<std::string> llvm_code_generator::constant_format_text(const expression& item) const
{
    if (const auto* literal = std::get_if<string_literal>(&item.data))
    {
        return decode_string_literal(literal->text);
    }
    if (const auto* reference = std::get_if<name_reference>(&item.data))
    {
        return find_variable(reference->name, item.position).constant_text;
    }
    return std::nullopt;
}

} // namespace tx
