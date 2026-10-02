#include "stdlib/native_gui/state.hpp"

#include <algorithm>
#include <cmath>

namespace tx_generated::native_gui
{
void activate(node& state)
{
    if (!available(state))
    {
        return;
    }
    event notification{"activated"};
    notification.source_id = state.id;
    if (state.kind == tx::graphics_kind::native_check_box)
    {
        state.checked = !state.checked;
        notification.kind = "check_changed";
        notification.state = state.checked;
    }
    else if (state.kind == tx::graphics_kind::native_radio_button)
    {
        if (state.checked)
        {
            return;
        }
        if (const auto parent = state.parent.lock())
        {
            for (const auto& child : parent->children)
            {
                if (child->kind == state.kind && child->radio_group == state.radio_group && child->checked)
                {
                    child->checked = false;
                    ++child->revision;
                }
            }
        }
        state.checked = true;
        notification.kind = "check_changed";
        notification.state = true;
    }
    else if (state.kind != tx::graphics_kind::native_button)
    {
        return;
    }
    notification.revision = ++state.revision;
    enqueue(owner_window(state), std::move(notification));
    owner_window(state).repaint = true;
}

namespace
{
void slider_value(node& state, double value)
{
    value = value <= state.minimum ? state.minimum : value >= state.maximum ? state.maximum :
        std::clamp(state.minimum + std::round((value - state.minimum) / state.step) * state.step,
            state.minimum, state.maximum);
    if (value != state.value)
    {
        state.value = value;
        event notification{"value_changed"};
        notification.source_id = state.id;
        notification.number = value;
        notification.revision = ++state.revision;
        enqueue(owner_window(state), std::move(notification));
        owner_window(state).repaint = true;
    }
}
}

void slider_pointer(node& state, double x)
{
    const double ratio = state.bounds.width <= 24 ? 0 :
        std::clamp((x - state.bounds.x - 12) / (state.bounds.width - 24), 0.0, 1.0);
    slider_value(state, state.minimum + (state.maximum - state.minimum) * ratio);
}

void slider_commit(node& state)
{
    event notification{"value_committed"};
    notification.source_id = state.id;
    notification.number = state.value;
    notification.revision = state.revision;
    enqueue(owner_window(state), std::move(notification));
}

bool slider_key(node& state, const tx::ui::window_event& event)
{
    double value = state.value;
    if (event.key == "left" || event.key == "down")
    {
        value -= state.step;
    }
    else if (event.key == "right" || event.key == "up")
    {
        value += state.step;
    }
    else if (event.key == "home" || event.key == "end")
    {
        value = event.key == "home" ? state.minimum : state.maximum;
    }
    else if (event.key == "page_up" || event.key == "page_down")
    {
        value += state.step * (event.key == "page_up" ? 10 : -10);
    }
    else
    {
        return false;
    }
    auto& root = root_node(state);
    if (event.kind == tx::ui::event_kind::key_down)
    {
        root.keyboard_pressed = true;
        root.pressed = state.shared_from_this();
        slider_value(state, value);
    }
    else if (root.pressed.lock().get() == &state)
    {
        slider_commit(state);
        root.pressed.reset();
        root.keyboard_pressed = false;
    }
    return true;
}

bool radio_key(node& state, const tx::ui::window_event& event)
{
    if (event.kind != tx::ui::event_kind::key_down ||
        (event.key != "left" && event.key != "right" && event.key != "up" && event.key != "down"))
    {
        return false;
    }
    const auto parent = state.parent.lock();
    if (!parent)
    {
        return true;
    }
    std::vector<std::shared_ptr<node>> group;
    std::size_t selected = 0;
    for (const auto& child : parent->children)
    {
        if (child->kind == state.kind && child->radio_group == state.radio_group && available(*child))
        {
            if (child.get() == &state)
            {
                selected = group.size();
            }
            group.push_back(child);
        }
    }
    if (!group.empty())
    {
        const bool reverse = event.key == "left" || event.key == "up";
        const auto target = group[(selected + (reverse ? group.size() - 1 : 1)) % group.size()];
        focus_node(root_node(state), target);
        activate(*target);
    }
    return true;
}
}
