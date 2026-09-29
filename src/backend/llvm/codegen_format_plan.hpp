#pragma once

#include "common/format_spec.hpp"
#include "frontend/ast/ast.hpp"

namespace tx
{

struct static_format_part
{
    std::string literal;
    std::optional<std::size_t> argument;
    format_spec spec;
    char conversion = 0;
    std::string tail = {};
};

std::optional<std::vector<static_format_part>> plan_static_format(
    std::string_view text, const call_expression& call);
bool plain_format_part(const static_format_part& part, const value_type& type);

} // namespace tx
