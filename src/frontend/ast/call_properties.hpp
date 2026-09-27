#pragma once

#include "frontend/ast/ast.hpp"

namespace tx
{

[[nodiscard]] bool scalar_container_type(const value_type& type);
[[nodiscard]] call_effects container_call_effects(
    const value_type& type, std::string_view operation);
[[nodiscard]] bool stable_borrow_expression(const expression& item);

} // namespace tx
