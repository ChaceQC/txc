#include "stdlib/native_gui/accessibility.hpp"
#include "stdlib/native_gui/commands.hpp"

namespace tx_generated::native_gui
{
void accessible_menu_tree(window& owner, accessible_tree& tree, tx::ui::point origin)
{
    const auto window_id = "w" + std::to_string(owner.id);
    const double scale = owner.dpi / 96;
    auto append = [&](menu& model, rectangle bounds, bool bar, int first, int selected)
    {
        accessible_node parent;
        parent.id = "m" + std::to_string(model.id);
        parent.parent = window_id;
        parent.role = "menu";
        parent.enabled = owner.modal_child.expired() && !owner.system_modal;
        parent.visible = owner.visible;
        parent.bounds = {origin.x + bounds.x * scale, origin.y + bounds.y * scale, bounds.width * scale, bounds.height * scale};
        for (std::size_t index = 0; index < model.items.size(); ++index)
        {
            const auto& item = model.items[index];
            if (!item.action && !item.submenu)
            {
                continue;
            }
            accessible_node value;
            value.id = "mi" + std::to_string(item.id);
            value.parent = parent.id;
            value.role = "menu_item";
            value.name = item.action ? item.action->text : item.text;
            value.help = item.action ? item.action->accelerator : "";
            value.node_id = model.id;
            value.item_id = index;
            value.item_kind = 4;
            value.enabled = parent.enabled && menu_item_enabled(item);
            value.visible = parent.visible;
            value.invoke = value.focusable = true;
            value.focused = static_cast<int>(index) == selected;
            value.checked = item.action && item.action->checked;
            value.checkable = item.action && item.action->checkable;
            rectangle cell = bar ? rectangle{index * 112.0, 0, 112, 32} :
                rectangle{bounds.x, bounds.y + (static_cast<int>(index) - first) * 30, bounds.width, 30};
            cell = tx::ui::intersect(cell, bounds);
            value.visible = value.visible && cell.width > 0 && cell.height > 0;
            value.bounds = {origin.x + cell.x * scale, origin.y + cell.y * scale, cell.width * scale, cell.height * scale};
            parent.children.push_back(value.id);
            tree.emplace(value.id, std::move(value));
        }
        tree[window_id].children.push_back(parent.id);
        tree.emplace(parent.id, std::move(parent));
    };
    if (menu_bar_height(owner))
    {
        append(*owner.menu_bar, {0, 0, owner.width / scale, 32}, true, 0,
            owner.menu_popup ? owner.menu_popup->bar_index : -1);
    }
    if (owner.menu_popup)
    {
        for (const auto& level : owner.menu_popup->levels)
        {
            append(*level.model, level.bounds, false, level.first, level.selected);
        }
    }
}
}
