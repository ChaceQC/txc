#include "backend/cpp/native_gui_result.hpp"
#include "backend/cpp/native_gui_abi.hpp"
#include "backend/cpp/native_gui_extended_abi.hpp"

using namespace tx_generated;
using graphics::resource;

namespace
{
tx::ui::color color_value(std::int64_t red, std::int64_t green, std::int64_t blue, std::int64_t alpha)
{
    for (auto value : {red, green, blue, alpha})
    {
        if (value < 0 || value > 255)
        {
            native_gui::fail("invalid_argument", "颜色分量必须位于 [0,255]");
        }
    }
    return {static_cast<std::uint8_t>(red), static_cast<std::uint8_t>(green),
        static_cast<std::uint8_t>(blue), static_cast<std::uint8_t>(alpha)};
}

void coordinates(std::initializer_list<double> values)
{
    for (auto value : values)
    {
        if (!std::isfinite(value) || std::abs(value) > 1000000)
        {
            native_gui::fail("invalid_argument", "画布坐标必须有限且绝对值不超过一百万");
        }
    }
}

void append(native_gui::node& state, native_gui::canvas_item item)
{
    if (state.canvas->items.size() >= 65536)
    {
        native_gui::fail("resource_limit", "画布最多保存 65536 条绘制命令");
    }
    state.canvas->items.push_back(std::move(item));
    native_gui::dirty(state);
}

void shape(resource* control, native_gui::canvas_operation operation, double x, double y, double width, double height,
    std::int64_t red, std::int64_t green, std::int64_t blue, std::int64_t alpha)
{
    auto& state = native_gui::require_node(control);
    coordinates({x, y});
    native_gui::checked_size(width);
    native_gui::checked_size(height);
    append(state, {operation, {x, y, width, height}, color_value(red, green, blue, alpha), 1, {}});
}
}

extern "C" int txrt_native_gui_create_canvas(resource* parent, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        const auto value = native_gui::create(native_gui::require_node(parent), tx::graphics_kind::native_canvas, "");
        value->canvas = std::make_unique<native_gui::canvas_state>();
        value->height = {gui::length_mode::stretch, 1};
        return graphics::make_handle(value);
    });
}

extern "C" int txrt_native_gui_canvas_clear(resource* control, std::int64_t red, std::int64_t green,
    std::int64_t blue, std::int64_t alpha) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        const auto color = color_value(red, green, blue, alpha);
        state.canvas->items.clear();
        state.canvas->background = color;
        native_gui::dirty(state);
    });
}

extern "C" int txrt_native_gui_canvas_fill_rect(resource* control, double x, double y, double width, double height,
    std::int64_t red, std::int64_t green, std::int64_t blue, std::int64_t alpha) noexcept
{
    return native_gui::leaf_call([&]
    {
        shape(control, native_gui::canvas_operation::rectangle, x, y, width, height, red, green, blue, alpha);
    });
}

extern "C" int txrt_native_gui_canvas_fill_ellipse(resource* control, double x, double y, double width, double height,
    std::int64_t red, std::int64_t green, std::int64_t blue, std::int64_t alpha) noexcept
{
    return native_gui::leaf_call([&]
    {
        shape(control, native_gui::canvas_operation::ellipse, x, y, width, height, red, green, blue, alpha);
    });
}

extern "C" int txrt_native_gui_canvas_line(resource* control, double x1, double y1, double x2, double y2, double width,
    std::int64_t red, std::int64_t green, std::int64_t blue, std::int64_t alpha) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        coordinates({x1, y1, x2, y2});
        native_gui::checked_size(width);
        append(state, {native_gui::canvas_operation::line, {x1, y1, x2, y2}, color_value(red, green, blue, alpha), width, {}});
    });
}

extern "C" int txrt_native_gui_canvas_text(resource* control, const void* text, double x, double y, double size,
    double width, std::int64_t red, std::int64_t green, std::int64_t blue, std::int64_t alpha) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        coordinates({x, y});
        native_gui::checked_size(width);
        const auto color = color_value(red, green, blue, alpha);
        const auto& value = detail::text_value(text);
        if (!std::isfinite(size) || size <= 0 || size > 256 || value.size() > 65536)
        {
            native_gui::fail("invalid_argument", "字号必须位于 (0,256]，文本不得超过 65536 字节");
        }
        auto layout = std::make_unique<tx::ui::text_layout>(*native_gui::root_node(state).font,
            tx::ui::decode_utf8(value).scalars, size, width, true);
        append(state, {native_gui::canvas_operation::text, {x, y, width, 0}, color, 1, std::move(layout)});
    });
}
