#include "stdlib/native_gui/core/line_break_internal.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace tx::ui
{
namespace
{
struct line_property_range
{
    char32_t first, last;
    line_class kind;
    unsigned flags;
};

#include "stdlib/native_gui/core/line_break_tables.inc"

line_unit property(char32_t scalar, std::size_t index)
{
    if (scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))
    {
        throw std::invalid_argument("断行文本包含非法 Unicode 标量");
    }
    const auto found = std::lower_bound(std::begin(line_ranges), std::end(line_ranges), scalar,
        [](const auto& range, char32_t value)
        {
            return range.last < value;
        });
    return {found->kind, found->flags, scalar, index};
}

std::vector<line_unit> effective_units(std::u32string_view text)
{
    std::vector<line_unit> units;
    units.reserve(text.size());
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        auto value = property(text[index], index);
        if (value.kind == line_class::cm || value.kind == line_class::zwj)
        {
            if (!units.empty() && !hard_line_break(units.back().scalar) &&
                units.back().kind != line_class::sp && units.back().kind != line_class::zw)
            {
                // LB9：附着的 CM/ZWJ 不参与后续上下文匹配，但原始位置仍为禁止断行。
                continue;
            }
            // LB10：剩余 CM/ZWJ 的全部相关属性均按 U+0041 处理。
            value.kind = line_class::al;
            value.scalar = U'A';
            value.flags = 0;
        }
        value.space_base = value.kind == line_class::sp && !units.empty() ? units.back().space_base : units.size();
        value.numeric = value.kind == line_class::nu ||
            ((value.kind == line_class::sy || value.kind == line_class::is) && !units.empty() && units.back().numeric);
        value.regional_run = value.kind == line_class::ri ? 1 + (units.empty() ? 0 : units.back().regional_run) : 0;
        units.push_back(value);
    }
    return units;
}
}

bool hard_line_break(char32_t scalar) noexcept
{
    return scalar == U'\r' || scalar == U'\n' || scalar == U'\v' || scalar == U'\f' ||
        scalar == 0x85 || scalar == 0x2028 || scalar == 0x2029;
}

const line_unit& line_context::at(std::ptrdiff_t relative) const noexcept
{
    static constexpr line_unit boundary;
    const auto index = static_cast<std::ptrdiff_t>(right) + relative;
    return index < 0 || static_cast<std::size_t>(index) >= units.size() ? boundary : units[index];
}

std::vector<line_break_kind> line_breaks(std::u32string_view text)
{
    const auto units = effective_units(text);
    std::vector<line_break_kind> result(text.size() + 1, line_break_kind::prohibited);
    result.back() = line_break_kind::mandatory; // LB3，包括空文本。
    for (std::size_t index = 1; index < units.size(); ++index)
    {
        const auto& a = units[index - 1];
        const auto& b = units[index];
        if (a.kind == line_class::cr && b.kind == line_class::lf)
        {
            continue;
        }
        if (hard_line_break(a.scalar)) // LB4–LB5
        {
            result[b.index] = line_break_kind::mandatory;
        }
        else if (!hard_line_break(b.scalar) && b.kind != line_class::sp && b.kind != line_class::zw) // LB6–LB7
        {
            if (units[a.space_base].kind == line_class::zw) // LB8 优先于 LB8a。
            {
                result[b.index] = line_break_kind::allowed;
            }
            else if (text[b.index - 1] != 0x200d && allowed_line_break({units, index}))
            {
                result[b.index] = line_break_kind::allowed;
            }
        }
    }
    return result;
}
}
