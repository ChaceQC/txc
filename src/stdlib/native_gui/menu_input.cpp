#include "stdlib/native_gui/commands.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
using tx::ui::event_kind;

void move_selection(menu_level& level, int direction)
{
    const auto count = static_cast<int>(level.model->items.size());
    if (!count)
    {
        return;
    }
    for (int attempt = 0; attempt < count; ++attempt)
    {
        level.selected = (level.selected + direction + count) % count;
        if (menu_item_enabled(level.model->items[level.selected]))
        {
            break;
        }
    }
    const auto rows = std::max(1, static_cast<int>(level.bounds.height / 30));
    level.first = std::clamp(level.first, std::max(0, level.selected - rows + 1), std::max(0, level.selected));
}

void execute_selected(window& owner, std::size_t index)
{
    const auto& level = owner.menu_popup->levels[index];
    if (level.selected < 0)
    {
        return;
    }
    const auto item = level.model->items[level.selected];
    if (!menu_item_enabled(item))
    {
        return;
    }
    if (item.submenu)
    {
        open_submenu(owner, index);
        move_selection(owner.menu_popup->levels.back(), 1);
    }
    else
    {
        close_menu(owner);
        invoke_command(*item.action);
    }
}

void open_bar(window& owner, int index)
{
    if (!owner.menu_bar || owner.menu_bar->closed || owner.menu_bar->items.empty())
    {
        return;
    }
    const auto count = static_cast<int>(owner.menu_bar->items.size());
    index = (index + count) % count;
    const auto item = owner.menu_bar->items[index];
    if (!menu_item_enabled(item))
    {
        return;
    }
    if (item.submenu)
    {
        show_menu(*item.submenu, index * 112.0, 32);
        owner.menu_popup->bar_index = index;
    }
    else
    {
        close_menu(owner);
        invoke_command(*item.action);
    }
}

bool menu_key(window& owner, const tx::ui::window_event& input)
{
    if (input.kind != event_kind::key_down)
    {
        return input.kind == event_kind::key_up || input.kind == event_kind::text_input;
    }
    if (input.repeat)
    {
        return true;
    }
    auto session = owner.menu_popup;
    auto& level = session->levels.back();
    if (input.key == "escape" || input.key == "left")
    {
        if (session->levels.size() > 1)
        {
            session->levels.pop_back();
        }
        else if (input.key == "left" && session->bar_index >= 0)
        {
            open_bar(owner, session->bar_index - 1);
        }
        else
        {
            close_menu(owner);
        }
    }
    else if (input.key == "down" || input.key == "up")
    {
        move_selection(level, input.key == "down" ? 1 : -1);
    }
    else if (input.key == "home" || input.key == "end")
    {
        level.selected = input.key == "home" ? -1 : 0;
        move_selection(level, input.key == "home" ? 1 : -1);
    }
    else if (input.key == "enter" || input.key == "space")
    {
        execute_selected(owner, session->levels.size() - 1);
    }
    else if (input.key == "right")
    {
        if (level.selected >= 0 && level.model->items[level.selected].submenu)
        {
            execute_selected(owner, session->levels.size() - 1);
        }
        else if (session->bar_index >= 0)
        {
            open_bar(owner, session->bar_index + 1);
        }
    }
    owner.repaint = true;
    return true;
}
}

bool menu_input(window& owner, const tx::ui::window_event& input)
{
    if (input.kind == event_kind::focus_lost || input.kind == event_kind::capture_lost)
    {
        close_menu(owner);
        return false;
    }
    const bool pointer = input.kind == event_kind::pointer_down || input.kind == event_kind::pointer_up ||
        input.kind == event_kind::pointer_moved || input.kind == event_kind::wheel;
    if (!owner.menu_popup)
    {
        if (input.kind == event_kind::key_down && !input.repeat && input.key == "f10" && !input.shift)
        {
            open_bar(owner, 0);
            return true;
        }
        if (input.kind == event_kind::pointer_down && input.button == 1 && input.y < menu_bar_height(owner))
        {
            open_bar(owner, static_cast<int>(input.x / 112));
            return true;
        }
        return false;
    }
    if (!pointer)
    {
        return menu_key(owner, input);
    }
    auto session = owner.menu_popup;
    for (std::size_t i = session->levels.size(); i-- > 0;)
    {
        auto& level = session->levels[i];
        if (!contains(level.bounds, input.x, input.y))
        {
            continue;
        }
        const int row = level.first + static_cast<int>((input.y - level.bounds.y) / 30);
        if (input.kind == event_kind::wheel)
        {
            const auto rows = std::max(1, static_cast<int>(level.bounds.height / 30));
            level.first = std::clamp(level.first + (input.wheel > 0 ? -3 : 3), 0,
                std::max(0, static_cast<int>(level.model->items.size()) - rows));
        }
        else if (row >= 0 && row < static_cast<int>(level.model->items.size()))
        {
            level.selected = row;
            if (input.kind == event_kind::pointer_down && input.button == 1)
            {
                session->pressed = true;
                owner.host->capture_pointer(true);
            }
            else if (input.kind == event_kind::pointer_up && input.button == 1 && session->pressed)
            {
                session->pressed = false;
                owner.host->capture_pointer(false);
                execute_selected(owner, i);
            }
        }
        owner.repaint = true;
        return true;
    }
    if (input.kind == event_kind::pointer_down)
    {
        if (input.y < menu_bar_height(owner) && input.button == 1)
        {
            open_bar(owner, static_cast<int>(input.x / 112));
        }
        else
        {
            close_menu(owner);
        }
    }
    else if (input.kind == event_kind::pointer_up)
    {
        session->pressed = false;
        owner.host->capture_pointer(false);
    }
    return true;
}
}
