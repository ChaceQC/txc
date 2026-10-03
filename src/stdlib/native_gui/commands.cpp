#include "stdlib/native_gui/commands.hpp"

#include <algorithm>
#include <set>

namespace tx_generated::native_gui
{
namespace
{
shortcut parse_shortcut(const std::string& value)
{
    shortcut result;
    std::set<std::string> seen;
    std::size_t start = 0;
    while (start < value.size())
    {
        const auto end = value.find('+', start);
        const auto part = value.substr(start, end == std::string::npos ? end : end - start);
        if (part.empty() || !seen.insert(part).second)
        {
            fail("invalid_argument", "快捷键包含空键名或重复修饰键");
        }
        if (part == "ctrl")
        {
            result.ctrl = true;
        }
        else if (part == "shift")
        {
            result.shift = true;
        }
        else if (part == "alt")
        {
            result.alt = true;
        }
        else if (part == "meta")
        {
            result.meta = true;
        }
        else if (result.key.empty())
        {
            result.key = part;
        }
        else
        {
            fail("invalid_argument", "快捷键只能包含一个普通键");
        }
        if (end == std::string::npos)
        {
            break;
        }
        start = end + 1;
        if (start == value.size())
        {
            fail("invalid_argument", "快捷键不能以加号结尾");
        }
    }
    const auto& key = result.key;
    const bool single = key.size() == 1 && ((key[0] >= 'a' && key[0] <= 'z') || (key[0] >= '0' && key[0] <= '9'));
    const std::set<std::string> names{"enter", "escape", "space", "tab", "delete", "backspace",
        "home", "end", "left", "right", "up", "down", "page_up", "page_down",
        "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12"};
    if (!value.empty() && !single && !names.contains(key))
    {
        fail("invalid_argument", "快捷键键名无效；请使用小写字母、数字或具名按键");
    }
    return result;
}
}

command& require_command(graphics::resource* value, bool allow_closed)
{
    auto& state = *static_cast<command*>(value);
    const auto owner = state.owner.lock();
    if (!owner)
    {
        fail("closed_resource", "命令所属窗口已释放");
    }
    require_window(owner.get(), allow_closed);
    if (state.closed && !allow_closed)
    {
        fail("closed_resource", "命令已经关闭");
    }
    return state;
}

void set_shortcut(command& state, const std::string& accelerator)
{
    const auto keys = parse_shortcut(accelerator);
    const auto owner = state.owner.lock();
    for (const auto& weak : owner->commands)
    {
        if (const auto other = weak.lock(); other && other.get() != &state && !other->closed &&
            !keys.key.empty() && other->keys == keys)
        {
            fail("ambiguous_shortcut", "同一窗口不能注册相同快捷键");
        }
    }
    state.keys = keys;
    state.accelerator = accelerator;
    owner->repaint = true;
}

std::shared_ptr<command> create_command(window& owner, const std::string& text,
    const std::string& accelerator, bool checkable)
{
    if (text.size() > 65536)
    {
        fail("resource_limit", "命令文字过长");
    }
    std::erase_if(owner.commands, [](const auto& value)
    {
        return value.expired();
    });
    if (owner.commands.size() >= 4096)
    {
        fail("resource_limit", "窗口命令不能超过 4096 个");
    }
    auto result = std::make_shared<command>();
    result->owner = owner.shared_from_this();
    result->id = owner.owner.lock()->next_id++;
    result->text = text;
    result->checkable = checkable;
    set_shortcut(*result, accelerator);
    owner.commands.push_back(result);
    return result;
}

void update_command(command& state)
{
    std::erase_if(state.bindings, [](const auto& value)
    {
        return value.expired();
    });
    for (const auto& weak : state.bindings)
    {
        if (const auto target = weak.lock(); target && !target->closed && target->action.get() == &state)
        {
            set_text(*target, state.text);
            target->checked = state.checked;
            dirty(*target);
        }
    }
    if (const auto owner = state.owner.lock(); owner && !owner->closed)
    {
        owner->repaint = true;
        if (owner->native_gui_root)
        {
            reset_interaction(*owner->native_gui_root);
        }
    }
}

void close_command(command& state)
{
    state.closed = true;
    update_command(state);
}

void bind_command(node& state, command& action)
{
    if (state.window.lock() != action.owner.lock())
    {
        fail("wrong_owner", "命令与控件必须属于同一窗口");
    }
    if (state.action.get() == &action)
    {
        return;
    }
    if (state.action)
    {
        std::erase_if(state.action->bindings, [&](const auto& value)
        {
            return value.expired() || value.lock().get() == &state;
        });
    }
    state.action = action.shared_from_this();
    action.bindings.push_back(state.shared_from_this());
    update_command(action);
}

void invoke_command(command& state)
{
    const auto owner = state.owner.lock();
    if (state.closed || !state.enabled || !owner || owner->closed || !owner->modal_child.expired() || owner->system_modal)
    {
        return;
    }
    if (state.checkable)
    {
        state.checked = !state.checked;
    }
    update_command(state);
    event notification{"command"};
    notification.source_id = state.id;
    notification.state = state.checked;
    enqueue(*owner, std::move(notification));
}

bool command_key(window& owner, const tx::ui::window_event& input)
{
    if (input.kind != tx::ui::event_kind::key_down || input.composing)
    {
        return false;
    }
    const shortcut keys{input.key, input.ctrl, input.shift, input.alt, input.meta};
    for (const auto& weak : owner.commands)
    {
        if (const auto action = weak.lock(); action && !action->closed && !action->keys.key.empty() && action->keys == keys)
        {
            if (!input.repeat)
            {
                invoke_command(*action);
            }
            return true;
        }
    }
    return false;
}
}
