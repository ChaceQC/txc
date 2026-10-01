#pragma once

namespace tx
{

// 数值是编译器和标准库共享的 C ABI 契约；捕获目标在编译期确定。
enum class error_kind : int
{
    none = 0,
    runtime = 1,
    parse = 2,
    io = 3,
    process = 4,
    database = 5,
    security = 6,
    cancelled = 7,
    graphics = 8
};

} // namespace tx
