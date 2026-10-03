#include "stdlib/native_gui/theme.hpp"

#include <chrono>
#include <cmath>

namespace tx_generated::native_gui
{
namespace
{
void invalidate_text(node& state)
{
    state.text_layout.reset();
    state.text_width = -1;
    if (state.data)
    {
        state.data->text_cache.clear();
    }
    for (const auto& child : state.children)
    {
        invalidate_text(*child);
    }
}
}

void set_theme(node& state, const std::string& name)
{
    if (name != "light" && name != "dark" && name != "system" && name != "high_contrast")
    {
        fail("invalid_argument", "主题必须为 light/dark/system/high_contrast");
    }
    auto& root = root_node(state);
    root.theme = name;
    root.dark = name == "dark";
    dirty(root);
}

void set_font_size(node& state, double size)
{
    if (!std::isfinite(size) || size < 8 || size > 72)
    {
        fail("invalid_argument", "GUI 字号必须在 8–72 之间");
    }
    auto& root = root_node(state);
    root.font_size = size;
    invalidate_text(root);
    dirty(root);
}

void set_ui_scale(node& state, double scale)
{
    if (!std::isfinite(scale) || scale < 0.5 || scale > 4)
    {
        fail("invalid_argument", "GUI 缩放必须在 0.5–4 之间");
    }
    auto& root = root_node(state);
    root.ui_scale = scale;
    owner_window(root).dpi = owner_window(root).host->scale() * 96 * scale;
    invalidate_text(root);
    dirty(root);
}

palette theme_palette(node& root)
{
    const auto owner = owner_window(root).owner.lock();
    const auto system = owner->theme ? *owner->theme : system_theme{};
    const bool high = root.theme == "high_contrast" || system.high_contrast;
    const bool dark = root.theme == "dark" || (root.theme == "system" && system.dark);
    root.dark = dark;
    if (high)
    {
        return {system.background, system.background, system.foreground, system.muted,
            system.accent, system.background, system.background, system.foreground};
    }
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

void poll_theme(app& state)
{
    const auto now = std::chrono::steady_clock::now();
    if (now < state.theme_poll)
    {
        return;
    }
    state.theme_poll = now + std::chrono::seconds(1);
    const auto current = read_system_theme();
    // 对颜色逐通道比较，避免依赖像素结构的填充字节。
    auto signature = [](const system_theme& value)
    {
        return std::tuple(value.dark, value.high_contrast,
            value.background.red, value.background.green, value.background.blue,
            value.foreground.red, value.foreground.green, value.foreground.blue,
            value.accent.red, value.accent.green, value.accent.blue,
            value.muted.red, value.muted.green, value.muted.blue);
    };
    if (state.theme && signature(*state.theme) == signature(current))
    {
        return;
    }
    state.theme = std::make_shared<system_theme>(current);
    for (const auto& window : state.windows)
    {
        if (!window->closed)
        {
            window->repaint = true;
            enqueue(*window, {"theme_changed"});
        }
    }
}
}
