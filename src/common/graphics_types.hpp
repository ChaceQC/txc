#pragma once

#include <string_view>

namespace tx
{

// 与不透明运行时资源的类型标签共用；不能由导入别名推断资源身份。
enum class graphics_kind
{
    app, window, canvas, image, surface, path, brush, font, text_layout,
    control, menu, unknown
};

inline graphics_kind graphics_type_kind(std::string_view name) noexcept
{
    constexpr std::string_view names[] = {"graphics_app", "graphics_window",
        "graphics_canvas", "graphics_image", "graphics_surface", "graphics_path",
        "graphics_brush", "graphics_font", "graphics_text_layout", "gui_control", "gui_menu"};
    for (int index = 0; index < 11; ++index)
    {
        if (name == names[index])
        {
            return static_cast<graphics_kind>(index);
        }
    }
    return graphics_kind::unknown;
}

} // namespace tx
