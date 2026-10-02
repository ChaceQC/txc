#include "stdlib/gui/windows/state.hpp"
#include "stdlib/gui/windows/commands.hpp"
#include "stdlib/gui/windows/models.hpp"
#include "stdlib/gui/windows/complex.hpp"

namespace tx_generated::gui
{

void notify(node& state, const char* action, bool with_text, bool with_state)
{
    if (state.suppress || state.closed)
    {
        return;
    }
    graphics::event::control_data data;
    data.source_id = state.id;
    data.action = action;
    data.revision = state.revision;
    if (with_text && !state.password)
    {
        data.text = state.text;
    }
    if (with_state && !state.three_state)
    {
        data.state = state.check == BST_CHECKED;
    }
    graphics::enqueue_control(owner_window(state), std::move(data));
}

void process_command(HWND child, unsigned code)
{
    DWORD_PTR reference = 0;
    if (!child || !GetWindowSubclass(child, control_proc, 1, &reference))
    {
        return;
    }
    auto& state = *reinterpret_cast<node*>(reference);
    if (state.closed || state.suppress)
    {
        return;
    }
    using tx::graphics_kind;
    if (state.kind == graphics_kind::text_box && code == EN_CHANGE)
    {
        auto text = read_text(state);
        if (text.size() > 65536 ||
            scalar_offsets(wide_text(text)).size() - 1 > static_cast<std::size_t>(state.text_limit))
        {
            ++state.suppress;
            const auto old = wide_text(state.text);
            SetWindowTextW(state.hwnd, old.c_str());
            --state.suppress;
            return;
        }
        if (text != state.text)
        {
            state.text.swap(text);
            ++state.revision;
            dirty(state);
            notify(state, "text_changed", true);
        }
    }
    else if (state.kind == graphics_kind::button && code == BN_CLICKED)
    {
        if (state.bound_command)
        {
            if (!state.bound_command->closed)
            {
                activate_command(owner_window(state), state.bound_command->native_id, state.id);
            }
        }
        else
        {
            notify(state, "activated");
        }
    }
    else if (state.kind == graphics_kind::check_box && code == BN_CLICKED)
    {
        const auto checked = static_cast<int>(SendMessageW(child, BM_GETCHECK, 0, 0));
        if (checked != state.check)
        {
            state.check = checked;
            ++state.revision;
            notify(state, "check_changed", false, true);
        }
    }
}

LRESULT CALLBACK control_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
    UINT_PTR subclass_id, DWORD_PTR reference) noexcept
{
    auto& state = *reinterpret_cast<node*>(reference);
    try
    {
        notification_guard guard(state);
        LRESULT result = 0;
        if (theme_message(state, message, wparam, lparam, result))
        {
            return result;
        }
        if (complex_message(state, message, wparam, lparam, result))
        {
            return result;
        }
        switch (message)
        {
        case WM_NOTIFY:
            return data_notification(*reinterpret_cast<NMHDR*>(lparam));
        case WM_COMMAND:
        {
            DWORD_PTR reference = 0;
            if (GetWindowSubclass(reinterpret_cast<HWND>(lparam), control_proc, 1, &reference))
            {
                const auto& child = *reinterpret_cast<node*>(reference);
                if (child.toolbar && activate_command(owner_window(state), LOWORD(wparam), child.id))
                {
                    return 0;
                }
            }
            process_command(reinterpret_cast<HWND>(lparam), HIWORD(wparam));
            return 0;
        }
        case WM_IME_STARTCOMPOSITION:
            state.composing = true;
            break;
        case WM_IME_ENDCOMPOSITION:
            state.composing = false;
            break;
        case WM_SETFOCUS:
            notify(state, "focus_gained");
            break;
        case WM_KILLFOCUS:
            state.composing = false;
            notify(state, "focus_lost");
            break;
        case WM_ERASEBKGND:
            if (state.kind == tx::graphics_kind::container && !state.toolbar)
            {
                RECT rect{};
                GetClientRect(hwnd, &rect);
                FillRect(reinterpret_cast<HDC>(wparam), &rect, GetSysColorBrush(COLOR_BTNFACE));
                return 1;
            }
            break;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        {
            const bool edit = message == WM_CTLCOLOREDIT;
            const auto dc = reinterpret_cast<HDC>(wparam);
            SetTextColor(dc, GetSysColor(edit ? COLOR_WINDOWTEXT : COLOR_BTNTEXT));
            SetBkColor(dc, GetSysColor(edit ? COLOR_WINDOW : COLOR_BTNFACE));
            return reinterpret_cast<LRESULT>(GetSysColorBrush(edit ? COLOR_WINDOW : COLOR_BTNFACE));
        }
        case WM_PAINT:
            if (state.kind == tx::graphics_kind::container && !state.toolbar)
            {
                PAINTSTRUCT paint{};
                const auto dc = BeginPaint(hwnd, &paint);
                FillRect(dc, &paint.rcPaint, GetSysColorBrush(COLOR_BTNFACE));
                if (state.split)
                {
                    const auto scale = owner_window(state).dpi / 96.0;
                    const auto& bar = state.divider;
                    RECT rect{static_cast<LONG>(bar.x * scale), static_cast<LONG>(bar.y * scale),
                        static_cast<LONG>((bar.x + bar.width) * scale), static_cast<LONG>((bar.y + bar.height) * scale)};
                    FillRect(dc, &rect, GetSysColorBrush(GetFocus() == hwnd ? COLOR_HIGHLIGHT : COLOR_BTNSHADOW));
                }
                EndPaint(hwnd, &paint);
                return 0;
            }
            break;
        case WM_NCDESTROY:
            close_accessibility(state);
            reset_interaction(state);
            state.hwnd = nullptr;
            RemoveWindowSubclass(hwnd, control_proc, subclass_id);
            break;
        }
    }
    catch (const runtime_failure& error)
    {
        if (const auto window = state.window.lock())
        {
            if (const auto owner = window->owner.lock())
            {
                try
                {
                    owner->pending_error = error.error();
                }
                catch (...)
                {
                    owner->queue_failed = true;
                }
            }
        }
    }
    catch (...)
    {
        if (const auto window = state.window.lock())
        {
            if (const auto owner = window->owner.lock())
            {
                owner->queue_failed = true;
            }
        }
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}

} // namespace tx_generated::gui
