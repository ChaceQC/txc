#include "stdlib/native_gui/state.hpp"

#include <algorithm>
#include <chrono>

namespace tx_generated::native_gui
{
namespace
{
using tx::ui::color;
using tx::ui::rect;
using tx::ui::rasterizer;

struct palette
{
    color background, panel, foreground, muted, accent, hover, pressed, border;
};

palette colors(bool dark)
{
    if (dark)
    {
        return {{18, 23, 33, 255}, {28, 36, 51, 255}, {235, 242, 255, 255},
            {117, 133, 158, 255}, {94, 158, 255, 255}, {46, 64, 92, 255},
            {59, 84, 122, 255}, {69, 84, 110, 255}};
    }
    return {{240, 245, 252, 255}, {255, 255, 255, 255}, {31, 43, 64, 255},
        {133, 143, 163, 255}, {48, 92, 194, 255}, {227, 237, 255, 255},
        {201, 219, 250, 255}, {204, 217, 235, 255}};
}

rect rectangle_of(rectangle value)
{
    return {value.x, value.y, value.width, value.height};
}

void control_background(node& state, node& root, rasterizer& painter, const palette& theme)
{
    const bool enabled = available(state);
    const bool hovered = root.hovered.lock().get() == &state;
    const bool pressed = root.pressed.lock().get() == &state && (hovered || root.keyboard_pressed);
    const auto bounds = rectangle_of(state.bounds);
    if (state.kind == tx::graphics_kind::native_panel && !state.parent.expired())
    {
        painter.fill_rounded_rect(bounds, 12, theme.panel);
    }
    else if (state.kind == tx::graphics_kind::native_button)
    {
        const auto fill = !enabled ? theme.background : pressed ? theme.pressed : hovered ? theme.hover : theme.panel;
        painter.fill_rounded_rect(bounds, 8, theme.border);
        painter.fill_rounded_rect({bounds.x + 1, bounds.y + 1, std::max(0.0, bounds.width - 2),
            std::max(0.0, bounds.height - 2)}, 7, fill);
    }
    else if (state.editor)
    {
        const bool focused = root.focused.lock().get() == &state && root.window_focused;
        painter.fill_rounded_rect(bounds, 6, focused ? theme.accent : theme.border);
        painter.fill_rounded_rect({bounds.x + 1, bounds.y + 1, std::max(0.0, bounds.width - 2),
            std::max(0.0, bounds.height - 2)}, 5, enabled ? theme.panel : theme.background);
    }
    else if (state.kind == tx::graphics_kind::native_check_box)
    {
        const rect box{bounds.x + 2, bounds.y + (bounds.height - 22) / 2, 22, 22};
        const auto fill = !enabled ? theme.muted : state.checked ? theme.accent : hovered ? theme.hover : theme.panel;
        painter.fill_rounded_rect(box, 5, theme.border);
        painter.fill_rounded_rect({box.x + 1, box.y + 1, 20, 20}, 4, fill);
        if (state.checked)
        {
            painter.line({box.x + 5, box.y + 11}, {box.x + 9, box.y + 15}, 2, theme.panel);
            painter.line({box.x + 9, box.y + 15}, {box.x + 17, box.y + 7}, 2, theme.panel);
        }
    }
    else if (state.kind == tx::graphics_kind::native_radio_button)
    {
        const rect box{bounds.x + 2, bounds.y + (bounds.height - 22) / 2, 22, 22};
        painter.fill_ellipse(box, theme.border);
        painter.fill_ellipse({box.x + 1, box.y + 1, 20, 20}, hovered ? theme.hover : theme.panel);
        if (state.checked)
        {
            painter.fill_ellipse({box.x + 6, box.y + 6, 10, 10}, enabled ? theme.accent : theme.muted);
        }
    }
    else if (state.kind == tx::graphics_kind::native_progress_bar)
    {
        painter.fill_rounded_rect(bounds, bounds.height / 2, theme.border);
        const double ratio = state.maximum == state.minimum ? 1 : (state.value - state.minimum) / (state.maximum - state.minimum);
        if (state.indeterminate)
        {
            const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            const double track = std::max(0.0, bounds.width - 16), phase = (millis % 1400) / 1400.0;
            const double left = std::max(0.0, phase * 1.3 - 0.3) * track;
            const double right = std::min(1.0, phase * 1.3) * track;
            painter.fill_rounded_rect({bounds.x + 8 + left, bounds.y + 2, right - left,
                std::max(0.0, bounds.height - 4)}, bounds.height / 2, enabled ? theme.accent : theme.muted);
        }
        else if (ratio > 0)
        {
            painter.fill_rounded_rect({bounds.x, bounds.y, bounds.width * ratio, bounds.height},
                bounds.height / 2, enabled ? theme.accent : theme.muted);
        }
    }
    else if (state.kind == tx::graphics_kind::native_slider)
    {
        const double width = std::max(0.0, bounds.width - 24), y = bounds.y + bounds.height / 2;
        const double ratio = (state.value - state.minimum) / (state.maximum - state.minimum);
        const double x = bounds.x + 12 + width * ratio;
        painter.fill_rounded_rect({bounds.x + 12, y - 3, width, 6}, 3, theme.border);
        painter.fill_rounded_rect({bounds.x + 12, y - 3, width * ratio, 6}, 3, enabled ? theme.accent : theme.muted);
        const double radius = pressed ? 10 : 8;
        painter.fill_ellipse({x - radius, y - radius, 2 * radius, 2 * radius}, enabled ? theme.accent : theme.muted);
    }
}

void draw_node(node& state, node& root, rasterizer& painter, const palette& theme)
{
    if (!state.visible || state.clip.width <= 0 || state.clip.height <= 0)
    {
        return;
    }
    painter.push_clip(rectangle_of(state.clip));
    control_background(state, root, painter, theme);
    if (state.editor)
    {
        draw_editor(state, painter, available(state) ? theme.foreground : theme.muted, theme.accent);
    }
    else if (state.text_layout)
    {
        double x = state.bounds.x;
        if (state.kind == tx::graphics_kind::native_button)
        {
            x += std::max(12.0, (state.bounds.width - state.text_layout->width()) / 2);
        }
        else if (state.kind == tx::graphics_kind::native_check_box || state.kind == tx::graphics_kind::native_radio_button)
        {
            x += 36;
        }
        const double y = state.bounds.y + std::max(0.0, (state.bounds.height - state.text_layout->height()) / 2);
        state.text_layout->draw(painter, {x, y}, available(state) ? theme.foreground : theme.muted);
    }
    if (!state.editor && root.window_focused && root.focused.lock().get() == &state && available(state))
    {
        const auto& b = state.bounds;
        painter.line({b.x + 4, b.y + 3}, {b.x + b.width - 4, b.y + 3}, 2, theme.accent);
        painter.line({b.x + 4, b.y + b.height - 3}, {b.x + b.width - 4, b.y + b.height - 3}, 2, theme.accent);
    }
    for (const auto& child : state.children)
    {
        draw_node(*child, root, painter, theme);
    }
    painter.pop_clip();
}
}

tx::ui::pixel_buffer render(node& state)
{
    auto& root = root_node(state);
    if (&root != &state)
    {
        fail("invalid_argument", "渲染只接受根面板");
    }
    require_idle(root);
    auto& window = owner_window(root);
    layout(root);
    tx::ui::pixel_buffer image(window.width, window.height);
    const auto theme = colors(root.dark);
    image.clear(theme.background);
    rasterizer painter(image);
    painter.set_scale(window.dpi / 96.0);
    draw_node(root, root, painter, theme);
    return image;
}

void paint(node& state)
{
    auto& window = owner_window(state);
    require_idle(state);
    if (!window.visible || window.minimized || !window.width || !window.height)
    {
        return;
    }
    const auto image = render(state);
    window.host->present(image);
    window.repaint = false;
}
}
