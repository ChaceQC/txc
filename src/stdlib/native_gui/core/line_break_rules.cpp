#include "stdlib/native_gui/core/line_break_internal.hpp"

#include <initializer_list>
#include <optional>

namespace tx::ui
{
namespace
{
using enum line_class;

bool one_of(line_class value, std::initializer_list<line_class> set)
{
    for (const auto item : set)
    {
        if (value == item)
        {
            return true;
        }
    }
    return false;
}

bool east(const line_unit& value)
{
    return (value.flags & 4) != 0;
}

bool letter(line_class value)
{
    return value == al || value == hl;
}

std::optional<bool> punctuation(const line_context& context)
{
    const auto& a = context.at(-1);
    const auto& b = context.at(0);
    const auto& after = context.at(1);
    const auto base_index = a.space_base;
    const auto& base = context.units[base_index];
    const auto prior = base_index ? context.units[base_index - 1].kind : boundary;
    if (a.kind == wj || b.kind == wj || a.kind == gl ||
        (b.kind == gl && !one_of(a.kind, {sp, ba, hy}))) // LB11–LB12a
    {
        return false;
    }
    if (one_of(b.kind, {cl, cp, ex, sy}) || base.kind == op) // LB13–LB14
    {
        return false;
    }
    if (base.kind == qu && (base.flags & 1) && one_of(prior, {boundary, bk, cr, lf, nl, op, qu, gl, sp, zw})) // LB15a
    {
        return false;
    }
    if (b.kind == qu && (b.flags & 2) && one_of(after.kind, {boundary, sp, gl, wj, cl, qu, cp, ex, is, sy, bk, cr, lf, nl, zw})) // LB15b
    {
        return false;
    }
    if (a.kind == sp && b.kind == is && after.kind == nu) // LB15c
    {
        return true;
    }
    if (b.kind == is || (one_of(base.kind, {cl, cp}) && b.kind == ns) || (base.kind == b2 && b.kind == b2)) // LB15d–LB17
    {
        return false;
    }
    if (a.kind == sp) // LB18
    {
        return true;
    }
    if ((b.kind == qu && !(b.flags & 1)) || (a.kind == qu && !(a.flags & 2))) // LB19
    {
        return false;
    }
    if ((b.kind == qu && (!east(a) || !east(after))) ||
        (a.kind == qu && (!east(b) || !east(context.at(-2))))) // LB19a
    {
        return false;
    }
    if (a.kind == cb || b.kind == cb) // LB20
    {
        return true;
    }
    return {};
}

bool words_and_numbers(const line_context& context)
{
    const auto& a = context.at(-1);
    const auto& b = context.at(0);
    const auto& before = context.at(-2);
    const auto& after = context.at(1);
    if ((a.kind == hy || a.scalar == 0x2010) && b.kind == al &&
        one_of(before.kind, {boundary, bk, cr, lf, nl, sp, zw, cb, gl})) // LB20a
    {
        return false;
    }
    if (one_of(b.kind, {ba, hy, ns}) || a.kind == bb || b.kind == in) // LB21/LB22
    {
        return false;
    }
    if (before.kind == hl && (a.kind == hy || (a.kind == ba && !east(a))) && b.kind != hl) // LB21a
    {
        return false;
    }
    if (a.kind == sy && b.kind == hl) // LB21b
    {
        return false;
    }
    if ((letter(a.kind) && b.kind == nu) || (a.kind == nu && letter(b.kind))) // LB23
    {
        return false;
    }
    if ((a.kind == pr && one_of(b.kind, {id, eb, em})) || (one_of(a.kind, {id, eb, em}) && b.kind == po)) // LB23a
    {
        return false;
    }
    if ((one_of(a.kind, {pr, po}) && letter(b.kind)) || (letter(a.kind) && one_of(b.kind, {pr, po}))) // LB24
    {
        return false;
    }
    // LB25 的可变长数值前缀在预处理阶段归约，避免长数字触发二次扫描。
    if (one_of(b.kind, {pr, po}) && (a.numeric || (one_of(a.kind, {cl, cp}) && before.numeric)))
    {
        return false;
    }
    if (one_of(a.kind, {pr, po}) && (b.kind == nu || (b.kind == op &&
        (after.kind == nu || (after.kind == is && context.at(2).kind == nu)))))
    {
        return false;
    }
    if (b.kind == nu && (a.kind == hy || a.kind == is || a.numeric))
    {
        return false;
    }
    return true;
}

bool syllables(const line_context& context)
{
    const auto& a = context.at(-1);
    const auto& b = context.at(0);
    if ((a.kind == jl && one_of(b.kind, {jl, jv, h2, h3})) ||
        (one_of(a.kind, {jv, h2}) && one_of(b.kind, {jv, jt})) || (one_of(a.kind, {jt, h3}) && b.kind == jt)) // LB26
    {
        return false;
    }
    if ((one_of(a.kind, {jl, jv, jt, h2, h3}) && b.kind == po) ||
        (a.kind == pr && one_of(b.kind, {jl, jv, jt, h2, h3}))) // LB27
    {
        return false;
    }
    if (letter(a.kind) && letter(b.kind)) // LB28
    {
        return false;
    }
    const auto brahmic = [](const line_unit& value)
    {
        return value.kind == ak || value.kind == as || value.scalar == 0x25cc;
    };
    if ((a.kind == ap && brahmic(b)) || (brahmic(a) && one_of(b.kind, {vf, vi})) ||
        (brahmic(context.at(-2)) && a.kind == vi && (b.kind == ak || b.scalar == 0x25cc)) ||
        (brahmic(a) && brahmic(b) && context.at(1).kind == vf)) // LB28a
    {
        return false;
    }
    if (a.kind == is && letter(b.kind)) // LB29
    {
        return false;
    }
    if (((letter(a.kind) || a.kind == nu) && b.kind == op && !east(b)) ||
        (a.kind == cp && !east(a) && (letter(b.kind) || b.kind == nu))) // LB30
    {
        return false;
    }
    if (a.kind == ri && b.kind == ri && a.regional_run % 2 == 1) // LB30a
    {
        return false;
    }
    return !(b.kind == em && (a.kind == eb || (a.flags & 24) == 24)); // LB30b–LB31
}
}

bool allowed_line_break(const line_context& context)
{
    if (const auto result = punctuation(context))
    {
        return *result;
    }
    return words_and_numbers(context) && syllables(context);
}
}
