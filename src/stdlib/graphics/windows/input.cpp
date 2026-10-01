#include "stdlib/graphics/windows/assets.hpp"

#include <windowsx.h>
#include <imm.h>
#include <algorithm>

namespace tx_generated::graphics
{

std::string key_name(UINT key)
{
    if (key >= 'A' && key <= 'Z')
    {
        return std::string(1, static_cast<char>('a' + key - 'A'));
    }
    if (key >= '0' && key <= '9')
    {
        return std::string(1, static_cast<char>(key));
    }
    if (key >= VK_F1 && key <= VK_F24)
    {
        return "f" + std::to_string(key - VK_F1 + 1);
    }
    switch (key)
    {
    case VK_LEFT: return "left";
    case VK_RIGHT: return "right";
    case VK_UP: return "up";
    case VK_DOWN: return "down";
    case VK_RETURN: return "enter";
    case VK_ESCAPE: return "escape";
    case VK_TAB: return "tab";
    case VK_BACK: return "backspace";
    case VK_DELETE: return "delete";
    case VK_HOME: return "home";
    case VK_END: return "end";
    case VK_PRIOR: return "page_up";
    case VK_NEXT: return "page_down";
    case VK_SPACE: return "space";
    case VK_SHIFT: return "shift";
    case VK_CONTROL: return "control";
    case VK_MENU: return "alt";
    case VK_LWIN:
    case VK_RWIN: return "meta";
    default: return "unknown";
    }
}

UINT key_code(const std::string& name)
{
    for (UINT code = 1; code < 256; ++code)
    {
        if (name != "unknown" && key_name(code) == name)
        {
            return code;
        }
    }
    fail("invalid_argument", "不支持的键名");
}

void reset_input(window& state) noexcept
{
    state.keys.fill(false);
    state.pending_surrogate = 0;
    state.composing = false;
    if (GetCapture() == state.hwnd)
    {
        ReleaseCapture();
    }
}

namespace
{

bool pressed(int key)
{
    return (GetKeyState(key) & 0x8000) != 0;
}

void pointer_message(window& state, UINT message, WPARAM wparam, LPARAM lparam)
{
    event item;
    item.kind = message == WM_MOUSEMOVE ? "pointer_moved" :
        message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL ? "wheel" :
        message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN ||
        message == WM_XBUTTONDOWN ? "pointer_down" : "pointer_up";
    POINT location{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    if (item.kind == "wheel")
    {
        ScreenToClient(state.hwnd, &location);
    }
    event::pointer_data data;
    data.x = location.x * 96.0 / state.dpi;
    data.y = location.y * 96.0 / state.dpi;
    data.shift = pressed(VK_SHIFT);
    data.ctrl = pressed(VK_CONTROL);
    data.alt = pressed(VK_MENU);
    data.meta = pressed(VK_LWIN) || pressed(VK_RWIN);
    if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
    {
        (message == WM_MOUSEWHEEL ? data.wheel_y : data.wheel_x) = GET_WHEEL_DELTA_WPARAM(wparam) / 120.0;
    }
    else if (message != WM_MOUSEMOVE)
    {
        data.button = message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ? "left" :
            message == WM_RBUTTONDOWN || message == WM_RBUTTONUP ? "right" :
            message == WM_MBUTTONDOWN || message == WM_MBUTTONUP ? "middle" :
            GET_XBUTTON_WPARAM(wparam) == XBUTTON1 ? "x1" : "x2";
    }
    item.pointer = std::move(data);
    enqueue_event(state, std::move(item));
}

void character(window& state, wchar_t value)
{
    if (!state.text_input || value < 32 || value == 127)
    {
        return;
    }
    if (value >= 0xd800 && value <= 0xdbff)
    {
        state.pending_surrogate = value;
        return;
    }
    std::wstring text;
    if (value >= 0xdc00 && value <= 0xdfff)
    {
        if (!state.pending_surrogate)
        {
            return;
        }
        text.push_back(state.pending_surrogate);
    }
    state.pending_surrogate = 0;
    text.push_back(value);
    event item;
    item.kind = "text_input";
    item.text = event::text_data{utf8(text), 0, 0};
    enqueue_event(state, std::move(item));
}

} // namespace

bool ime_message(window& state, UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result);

bool input_message(window& state, UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result)
{
    result = 0;
    if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST &&
        (message == WM_MOUSEMOVE || message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL ||
        message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ||
        message == WM_RBUTTONDOWN || message == WM_RBUTTONUP ||
        message == WM_MBUTTONDOWN || message == WM_MBUTTONUP ||
        message == WM_XBUTTONDOWN || message == WM_XBUTTONUP))
    {
        pointer_message(state, message, wparam, lparam);
        result = message == WM_XBUTTONDOWN || message == WM_XBUTTONUP;
        return true;
    }
    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP)
    {
        const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
        if (wparam < state.keys.size())
        {
            state.keys[wparam] = down;
        }
        event item;
        item.kind = down ? "key_down" : "key_up";
        item.key = event::key_data{key_name(static_cast<UINT>(wparam)), (lparam >> 16) & 0x1ff,
            pressed(VK_SHIFT), pressed(VK_CONTROL), pressed(VK_MENU),
            pressed(VK_LWIN) || pressed(VK_RWIN), down && ((lparam & (1ll << 30)) != 0)};
        enqueue_event(state, std::move(item));
        return false;
    }
    if (message == WM_CHAR)
    {
        character(state, static_cast<wchar_t>(wparam));
        return state.text_input;
    }
    if (message == WM_SETFOCUS || message == WM_KILLFOCUS)
    {
        if (message == WM_KILLFOCUS)
        {
            reset_input(state);
        }
        enqueue(state, message == WM_SETFOCUS ? "focus_gained" : "focus_lost");
    }
    if (message == WM_CAPTURECHANGED || message == WM_CANCELMODE)
    {
        state.keys.fill(false);
    }
    if (message == WM_TIMER && state.timers.contains(wparam))
    {
        if (!state.minimized)
        {
            event item;
            item.kind = "timer";
            item.timer_id = static_cast<std::int64_t>(wparam);
            enqueue_event(state, std::move(item));
        }
        return true;
    }
    return ime_message(state, message, wparam, lparam, result);
}

} // namespace tx_generated::graphics
