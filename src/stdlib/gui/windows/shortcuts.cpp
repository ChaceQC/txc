#include "stdlib/gui/windows/commands.hpp"

namespace tx_generated::gui
{

void set_shortcut(command& value, chord shortcut)
{
    const auto window = value.hosted_window.lock();
    for (const auto& resource : value.owner.lock()->resources)
    {
        if (resource.get() == &value || resource->closed || resource->kind != tx::graphics_kind::command ||
            resource->hosted_window.lock() != window)
        {
            continue;
        }
        const auto& other = *static_cast<command*>(resource.get());
        if (other.shortcut && *other.shortcut == shortcut)
        {
            fail("duplicate_id", "同一窗口作用域的快捷键重复");
        }
    }
    value.shortcut = shortcut;
    ++value.revision;
}

bool translate_shortcut(graphics::app& app, const MSG& message)
{
    if (message.message != WM_KEYDOWN && message.message != WM_SYSKEYDOWN)
    {
        return false;
    }
    chord pressed{static_cast<UINT>(message.wParam), (GetKeyState(VK_CONTROL) & 0x8000) != 0,
        (GetKeyState(VK_MENU) & 0x8000) != 0, (GetKeyState(VK_SHIFT) & 0x8000) != 0,
        ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) != 0};
    DWORD_PTR reference = 0;
    if (GetWindowSubclass(message.hwnd, control_proc, 1, &reference))
    {
        const auto& node = *reinterpret_cast<gui::node*>(reference);
        if (node.composing)
        {
            return false;
        }
        if (node.kind == tx::graphics_kind::text_box &&
            (!pressed.ctrl && !pressed.alt && !pressed.win))
        {
            return false;
        }
        if (node.kind == tx::graphics_kind::text_box && pressed.ctrl && !pressed.alt && !pressed.win)
        {
            switch (pressed.key)
            {
            case 'A': case 'C': case 'V': case 'X': case 'Z': case 'Y':
            case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
            case VK_HOME: case VK_END: case VK_BACK: case VK_DELETE: case VK_INSERT:
                return false;
            }
        }
    }
    for (const auto& resource : app.resources)
    {
        if (resource->kind != tx::graphics_kind::command || resource->closed)
        {
            continue;
        }
        auto& value = *static_cast<command*>(resource.get());
        const auto window = value.hosted_window.lock();
        if (!window || window->closed || window->composing || !IsWindowEnabled(window->hwnd) ||
            (message.hwnd != window->hwnd && !IsChild(window->hwnd, message.hwnd)))
        {
            continue;
        }
        if (value.shortcut && *value.shortcut == pressed && value.enabled)
        {
            return activate_command(*window, value.native_id);
        }
    }
    return false;
}

} // namespace tx_generated::gui
