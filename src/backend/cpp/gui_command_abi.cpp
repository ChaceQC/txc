#include "backend/cpp/graphics_result.hpp"
#include "stdlib/gui/windows/commands.hpp"

using namespace tx_generated;
using namespace tx_generated::gui;

extern "C" int txrt_gui_create_command(graphics::resource* value, const void* text, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(create_command(graphics::require_window(value), detail::text_value(text)));
    });
}

extern "C" int txrt_gui_bind_button(graphics::resource* button, graphics::resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        bind_button(require_node(button), require_command(value));
    });
}

extern "C" int txrt_gui_set_shortcut(graphics::resource* value, const void* key, bool ctrl, bool alt, bool shift, bool win) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_shortcut(require_command(value), {graphics::key_code(detail::text_value(key)), ctrl, alt, shift, win});
    });
}

extern "C" int txrt_gui_clear_shortcut(graphics::resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& command = require_command(value);
        command.shortcut.reset();
        ++command.revision;
    });
}

extern "C" int txrt_gui_id_command(graphics::resource* value, std::int64_t* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = require_command(value).id;
    });
}

extern "C" int txrt_gui_revision_command(graphics::resource* value, std::int64_t* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = require_command(value).revision;
    });
}

extern "C" int txrt_gui_enabled_command(graphics::resource* value, bool* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = require_command(value).enabled;
    });
}

extern "C" int txrt_gui_checked_command(graphics::resource* value, bool* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = require_command(value).checked;
    });
}

extern "C" int txrt_gui_text_command(graphics::resource* value, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = detail::make_handle<std::string>(require_command(value).text);
    });
}

extern "C" int txrt_gui_set_text_command(graphics::resource* value, const void* next) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_command(value);
        set_command(state, detail::text_value(next), state.enabled, state.checked);
    });
}

extern "C" int txrt_gui_set_enabled_command(graphics::resource* value, bool next) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_command(value);
        set_command(state, state.text, next, state.checked);
    });
}

extern "C" int txrt_gui_set_checked_command(graphics::resource* value, bool next) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_command(value);
        set_command(state, state.text, state.enabled, next);
    });
}

extern "C" int txrt_gui_close_command(graphics::resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = *static_cast<command*>(value);
        graphics::require_thread(state.thread);
        graphics::close_owned(state);
    });
}

extern "C" int txrt_gui_close_menu(graphics::resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = *static_cast<menu*>(value);
        graphics::require_thread(state.thread);
        graphics::close_owned(state);
    });
}

extern "C" int txrt_gui_create_menu(graphics::resource* value, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(create_menu(graphics::require_window(value)));
    });
}

extern "C" int txrt_gui_add_menu(graphics::resource* value, const void* text, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        auto& parent = require_menu(value);
        return graphics::make_handle(create_menu(*parent.hosted_window.lock(), &parent, detail::text_value(text)));
    });
}

extern "C" int txrt_gui_add_command(graphics::resource* target, graphics::resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        add_command(require_menu(target), require_command(value));
    });
}

extern "C" int txrt_gui_add_separator(graphics::resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_menu(value);
        if (!AppendMenuW(state.handle, MF_SEPARATOR, 0, nullptr))
        {
            platform_fail(state.owner.lock().get(), "添加菜单分隔线", GetLastError());
        }
    });
}

extern "C" int txrt_gui_set_menu(graphics::resource* value, graphics::resource* target, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        auto& window = graphics::require_window(value);
        auto& menu = require_menu(target);
        require_system_idle(window);
        if (menu.hosted_window.lock().get() != &window || !menu.parent.expired())
        {
            fail("wrong_owner", "窗口只能使用属于自己的根菜单");
        }
        if (!SetMenu(window.hwnd, menu.handle))
        {
            platform_fail(window.owner.lock().get(), "设置窗口菜单", GetLastError());
        }
        if (window.menu_bar)
        {
            window.menu_bar->attached = false;
        }
        window.menu_bar = menu.shared_from_this();
        menu.attached = true;
        DrawMenuBar(window.hwnd);
        graphics::update_size(window);
        return {};
    });
}

extern "C" int txrt_gui_create_toolbar(graphics::resource* parent, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        auto state = create(require_node(parent), tx::graphics_kind::container, {}, true);
        state->height = {length_mode::fixed, 32};
        return graphics::make_handle(state);
    });
}

extern "C" int txrt_gui_add_tool(graphics::resource* toolbar, graphics::resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        add_tool(require_node(toolbar), require_command(value));
    });
}

extern "C" int txrt_gui_show_modal(graphics::resource* child, graphics::resource* owner, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        show_modal(graphics::require_window(child), graphics::require_window(owner));
        return {};
    });
}
