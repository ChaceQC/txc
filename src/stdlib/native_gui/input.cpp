#include "stdlib/native_gui/state.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
using tx::ui::event_kind;
using tx::ui::window_event;

void reset_interaction(node& root) noexcept
{
    const bool captured = !root.pressed.expired() && !root.keyboard_pressed;
    root.pressed.reset();
    root.hovered.reset();
    root.keyboard_pressed = false;
    if (const auto focus = root.focused.lock(); focus && !available(*focus))
    {
        root.focused.reset();
    }
    if (const auto window = root.window.lock(); window && captured)
    {
        try
        {
            window->host->capture_pointer(false);
        }
        catch (...)
        {
        }
    }
}

namespace
{
std::shared_ptr<node> hit_test(node& state, double x, double y)
{
    if (!available(state) || !contains(state.clip, x, y))
    {
        return {};
    }
    for (auto child = state.children.rbegin(); child != state.children.rend(); ++child)
    {
        if (auto hit = hit_test(**child, x, y))
        {
            return hit;
        }
    }
    return interactive(state) ? state.shared_from_this() : nullptr;
}

void focusable(node& state, std::vector<std::shared_ptr<node>>& result)
{
    if (!available(state) || state.clip.width <= 0 || state.clip.height <= 0)
    {
        return;
    }
    if (interactive(state))
    {
        result.push_back(state.shared_from_this());
    }
    for (const auto& child : state.children)
    {
        focusable(*child, result);
    }
}

void advance_focus(node& root, bool reverse)
{
    std::vector<std::shared_ptr<node>> candidates;
    focusable(root, candidates);
    if (candidates.empty())
    {
        root.focused.reset();
        return;
    }
    const auto found = std::find(candidates.begin(), candidates.end(), root.focused.lock());
    const auto count = candidates.size();
    const auto index = found == candidates.end() ? (reverse ? count - 1 : 0) :
        (static_cast<std::size_t>(found - candidates.begin()) + (reverse ? count - 1 : 1)) % count;
    focus_node(root, candidates[index]);
}

void pointer(node& root, const window_event& event)
{
    auto& window = owner_window(root);
    const auto hit = hit_test(root, event.x, event.y);
    bool changed = root.hovered.lock() != hit;
    root.hovered = hit;
    if (event.kind == event_kind::wheel && hit && hit->editor)
    {
        ensure_text(*hit, std::max(1.0, hit->bounds.width - 24));
        hit->text_scroll_y = std::clamp(hit->text_scroll_y - event.wheel * 48,
            0.0, std::max(0.0, hit->text_layout->height() - std::max(1.0, hit->bounds.height - 16)));
        window.repaint = true;
        return;
    }
    if (event.kind == event_kind::pointer_down && event.button == 1)
    {
        reset_interaction(root);
        root.hovered = hit;
        focus_node(root, hit);
        window.host->focus();
        if (hit)
        {
            window.host->capture_pointer(true);
        }
        root.pressed = hit;
        if (hit && hit->editor)
        {
            text_pointer(*hit, event.x, event.y, event.shift);
        }
        else if (hit && hit->kind == tx::graphics_kind::native_slider)
        {
            slider_pointer(*hit, event.x);
        }
        changed = true;
    }
    else if (event.kind == event_kind::pointer_up && event.button == 1 && !root.keyboard_pressed)
    {
        const auto pressed = root.pressed.lock();
        root.pressed.reset();
        if (pressed)
        {
            window.host->capture_pointer(false);
        }
        root.hovered = hit;
        if (pressed && pressed->kind == tx::graphics_kind::native_slider)
        {
            slider_pointer(*pressed, event.x);
            slider_commit(*pressed);
        }
        else if (pressed && pressed->editor)
        {
            text_pointer(*pressed, event.x, event.y, true);
        }
        else if (pressed && pressed == hit)
        {
            activate(*pressed);
        }
        changed = true;
    }
    else if (event.kind == event_kind::pointer_moved && !root.keyboard_pressed)
    {
        if (const auto pressed = root.pressed.lock())
        {
            if (pressed->editor)
            {
                text_pointer(*pressed, event.x, event.y, true);
            }
            else if (pressed->kind == tx::graphics_kind::native_slider)
            {
                slider_pointer(*pressed, event.x);
            }
        }
    }
    if (changed)
    {
        window.repaint = true;
    }
}

void key(node& root, const window_event& event)
{
    if (event.composing)
    {
        return;
    }
    const bool down = event.kind == event_kind::key_down;
    if (down && event.key == "tab" && !event.ctrl && !event.alt && !event.meta)
    {
        reset_interaction(root);
        advance_focus(root, event.shift);
    }
    else if (down && event.key == "escape")
    {
        reset_interaction(root);
    }
    else if (const auto focus = root.focused.lock(); focus && available(*focus))
    {
        if (focus->editor)
        {
            text_key(*focus, event);
            return;
        }
        if ((focus->kind == tx::graphics_kind::native_slider && slider_key(*focus, event)) ||
            (focus->kind == tx::graphics_kind::native_radio_button && radio_key(*focus, event)))
        {
            return;
        }
        if (event.ctrl || event.alt || event.meta)
        {
            reset_interaction(root);
        }
        else if (down && !event.repeat && event.key == "enter")
        {
            activate(*focus);
        }
        else if (event.key == "space")
        {
            if (down && !event.repeat)
            {
                reset_interaction(root);
                root.pressed = focus;
                root.keyboard_pressed = true;
            }
            else if (!down && root.keyboard_pressed)
            {
                const bool activate_focus = root.pressed.lock() == focus;
                reset_interaction(root);
                if (activate_focus)
                {
                    activate(*focus);
                }
            }
        }
    }
    owner_window(root).repaint = true;
}
}

void process_event(window& window, const window_event& event)
{
    const auto root = window.native_gui_root;
    if (!root)
    {
        return;
    }
    if (event.kind == event_kind::pointer_down || event.kind == event_kind::pointer_up ||
        event.kind == event_kind::pointer_moved || event.kind == event_kind::wheel)
    {
        layout(*root);
        pointer(*root, event);
    }
    else if (event.kind == event_kind::key_down || event.kind == event_kind::key_up)
    {
        layout(*root);
        key(*root, event);
    }
    else if (event.kind == event_kind::text_input || event.kind == event_kind::composition)
    {
        if (const auto focused = root->focused.lock(); root->window_focused && focused && focused->editor && available(*focused))
        {
            text_input(*focused, event);
        }
    }
    else if (event.kind == event_kind::focus_lost || event.kind == event_kind::focus_gained)
    {
        if (event.kind == event_kind::focus_lost)
        {
            if (const auto focused = root->focused.lock(); focused && focused->editor)
            {
                focused->composing = false;
                focused->composition.clear();
                focused->text_layout.reset();
                dirty(*focused);
            }
            reset_interaction(*root);
        }
        root->window_focused = event.kind == event_kind::focus_gained;
        window.repaint = true;
    }
    else if (event.kind == event_kind::pointer_left || event.kind == event_kind::capture_lost)
    {
        if (event.kind == event_kind::capture_lost)
        {
            reset_interaction(*root);
        }
        else
        {
            root->hovered.reset();
        }
        window.repaint = true;
    }
}
}
