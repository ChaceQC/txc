#include "backend/cpp/native_gui_system_abi.hpp"
#include "backend/cpp/native_gui_result.hpp"
#include "stdlib/native_gui/commands.hpp"
#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/theme.hpp"
#include "stdlib/native_gui/accessibility.hpp"

using namespace tx_generated;
using graphics::resource;

extern "C" int txrt_native_gui_create_command(resource* window, const void* text, const void* accelerator, bool checkable, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create_command(native_gui::require_window(window), detail::text_value(text), detail::text_value(accelerator), checkable));
    });
}

extern "C" int txrt_native_gui_id_command(resource* command, std::int64_t* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_command(command).id;
    });
}

extern "C" int txrt_native_gui_close_command(resource* command) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::close_command(native_gui::require_command(command, true));
    });
}

extern "C" int txrt_native_gui_set_command_state(resource* command, bool enabled, bool checked) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_command(command);
        state.enabled = enabled;
        state.checked = state.checkable && checked;
        native_gui::update_command(state);
    });
}

extern "C" int txrt_native_gui_command_enabled(resource* command, bool* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_command(command).enabled;
    });
}

extern "C" int txrt_native_gui_command_checked(resource* command, bool* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_command(command).checked;
    });
}

extern "C" int txrt_native_gui_set_text_command(resource* command, const void* text) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_command(command);
        const auto& value = detail::text_value(text);
        if (value.size() > 65536)
        {
            native_gui::fail("resource_limit", "命令文字过长");
        }
        tx::ui::decode_utf8(value);
        state.text = value;
        native_gui::update_command(state);
    });
}

extern "C" int txrt_native_gui_set_shortcut(resource* command, const void* accelerator) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_shortcut(native_gui::require_command(command), detail::text_value(accelerator));
    });
}

extern "C" int txrt_native_gui_invoke_command(resource* command) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::invoke_command(native_gui::require_command(command));
    });
}

extern "C" int txrt_native_gui_create_menu(resource* window, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create_menu(native_gui::require_window(window)));
    });
}

extern "C" int txrt_native_gui_id_menu(resource* menu, std::int64_t* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_menu(menu).id;
    });
}

extern "C" int txrt_native_gui_close_menu(resource* menu) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_menu(menu, true);
        state.closed = true;
        if (const auto owner = state.owner.lock(); owner && !owner->closed)
        {
            native_gui::close_menu(*owner);
            if (owner->native_gui_root)
            {
                native_gui::dirty(*owner->native_gui_root);
            }
        }
    });
}

extern "C" int txrt_native_gui_append_menu_command(resource* menu, resource* command) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::append_menu(native_gui::require_menu(menu), "", native_gui::require_command(command).shared_from_this(), {});
    });
}

extern "C" int txrt_native_gui_append_submenu(resource* menu, const void* title, resource* submenu) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::append_menu(native_gui::require_menu(menu), detail::text_value(title), {}, native_gui::require_menu(submenu).shared_from_this());
    });
}

extern "C" int txrt_native_gui_append_separator(resource* menu) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::append_menu(native_gui::require_menu(menu), "", {}, {});
    });
}

extern "C" int txrt_native_gui_set_menu_bar(resource* window, resource* menu) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& owner = native_gui::require_window(window);
        auto& state = native_gui::require_menu(menu);
        if (state.owner.lock().get() != &owner)
        {
            native_gui::fail("wrong_owner", "菜单必须属于该窗口");
        }
        owner.menu_bar = state.shared_from_this();
        native_gui::dirty(*native_gui::root(owner));
    });
}

extern "C" int txrt_native_gui_popup_menu(resource* menu, double x, double y) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::show_menu(native_gui::require_menu(menu), x, y);
    });
}

extern "C" int txrt_native_gui_dismiss_menu(resource* window) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::close_menu(native_gui::require_window(window));
    });
}
