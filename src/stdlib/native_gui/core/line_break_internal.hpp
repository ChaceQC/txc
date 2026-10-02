#pragma once

#include "stdlib/native_gui/core/line_break.hpp"

#include <cstdint>
#include <span>

namespace tx::ui
{
enum class line_class
{
    boundary, ak, al, ap, as, b2, ba, bb, bk, cb, cl, cm, cp, cr, eb, em, ex, gl,
    h2, h3, hl, hy, id, in, is, jl, jt, jv, lf, nl, ns, nu, op, po, pr, qu, ri,
    sp, sy, vf, vi, wj, zw, zwj
};

struct line_unit
{
    line_class kind = line_class::boundary;
    unsigned flags = 0;
    char32_t scalar = 0;
    std::size_t index = 0, space_base = 0, regional_run = 0;
    bool numeric = false;
};

struct line_context
{
    std::span<const line_unit> units;
    std::size_t right;
    const line_unit& at(std::ptrdiff_t relative) const noexcept;
};

bool allowed_line_break(const line_context& context);
}
