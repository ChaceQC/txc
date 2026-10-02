#include "stdlib/native_gui/combo.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
void set_access_key(node& state, const std::string& key)
{
    require_idle(state);
    if (!key.empty() && (key.size() != 1 || !((key[0] >= 'a' && key[0] <= 'z') ||
        (key[0] >= 'A' && key[0] <= 'Z') || (key[0] >= '0' && key[0] <= '9'))))
    {
        fail("invalid_argument", "访问键必须为空或单个 ASCII 字母/数字");
    }
    state.access_key = key;
    if (!key.empty() && key[0] >= 'A' && key[0] <= 'Z')
    {
        state.access_key[0] += 'a' - 'A';
    }
}

void set_action_button(node& state, bool cancel, bool enabled)
{
    require_idle(state);
    auto& root = root_node(state);
    auto& target = cancel ? root.cancel_button : root.default_button;
    if (enabled)
    {
        target = state.shared_from_this();
    }
    else if (target.lock().get() == &state)
    {
        target.reset();
    }
    dirty(root);
}

namespace
{
void collect_access_keys(node& state, const std::string& key, std::vector<std::shared_ptr<node>>& matches)
{
    if (!available(state))
    {
        return;
    }
    if (interactive(state) && state.access_key == key)
    {
        matches.push_back(state.shared_from_this());
    }
    for (const auto& child : state.children)
    {
        collect_access_keys(*child, key, matches);
    }
}
}

bool access_key(node& root, const tx::ui::window_event& input)
{
    if (!input.alt || input.ctrl || input.meta || input.key.size() != 1)
    {
        return false;
    }
    std::string key = input.key;
    if (key[0] >= 'A' && key[0] <= 'Z')
    {
        key[0] += 'a' - 'A';
    }
    std::vector<std::shared_ptr<node>> matches;
    collect_access_keys(root, key, matches);
    if (matches.empty())
    {
        return false;
    }
    if (input.kind != tx::ui::event_kind::key_down || input.repeat)
    {
        return true;
    }
    const auto found = std::find(matches.begin(), matches.end(), root.focused.lock());
    const auto target = found == matches.end() || found + 1 == matches.end() ?
        matches.front() : *(found + 1);
    reset_interaction(root);
    focus_node(root, target);
    if (matches.size() == 1)
    {
        if (target->combo)
        {
            open_combo(*target);
        }
        else
        {
            activate(*target);
        }
    }
    return true;
}

bool window_action(node& root, const tx::ui::window_event& input)
{
    if (input.ctrl || input.alt || input.meta || input.shift ||
        (input.key != "enter" && input.key != "escape"))
    {
        return false;
    }
    const auto target = (input.key == "escape" ? root.cancel_button : root.default_button).lock();
    if (!target || !available(*target))
    {
        return false;
    }
    if (input.kind == tx::ui::event_kind::key_down && !input.repeat)
    {
        reset_interaction(root);
        activate(*target);
    }
    return true;
}
}
