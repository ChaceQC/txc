#include "stdlib/native_gui/commands.hpp"
#include "stdlib/native_gui/combo.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
menu& require_menu(graphics::resource* value, bool allow_closed)
{
    auto& state = *static_cast<menu*>(value);
    const auto owner = state.owner.lock();
    if (!owner)
    {
        fail("closed_resource", "菜单所属窗口已释放");
    }
    require_window(owner.get(), allow_closed);
    if (state.closed && !allow_closed)
    {
        fail("closed_resource", "菜单已经关闭");
    }
    return state;
}

std::shared_ptr<menu> create_menu(window& owner)
{
    auto result = std::make_shared<menu>();
    result->owner = owner.shared_from_this();
    result->id = owner.owner.lock()->next_id++;
    owner.menus.push_back(result);
    return result;
}

namespace
{
bool reaches(const menu& from, const menu* target, unsigned depth)
{
    if (&from == target || depth >= 32)
    {
        return true;
    }
    for (const auto& item : from.items)
    {
        if (item.submenu && reaches(*item.submenu, target, depth + 1))
        {
            return true;
        }
    }
    return false;
}
}

void append_menu(menu& state, std::string text, std::shared_ptr<command> action, std::shared_ptr<menu> submenu)
{
    if (state.items.size() >= 256 || text.size() > 65536)
    {
        fail("resource_limit", "每层菜单最多 256 项，文字不能超过 65536 字节");
    }
    if ((action && action->owner.lock() != state.owner.lock()) ||
        (submenu && submenu->owner.lock() != state.owner.lock()))
    {
        fail("wrong_owner", "菜单和命令必须属于同一窗口");
    }
    if (submenu && reaches(*submenu, &state, 0))
    {
        fail("invalid_argument", "菜单不能循环引用或嵌套超过 32 层");
    }
    state.items.push_back({std::move(text), std::move(action), std::move(submenu), state.owner.lock()->owner.lock()->next_id++});
    close_menu(*state.owner.lock());
    if (const auto root = state.owner.lock()->native_gui_root)
    {
        dirty(*root);
    }
}

bool menu_item_enabled(const menu_item& item)
{
    return item.action ? !item.action->closed && item.action->enabled : item.submenu && !item.submenu->closed;
}

double menu_bar_height(const window& owner)
{
    return owner.menu_bar && !owner.menu_bar->closed ? 32 : 0;
}

void close_menu(window& owner) noexcept
{
    if (!owner.menu_popup)
    {
        return;
    }
    const auto restore = owner.menu_popup->restore_focus.lock();
    owner.menu_popup.reset();
    try
    {
        owner.host->capture_pointer(false);
        if (owner.native_gui_root && restore && available(*restore))
        {
            focus_node(*owner.native_gui_root, restore);
        }
    }
    catch (...)
    {
    }
    owner.repaint = true;
}

void show_menu(menu& state, double x, double y)
{
    auto& owner = *state.owner.lock();
    if (!owner.modal_child.expired())
    {
        fail("modal_blocked", "模态窗口打开期间不能打开 owner 菜单");
    }
    checked_size(x);
    checked_size(y);
    auto panel = root(owner);
    close_menu(owner);
    reset_interaction(*panel);
    auto session = std::make_shared<menu_session>();
    session->restore_focus = panel->focused;
    focus_node(*panel, nullptr);
    const double width = owner.width * 96.0 / owner.dpi, height = owner.height * 96.0 / owner.dpi;
    const double w = std::min(280.0, width), h = std::min(state.items.size() * 30.0, height);
    session->levels.push_back({state.shared_from_this(),
        {std::clamp(x, 0.0, std::max(0.0, width - w)), std::clamp(y, 0.0, std::max(0.0, height - h)), w, h}});
    owner.menu_popup = std::move(session);
    owner.repaint = true;
}

void open_submenu(window& owner, std::size_t index)
{
    auto& levels = owner.menu_popup->levels;
    auto& level = levels[index];
    if (level.selected < 0 || !menu_item_enabled(level.model->items[level.selected]))
    {
        return;
    }
    const auto model = level.model->items[level.selected].submenu;
    if (!model)
    {
        return;
    }
    const double width = owner.width * 96.0 / owner.dpi, height = owner.height * 96.0 / owner.dpi;
    const double w = std::min(280.0, width), h = std::min(model->items.size() * 30.0, height);
    const double x = level.bounds.x + level.bounds.width + w <= width ? level.bounds.x + level.bounds.width : level.bounds.x - w;
    const double y = level.bounds.y + (level.selected - level.first) * 30;
    levels.resize(index + 1);
    levels.push_back({model, {std::max(0.0, x), std::clamp(y, 0.0, std::max(0.0, height - h)), w, h}});
    owner.repaint = true;
}
}
