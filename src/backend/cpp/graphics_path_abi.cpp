#include "backend/cpp/graphics_result.hpp"
#include "stdlib/graphics/windows/assets.hpp"

using namespace tx_generated;
using namespace tx_generated::graphics;

namespace
{

path& writable(resource* value, bool figure, bool adds_segment = true)
{
    auto& state = asset<path>(value);
    if (state.frozen || (figure && !state.figure))
    {
        fail("invalid_argument", "路径已冻结或尚未 move_to");
    }
    if (adds_segment && state.segments >= 65536)
    {
        fail("resource_limit", "路径超过 65536 段");
    }
    return state;
}

D2D1_POINT_2F point(double x, double y)
{
    return D2D1::Point2F(checked_number(x), checked_number(y));
}

} // namespace

extern "C" int txrt_graphics_create_path(resource* value, const void* rule, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        auto& owner = require_app(value);
        const auto& fill = detail::text_value(rule);
        if (fill != "alternate" && fill != "winding")
        {
            fail("invalid_argument", "填充规则必须为 alternate/winding");
        }
        auto state = own<path>(owner);
        ID2D1PathGeometry* raw = nullptr;
        check_hr(&owner, owner.factory->CreatePathGeometry(&raw), "创建路径");
        state->geometry.reset(raw);
        ID2D1GeometrySink* sink = nullptr;
        check_hr(&owner, raw->Open(&sink), "打开路径");
        state->sink.reset(sink);
        sink->SetFillMode(fill == "alternate" ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);
        return make_handle(state);
    });
}

extern "C" int txrt_graphics_move_to(resource* value, double x, double y) noexcept
{
    return detail::invoke_leaf([&]
    {
        const auto start = point(x, y);
        auto& state = writable(value, false);
        if (state.figure)
        {
            state.sink->EndFigure(D2D1_FIGURE_END_OPEN);
        }
        state.sink->BeginFigure(start, D2D1_FIGURE_BEGIN_FILLED);
        state.figure = true;
        ++state.segments;
    });
}

extern "C" int txrt_graphics_line_to(resource* value, double x, double y) noexcept
{
    return detail::invoke_leaf([&]
    {
        const auto end = point(x, y);
        auto& state = writable(value, true);
        state.sink->AddLine(end);
        ++state.segments;
    });
}

extern "C" int txrt_graphics_bezier_to(resource* value, double x1, double y1, double x2, double y2, double x3, double y3) noexcept
{
    return detail::invoke_leaf([&]
    {
        const auto bezier = D2D1::BezierSegment(point(x1, y1), point(x2, y2), point(x3, y3));
        auto& state = writable(value, true);
        state.sink->AddBezier(bezier);
        ++state.segments;
    });
}

extern "C" int txrt_graphics_close_path(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = writable(value, true, false);
        state.sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        state.figure = false;
    });
}

extern "C" int txrt_graphics_finish_path(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = writable(value, false, false);
        if (state.figure)
        {
            state.sink->EndFigure(D2D1_FIGURE_END_OPEN);
            state.figure = false;
        }
        check_hr(state.owner.lock().get(), state.sink->Close(), "冻结路径");
        state.sink.reset();
        state.frozen = true;
    });
}

extern "C" int txrt_graphics_draw_path(resource* canvas_value, resource* path_value, resource* brush_value, double width) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(canvas_value);
        auto& geometry = asset<path>(path_value);
        same_owner(state, geometry);
        const auto stroke = checked_width(width);
        if (!geometry.frozen)
        {
            fail("invalid_argument", "绘制要求已冻结路径");
        }
        auto paint = native_brush(state, asset<brush>(brush_value));
        state.target->DrawGeometry(geometry.geometry.get(), paint.get(), stroke);
    });
}

extern "C" int txrt_graphics_fill_path(resource* canvas_value, resource* path_value, resource* brush_value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(canvas_value);
        auto& geometry = asset<path>(path_value);
        same_owner(state, geometry);
        if (!geometry.frozen)
        {
            fail("invalid_argument", "绘制要求已冻结路径");
        }
        auto paint = native_brush(state, asset<brush>(brush_value));
        state.target->FillGeometry(geometry.geometry.get(), paint.get());
    });
}
