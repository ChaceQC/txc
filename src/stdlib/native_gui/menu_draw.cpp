#include "stdlib/native_gui/commands.hpp"
#include "stdlib/native_gui/drawing.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
void item_text(window& owner, tx::ui::rasterizer& painter, const menu_item& item,
    rectangle bounds, const palette& theme, bool bar)
{
    const auto text = item.action ? item.action->text : item.text;
    const auto foreground = menu_item_enabled(item) ? theme.foreground : theme.muted;
    const auto font = owner.owner.lock()->font;
    tx::ui::text_layout label(*font, tx::ui::decode_utf8(text).scalars, root(owner)->font_size,
        std::max(1.0, bounds.width - (bar ? 16 : 60)), false);
    painter.push_clip(bounds);
    label.draw(painter, {bounds.x + (bar ? 8 : 28), bounds.y + 5}, foreground);
    if (!bar && item.action && item.action->checked)
    {
        painter.line({bounds.x + 7, bounds.y + 15}, {bounds.x + 12, bounds.y + 20}, 2, foreground);
        painter.line({bounds.x + 12, bounds.y + 20}, {bounds.x + 21, bounds.y + 9}, 2, foreground);
    }
    if (!bar && item.submenu)
    {
        const double x = bounds.x + bounds.width - 12, y = bounds.y + 15;
        painter.line({x - 3, y - 4}, {x + 1, y}, 2, foreground);
        painter.line({x + 1, y}, {x - 3, y + 4}, 2, foreground);
    }
    if (!bar && item.action && !item.action->accelerator.empty())
    {
        tx::ui::text_layout keys(*font, tx::ui::decode_utf8(item.action->accelerator).scalars, 12, 100, false);
        painter.fill_rect({bounds.x + bounds.width - 106, bounds.y + 2, 100, 26}, theme.panel);
        keys.draw(painter, {bounds.x + bounds.width - 106, bounds.y + 7}, foreground);
    }
    painter.pop_clip();
}
}

void draw_menus(window& owner, tx::ui::rasterizer& painter, const palette& theme)
{
    if (menu_bar_height(owner))
    {
        painter.fill_rect({0, 0, owner.width * 96.0 / owner.dpi, 32}, theme.panel);
        double x = 0;
        for (const auto& item : owner.menu_bar->items)
        {
            item_text(owner, painter, item, {x, 0, 112, 32}, theme, true);
            x += 112;
        }
    }
    if (!owner.menu_popup)
    {
        return;
    }
    for (const auto& level : owner.menu_popup->levels)
    {
        painter.fill_rect(level.bounds, theme.border);
        painter.push_clip(level.bounds);
        for (int index = level.first; index < static_cast<int>(level.model->items.size()); ++index)
        {
            const auto& item = level.model->items[index];
            const rectangle row{level.bounds.x + 1, level.bounds.y + (index - level.first) * 30 + 1,
                level.bounds.width - 2, 28};
            if (row.y >= level.bounds.y + level.bounds.height)
            {
                break;
            }
            painter.fill_rect(row, index == level.selected ? theme.hover : theme.panel);
            if (!item.action && !item.submenu)
            {
                painter.line({row.x + 8, row.y + 14}, {row.x + row.width - 8, row.y + 14}, 1, theme.border);
            }
            else
            {
                item_text(owner, painter, item, row, theme, false);
            }
        }
        painter.pop_clip();
    }
}
}
