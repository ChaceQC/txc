#pragma once

#include <string_view>
#include <vector>

namespace tx::ui
{
enum class line_break_kind
{
    prohibited, allowed, mandatory
};

// 每个 Unicode 标量之前的位置，末项为文本终点；不是 UTF-8 字节偏移。
std::vector<line_break_kind> line_breaks(std::u32string_view text);
bool hard_line_break(char32_t scalar) noexcept;
}
