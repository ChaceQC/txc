#include "stdlib/gui/windows/complex.hpp"

#include <windowsx.h>
#include <algorithm>
#include <cmath>

namespace tx_generated::gui
{
namespace
{
bool scroll_message(node& state, UINT message, WPARAM wparam)
{
    const bool horizontal = message == WM_HSCROLL || message == WM_MOUSEHWHEEL ||
        (message == WM_MOUSEWHEEL && !state.scroll_vertical);
    if (!(horizontal ? state.scroll_horizontal : state.scroll_vertical))
    {
        return false;
    }
    auto& position = horizontal ? state.scroll_x : state.scroll_y;
    const auto page = horizontal ? state.arranged.width : state.arranged.height;
    const auto limit = horizontal ? state.scroll_extent.width : state.scroll_extent.height;
    auto next = position;
    if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
    {
        next += GET_WHEEL_DELTA_WPARAM(wparam) / 120.0 * 48 * (message == WM_MOUSEHWHEEL ? 1 : -1);
    }
    else
    {
        switch (LOWORD(wparam))
        {
        case SB_LINEUP: next -= 16; break;
        case SB_LINEDOWN: next += 16; break;
        case SB_PAGEUP: next -= page; break;
        case SB_PAGEDOWN: next += page; break;
        case SB_TOP: next = 0; break;
        case SB_BOTTOM: next = limit; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
        {
            SCROLLINFO info{};
            info.cbSize = sizeof(info);
            info.fMask = SIF_TRACKPOS;
            GetScrollInfo(state.hwnd, horizontal ? SB_HORZ : SB_VERT, &info);
            next = info.nTrackPos;
            break;
        }
        }
    }
    position = std::clamp(next, 0.0, limit);
    ++state.revision;
    dirty(state);
    return true;
}

void drag_split(node& state, LPARAM lparam)
{
    const auto scale = owner_window(state).dpi / 96.0;
    const auto coordinate = (state.split_vertical ? GET_Y_LPARAM(lparam) : GET_X_LPARAM(lparam)) / scale;
    const auto axis = state.split_vertical ? state.arranged.height : state.arranged.width;
    set_split_position(state, std::clamp((coordinate - 3) / std::max(1.0, axis - 6), 0.0, 1.0), true);
}
}

void reset_interaction(node& state) noexcept
{
    state.composing = false;
    state.dragging = false;
    if (GetCapture() == state.hwnd)
    {
        ReleaseCapture();
    }
    if (state.canvas_window)
    {
        graphics::reset_input(*state.canvas_window);
    }
    for (const auto& child : state.children)
    {
        reset_interaction(*child);
    }
}

bool complex_key(node& state, WPARAM key)
{
    if (state.kind == tx::graphics_kind::gui_canvas && (key == VK_RETURN || key == VK_SPACE))
    {
        notify(state, "activated");
        return true;
    }
    if (state.kind == tx::graphics_kind::tabs && !state.children.empty() &&
        (key == VK_LEFT || key == VK_RIGHT))
    {
        auto index = static_cast<int>(SendMessageW(state.hwnd, TCM_GETCURSEL, 0, 0));
        const auto count = static_cast<int>(state.children.size());
        index = (index + (key == VK_LEFT ? -1 : 1) + count) % count;
        select_tab(state, *state.children[index], true);
        return true;
    }
    if (state.split && (key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN ||
        key == VK_HOME || key == VK_END))
    {
        auto value = state.split_position + ((key == VK_LEFT || key == VK_UP) ? -0.02 : 0.02);
        value = key == VK_HOME ? 0 : key == VK_END ? 1 : std::clamp(value, 0.0, 1.0);
        set_split_position(state, value, true);
        value_event(state, "value_committed", value);
        return true;
    }
    if ((state.scroll_horizontal || state.scroll_vertical) &&
        (key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN ||
            key == VK_PRIOR || key == VK_NEXT || key == VK_HOME || key == VK_END))
    {
        const auto code = key == VK_HOME ? SB_TOP : key == VK_END ? SB_BOTTOM :
            key == VK_PRIOR ? SB_PAGEUP : key == VK_NEXT ? SB_PAGEDOWN :
            key == VK_LEFT || key == VK_UP ? SB_LINEUP : SB_LINEDOWN;
        return scroll_message(state, key == VK_LEFT || key == VK_RIGHT || !state.scroll_vertical ?
            WM_HSCROLL : WM_VSCROLL, code);
    }
    return false;
}

namespace
{
bool native_notification(UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_NOTIFY)
    {
        const auto& header = *reinterpret_cast<NMHDR*>(lparam);
        DWORD_PTR reference = 0;
        if (header.code == TCN_SELCHANGE && GetWindowSubclass(header.hwndFrom, control_proc, 1, &reference))
        {
            auto& tabs = *reinterpret_cast<node*>(reference);
            const auto index = SendMessageW(tabs.hwnd, TCM_GETCURSEL, 0, 0);
            if (index >= 0 && static_cast<std::size_t>(index) < tabs.children.size())
            {
                select_tab(tabs, *tabs.children[index], true);
            }
            return true;
        }
    }
    if ((message == WM_HSCROLL || message == WM_VSCROLL) && lparam)
    {
        DWORD_PTR reference = 0;
        if (GetWindowSubclass(reinterpret_cast<HWND>(lparam), control_proc, 1, &reference))
        {
            auto& slider = *reinterpret_cast<node*>(reference);
            if (slider.kind == tx::graphics_kind::slider)
            {
                set_slider_value(slider, SendMessageW(slider.hwnd, TBM_GETPOS, 0, 0), true);
                if (LOWORD(wparam) == TB_ENDTRACK)
                {
                    value_event(slider, "value_committed", static_cast<double>(slider.range_value));
                }
                return true;
            }
        }
    }
    return false;
}

bool accessibility_message(node& state, UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result)
{
    if (message == WM_GETOBJECT && static_cast<LONG>(lparam) == -25 &&
        (state.split || state.kind == tx::graphics_kind::gui_canvas))
    {
        result = accessibility_object(state, wparam, lparam);
        return true;
    }
    if (message != accessibility_action && message != accessibility_focus)
    {
        return false;
    }
    if (!IsWindowEnabled(owner_window(state).hwnd))
    {
        return true;
    }
    for (auto current = state.shared_from_this(); current; current = current->parent.lock())
    {
        if (current->closed || !current->enabled || !current->visible || !current->page_active)
        {
            return true;
        }
    }
    if (message == accessibility_focus)
    {
        SetFocus(state.hwnd);
    }
    else if (state.split)
    {
        set_split_position(state, static_cast<double>(wparam) / 1000000, true);
        value_event(state, "value_committed", state.split_position);
    }
    else
    {
        notify(state, "activated");
    }
    return true;
}

bool split_message(node& state, UINT message, LPARAM lparam)
{
    if (!state.split)
    {
        return false;
    }
    if (message == WM_SETCURSOR)
    {
        SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(state.split_vertical ? 32645 : 32644)));
        return true;
    }
    if (message != WM_LBUTTONDOWN && message != WM_MOUSEMOVE && message != WM_LBUTTONUP)
    {
        return false;
    }
    if (message == WM_LBUTTONDOWN)
    {
        const auto scale = owner_window(state).dpi / 96.0;
        const auto x = GET_X_LPARAM(lparam) / scale;
        const auto y = GET_Y_LPARAM(lparam) / scale;
        const auto& bar = state.divider;
        if (x < bar.x || x >= bar.x + bar.width || y < bar.y || y >= bar.y + bar.height)
        {
            return false;
        }
        state.dragging = true;
        SetFocus(state.hwnd);
        SetCapture(state.hwnd);
    }
    if (state.dragging)
    {
        drag_split(state, lparam);
        if (message == WM_LBUTTONUP)
        {
            state.dragging = false;
            ReleaseCapture();
            value_event(state, "value_committed", state.split_position);
        }
    }
    return true;
}

