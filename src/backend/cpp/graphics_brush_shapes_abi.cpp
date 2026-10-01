#include "backend/cpp/graphics_result.hpp"
#include "stdlib/graphics/windows/assets.hpp"
#include <algorithm>

using namespace tx_generated;
using namespace tx_generated::graphics;

extern "C" int txrt_graphics_draw_line_brush(resource* value, double x, double y, double w, double h, resource* brush_value, double width) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        auto paint = native_brush(state, asset<brush>(brush_value));
        const auto stroke = checked_width(width);
        state.target->DrawLine(D2D1::Point2F(checked_number(x), checked_number(y)), D2D1::Point2F(checked_number(w), checked_number(h)), paint.get(), stroke);
    });
}

extern "C" int txrt_graphics_draw_rect_brush(resource* value, double x, double y, double w, double h, resource* brush_value, double width) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        auto paint = native_brush(state, asset<brush>(brush_value));
        const auto stroke = checked_width(width);
        const auto bounds = checked_rect({x, y, w, h});
        state.target->DrawRectangle(bounds, paint.get(), stroke);
    });
}

extern "C" int txrt_graphics_fill_rect_brush(resource* value, double x, double y, double w, double h, resource* brush_value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        auto paint = native_brush(state, asset<brush>(brush_value));
        const auto bounds = checked_rect({x, y, w, h});
        state.target->FillRectangle(bounds, paint.get());
    });
}

extern "C" int txrt_graphics_draw_ellipse_brush(resource* value, double x, double y, double w, double h, resource* brush_value, double width) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        auto paint = native_brush(state, asset<brush>(brush_value));
        const auto stroke = checked_width(width);
        const auto bounds = checked_rect({x, y, w, h});
        const auto shape = D2D1::Ellipse(D2D1::Point2F(checked_number(x + w / 2), checked_number(y + h / 2)), checked_number(w / 2), checked_number(h / 2));
        state.target->DrawEllipse(shape, paint.get(), stroke);
    });
}

extern "C" int txrt_graphics_fill_ellipse_brush(resource* value, double x, double y, double w, double h, resource* brush_value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        auto paint = native_brush(state, asset<brush>(brush_value));
        const auto bounds = checked_rect({x, y, w, h});
        const auto shape = D2D1::Ellipse(D2D1::Point2F(checked_number(x + w / 2), checked_number(y + h / 2)), checked_number(w / 2), checked_number(h / 2));
        state.target->FillEllipse(shape, paint.get());
    });
}

extern "C" int txrt_graphics_draw_rounded_rect_brush(resource* value, double x, double y, double w, double h, double radius_x, double radius_y, resource* brush_value, double width) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        auto paint = native_brush(state, asset<brush>(brush_value));
        const auto stroke = checked_width(width);
        const auto bounds = checked_rect({x, y, w, h});
        const auto rx = checked_number(radius_x), ry = checked_number(radius_y);
        if (rx < 0 || ry < 0)
        {
            fail("invalid_argument", "圆角半径不能为负");
        }
        const auto shape = D2D1::RoundedRect(bounds, std::min(rx, checked_number(w / 2)), std::min(ry, checked_number(h / 2)));
        state.target->DrawRoundedRectangle(shape, paint.get(), stroke);
    });
}

extern "C" int txrt_graphics_fill_rounded_rect_brush(resource* value, double x, double y, double w, double h, double radius_x, double radius_y, resource* brush_value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        auto paint = native_brush(state, asset<brush>(brush_value));
        const auto bounds = checked_rect({x, y, w, h});
        const auto rx = checked_number(radius_x), ry = checked_number(radius_y);
        if (rx < 0 || ry < 0)
        {
            fail("invalid_argument", "圆角半径不能为负");
        }
        const auto shape = D2D1::RoundedRect(bounds, std::min(rx, checked_number(w / 2)), std::min(ry, checked_number(h / 2)));
        state.target->FillRoundedRectangle(shape, paint.get());
    });
}
