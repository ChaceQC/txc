#pragma once

#include "frontend/ast/ast.hpp"

#include <functional>

namespace tx
{

// 受检 AST 的保守执行计划：只读 record 借用，标量局部可修改，
// 不产生逃逸引用。错误仍由原发射器处理，不代表函数不会失败。
struct static_function_rules
{
    std::function<bool(const value_type&)> record;
    std::function<bool(const call_expression&)> call;
    std::function<bool(const operator_binding&)> operation;
};

[[nodiscard]] bool analyze_static_function(const function_decl& function,
    const static_function_rules& rules);

} // namespace tx