bool canvas_message(node& state, UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result)
{
    if (!state.canvas_window)
    {
        return false;
    }
    if (message == WM_LBUTTONDOWN && IsWindowEnabled(state.hwnd))
    {
        SetFocus(state.hwnd);
    }
    if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
    {
        for (auto parent = state.parent.lock(); parent; parent = parent->parent.lock())
        {
            if (scroll_message(*parent, message, wparam))
            {
                return true;
            }
        }
    }
    if (message == WM_PAINT)
    {
        PAINTSTRUCT paint{};
        BeginPaint(state.hwnd, &paint);
        EndPaint(state.hwnd, &paint);
        canvas_paint(state);
        return true;
    }
    if (message == WM_SIZE)
    {
        sync_canvas(state);
    }
    return graphics::input_message(*state.canvas_window, message, wparam, lparam, result);
}
}

bool complex_message(node& state, UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result)
{
    result = 0;
    if (accessibility_message(state, message, wparam, lparam, result) ||
        native_notification(message, wparam, lparam))
    {
        return true;
    }
    if (((message == WM_HSCROLL || message == WM_VSCROLL) && !lparam) ||
        message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
    {
        if (scroll_message(state, message, wparam))
        {
            return true;
        }
    }
    if (message == WM_KILLFOCUS || message == WM_CANCELMODE || message == WM_CAPTURECHANGED)
    {
        reset_interaction(state);
    }
    if (message == WM_SETFOCUS || message == WM_KILLFOCUS || message == WM_WINDOWPOSCHANGED ||
        message == WM_ENABLE || message == WM_SHOWWINDOW)
    {
        update_accessibility(state);
    }
    return split_message(state, message, lparam) ||
        canvas_message(state, message, wparam, lparam, result) ||
        (message == WM_KEYDOWN && complex_key(state, wparam));
}
} // namespace tx_generated::gui
