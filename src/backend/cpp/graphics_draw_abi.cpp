#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/graphics/windows/state.hpp"

#include <algorithm>

using namespace tx_generated;
using namespace tx_generated::graphics;

namespace
{

enum class shape
{
    rect, ellipse, rounded_rect
};

template<shape kind, bool filled>
void draw_shape(resource* value, rectangle rect, rgba color, double width,
    double radius_x = 0, double radius_y = 0)
{
    auto& state = require_canvas(value);
    const auto bounds = checked_rect(rect);
    auto* brush = color_brush(state, color);
    const auto stroke = filled ? 1.0f : checked_width(width);
    const auto rx = checked_number(radius_x);
    const auto ry = checked_number(radius_y);
    if (rx < 0 || ry < 0)
    {
        fail("invalid_argument", "圆角半径不能为负");
    }
    if (rect.width == 0 || rect.height == 0)
    {
        return;
    }
    if constexpr (kind == shape::rect)
    {
        if constexpr (filled)
        {
            state.target->FillRectangle(bounds, brush);
        }
        else
        {
            state.target->DrawRectangle(bounds, brush, stroke);
        }
    }
    else if constexpr (kind == shape::ellipse)
    {
        const auto ellipse = D2D1::Ellipse(D2D1::Point2F(
            checked_number(rect.x + rect.width / 2), checked_number(rect.y + rect.height / 2)),
            checked_number(rect.width / 2), checked_number(rect.height / 2));
        if constexpr (filled)
        {
            state.target->FillEllipse(ellipse, brush);
        }
        else
        {
            state.target->DrawEllipse(ellipse, brush, stroke);
        }
    }
    else
    {
        const auto rounded = D2D1::RoundedRect(bounds,
            std::min(rx, checked_number(rect.width / 2)), std::min(ry, checked_number(rect.height / 2)));
        if constexpr (filled)
        {
            state.target->FillRoundedRectangle(rounded, brush);
        }
        else
        {
            state.target->DrawRoundedRectangle(rounded, brush, stroke);
        }
    }
}

} // namespace

extern "C" int txrt_graphics_clear(resource* value,
    double red, double green, double blue, double alpha) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        state.target->Clear(checked_color({red, green, blue, alpha}));
    });
}

extern "C" int txrt_graphics_draw_line(resource* value,
    double x1, double y1, double x2, double y2,
    double red, double green, double blue, double alpha, double width) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        const auto start = D2D1::Point2F(checked_number(x1), checked_number(y1));
        const auto end = D2D1::Point2F(checked_number(x2), checked_number(y2));
        state.target->DrawLine(start, end, color_brush(state, {red, green, blue, alpha}),
            checked_width(width));
    });
}

extern "C" int txrt_graphics_draw_rect(resource* value,
    double x, double y, double width, double height,
    double red, double green, double blue, double alpha, double stroke) noexcept
{
    return detail::invoke_leaf([&]
    {
        draw_shape<shape::rect, false>(value, {x, y, width, height}, {red, green, blue, alpha}, stroke);
    });
}

extern "C" int txrt_graphics_fill_rect(resource* value,
    double x, double y, double width, double height,
    double red, double green, double blue, double alpha) noexcept
{
    return detail::invoke_leaf([&]
    {
        draw_shape<shape::rect, true>(value, {x, y, width, height}, {red, green, blue, alpha}, 0);
    });
}

extern "C" int txrt_graphics_draw_ellipse(resource* value,
    double x, double y, double width, double height,
    double red, double green, double blue, double alpha, double stroke) noexcept
{
    return detail::invoke_leaf([&]
    {
        draw_shape<shape::ellipse, false>(value, {x, y, width, height}, {red, green, blue, alpha}, stroke);
    });
}

extern "C" int txrt_graphics_fill_ellipse(resource* value,
    double x, double y, double width, double height,
    double red, double green, double blue, double alpha) noexcept
{
    return detail::invoke_leaf([&]
    {
        draw_shape<shape::ellipse, true>(value, {x, y, width, height}, {red, green, blue, alpha}, 0);
    });
}

extern "C" int txrt_graphics_draw_rounded_rect(resource* value,
    double x, double y, double width, double height, double radius_x, double radius_y,
    double red, double green, double blue, double alpha, double stroke) noexcept
{
    return detail::invoke_leaf([&]
    {
        draw_shape<shape::rounded_rect, false>(value, {x, y, width, height},
            {red, green, blue, alpha}, stroke, radius_x, radius_y);
    });
}

extern "C" int txrt_graphics_fill_rounded_rect(resource* value,
    double x, double y, double width, double height, double radius_x, double radius_y,
    double red, double green, double blue, double alpha) noexcept
{
    return detail::invoke_leaf([&]
    {
        draw_shape<shape::rounded_rect, true>(value, {x, y, width, height},
            {red, green, blue, alpha}, 0, radius_x, radius_y);
    });
}
