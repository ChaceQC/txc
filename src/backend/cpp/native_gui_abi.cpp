#include "backend/cpp/native_gui_abi.hpp"
#include "backend/cpp/native_gui_result.hpp"
#include "backend/cpp/graphics_result.hpp"
#include "stdlib/native_gui/state.hpp"
#include "stdlib/native_gui/core/image_io.hpp"

using namespace tx_generated;
using graphics::resource;

namespace
{
template<tx::graphics_kind kind>
int create_control(resource* parent, const void* text, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create(native_gui::require_node(parent), kind,
            text ? detail::text_value(text) : std::string{}));
    });
}

int set_layout(resource* panel, double padding, double gap, bool row) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(panel);
        native_gui::require_idle(state);
        native_gui::checked_size(padding);
        native_gui::checked_size(gap);
        state.padding = padding;
        state.gap = gap;
        state.layout = row ? native_gui::layout_mode::row : native_gui::layout_mode::column;
        native_gui::dirty(state);
    });
}
}

extern "C" int txrt_native_gui_root(resource* window, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::root(native_gui::require_window(window)));
    });
}

extern "C" int txrt_native_gui_create_panel(resource* parent, const char* type, void** result) noexcept
{
    return create_control<tx::graphics_kind::native_panel>(parent, nullptr, type, result);
}

extern "C" int txrt_native_gui_create_label(resource* parent, const void* text, const char* type, void** result) noexcept
{
    return create_control<tx::graphics_kind::native_label>(parent, text, type, result);
}

extern "C" int txrt_native_gui_create_button(resource* parent, const void* text, const char* type, void** result) noexcept
{
    return create_control<tx::graphics_kind::native_button>(parent, text, type, result);
}

extern "C" int txrt_native_gui_create_check_box(resource* parent, const void* text, const char* type, void** result) noexcept
{
    return create_control<tx::graphics_kind::native_check_box>(parent, text, type, result);
}

extern "C" int txrt_native_gui_set_column(resource* panel, double padding, double gap) noexcept
{
    return set_layout(panel, padding, gap, false);
}

extern "C" int txrt_native_gui_set_row(resource* panel, double padding, double gap) noexcept
{
    return set_layout(panel, padding, gap, true);
}

extern "C" int txrt_native_gui_set_theme(resource* panel, const void* theme) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(panel);
        native_gui::require_idle(state);
        const auto& name = detail::text_value(theme);
        if (name != "light" && name != "dark")
        {
            native_gui::fail("invalid_argument", "自绘主题只支持 light/dark");
        }
        native_gui::root_node(state).dark = name == "dark";
        native_gui::dirty(state);
    });
}

extern "C" int txrt_native_gui_paint(resource* panel, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        native_gui::paint(native_gui::require_node(panel));
        return {};
    });
}

extern "C" int txrt_native_gui_save_bitmap(resource* panel, const void* path, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        const auto image = native_gui::render(native_gui::require_node(panel));
        const auto& name = detail::text_value(path);
        const auto file = std::filesystem::path(std::u8string(name.begin(), name.end()));
        tx::ui::save_bitmap(image, file);
        return {};
    });
}

extern "C" int txrt_native_gui_set_checked(resource* control, bool checked) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        native_gui::require_idle(state);
        if (state.checked != checked)
        {
            state.checked = checked;
            ++state.revision;
            native_gui::dirty(state);
        }
    });
}

extern "C" int txrt_native_gui_checked(resource* control, bool* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_node(control).checked;
    });
}
