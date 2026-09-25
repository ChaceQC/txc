#pragma once

#include "frontend/ast/ast.hpp"

namespace tx
{

[[nodiscard]] bool same_parameter_types(const function_decl& left,
                                        const function_decl& right);
[[nodiscard]] bool same_class_layout(const class_decl& left,
                                     const class_decl& right);

} // namespace tx
