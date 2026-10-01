#include "stdlib/gui/windows/state.hpp"
#include "stdlib/gui/windows/complex.hpp"

#include <algorithm>

namespace tx_generated::gui
{
namespace
{

bool interactive(const node& state)
{
    return state.kind == tx::graphics_kind::button ||
        state.kind == tx::graphics_kind::text_box || state.kind == tx::graphics_kind::check_box ||
        state.kind == tx::graphics_kind::list_view || state.kind == tx::graphics_kind::table_view ||
        state.kind == tx::graphics_kind::tree_view || state.kind == tx::graphics_kind::tabs ||
        state.kind == tx::graphics_kind::slider || state.kind == tx::graphics_kind::gui_canvas ||
        state.scroll_horizontal || state.scroll_vertical || state.split || state.toolbar;
}

void collect(node& state, std::vector<node*>& all, bool available = true)
{
    available = available && !state.closed && state.visible && state.page_active && state.enabled;
    if (available && interactive(state))
    {
        all.push_back(&state);
    }
    for (const auto& child : state.children)
    {
        collect(*child, all, available);
    }
}

bool excluded_by(const node& state, node* excluded)
{
    return excluded && (&state == excluded || IsChild(excluded->hwnd, state.hwnd));
}

node* focused_node(node& state, HWND hwnd)
{
    if (state.hwnd == hwnd)
    {
        return &state;
    }
    for (const auto& child : state.children)
    {
        if (auto* found = focused_node(*child, hwnd))
        {
            return found;
        }
    }
    return nullptr;
}

bool activate(const std::shared_ptr<node>& button)
{
    if (!button || button->closed || !IsWindowVisible(button->hwnd) ||
        !IsWindowEnabled(button->hwnd))
    {
        return false;
    }
    for (auto parent = button->parent.lock(); parent; parent = parent->parent.lock())
    {
        if (!parent->enabled || !parent->visible || !parent->page_active)
        {
            return false;
        }
    }
    SendMessageW(button->hwnd, BM_CLICK, 0, 0);
    return true;
}

} // namespace

void focus(node& state)
{
    if (!interactive(state) || !IsWindowVisible(state.hwnd) || !IsWindowEnabled(state.hwnd))
    {
        fail("invalid_argument", "焦点目标必须为可见且启用的交互控件");
    }
    for (auto parent = state.parent.lock(); parent; parent = parent->parent.lock())
    {
        if (!parent->enabled || !parent->visible || !parent->page_active)
        {
            fail("invalid_argument", "焦点目标的父容器已隐藏或禁用");
        }
    }
    flush_layout(owner_window(state));
    SetFocus(state.hwnd);
    if (GetFocus() != state.hwnd)
    {
        platform_fail(owner_window(state).owner.lock().get(), "切换 GUI 焦点", GetLastError());
    }
}

void advance_focus(node& root, node* excluded, bool reverse)
{
    std::vector<node*> all;
    collect(root, all);
    const auto current = std::find_if(all.begin(), all.end(), [](const node* item)
    {
        return item->hwnd == GetFocus();
    });
    const auto count = static_cast<std::int64_t>(all.size());
    auto index = current == all.end() ? (reverse ? 0 : -1) : current - all.begin();
    for (std::int64_t offset = 0; offset < count; ++offset)
    {
        index = (index + (reverse ? -1 : 1) + count) % count;
        if (!excluded_by(*all[index], excluded))
        {
            SetFocus(all[index]->hwnd);
            return;
        }
    }
    if (const auto window = root.window.lock())
    {
        SetFocus(window->hwnd);
    }
}

bool translate_message(graphics::app& app, const MSG& message)
{
    if (message.message != WM_KEYDOWN)
    {
        return false;
    }
    for (const auto& window : app.windows)
    {
        if (!window->gui_root || (message.hwnd != window->hwnd &&
            !IsChild(window->hwnd, message.hwnd)))
        {
            continue;
        }
        auto& root = *window->gui_root;
        auto* target = focused_node(root, message.hwnd);
        if (target && target->composing)
        {
            return false;
        }
        if (message.wParam == VK_TAB)
        {
            if (target && (GetKeyState(VK_CONTROL) & 0x8000))
            {
                auto tabs = target->shared_from_this();
                while (tabs && tabs->kind != tx::graphics_kind::tabs)
                {
                    tabs = tabs->parent.lock();
                }
                if (tabs)
                {
                    return complex_key(*tabs, (GetKeyState(VK_SHIFT) & 0x8000) ? VK_LEFT : VK_RIGHT);
                }
            }
            advance_focus(root, nullptr, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
            return true;
        }
        if (target && complex_key(*target, message.wParam))
        {
            return true;
        }
        if (message.wParam == VK_RETURN)
        {
            if (target && target->kind == tx::graphics_kind::text_box)
            {
                if (target->multiline)
                {
                    return false;
                }
                notify(*target, "text_committed", true);
                return true;
            }
            return activate(target && target->kind == tx::graphics_kind::button ?
                target->shared_from_this() : root.default_button.lock());
        }
        if (message.wParam == VK_ESCAPE)
        {
            if (!activate(root.cancel_button.lock()))
            {
                graphics::enqueue(*window, "close_requested");
            }
            return true;
        }
    }
    return false;
}

} // namespace tx_generated::gui
