#pragma once

#include <cstdint>
#include <string_view>

namespace tx
{

// 固定布局供 LLVM 常量和运行时共同使用；-1 表示没有精度限制。
struct format_spec
{
    std::int64_t fill = ' ';
    std::int64_t align = 0;
    std::int64_t sign = 0;
    std::int64_t type = 0;
    std::int64_t zero = 0;
    std::uint64_t width = 0;
    std::int64_t precision = -1;
};

static_assert(sizeof(format_spec) == 56);
format_spec parse_format_spec(std::string_view source);

} // namespace tx
