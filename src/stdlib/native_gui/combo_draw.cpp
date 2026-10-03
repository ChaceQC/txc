#include "stdlib/native_gui/combo.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
void draw_combo(node& state, tx::ui::rasterizer& painter, const palette& theme)
{
    const auto& b = state.bounds;
    painter.fill_rounded_rect(b, 6, theme.border);
    painter.fill_rounded_rect({b.x + 1, b.y + 1, std::max(0.0, b.width - 2), std::max(0.0, b.height - 2)}, 5,
        available(state) ? theme.panel : theme.background);
    const double x = b.x + b.width - 16, y = b.y + b.height / 2;
    painter.line({x - 4, y - 2}, {x, y + 2}, 2, theme.muted);
    painter.line({x, y + 2}, {x + 4, y - 2}, 2, theme.muted);
}

void draw_combo_popup(node& root, tx::ui::rasterizer& painter, const palette& theme)
{
    const auto state = root.popup.lock();
    if (!state || !available(*state) || !state->combo->open)
    {
        return;
    }
    const auto bounds = combo_popup(*state);
    const auto& combo = *state->combo;
    painter.push_clip(bounds);
    painter.fill_rect(bounds, theme.border);
    const auto count = static_cast<std::size_t>(bounds.height / 32);
    for (std::size_t row = 0; row < count && combo.first + row < combo.items.size(); ++row)
    {
        const auto index = combo.first + row;
        const tx::ui::rect cell{bounds.x + 1, bounds.y + row * 32 + 1, std::max(0.0, bounds.width - 2), 30};
        painter.fill_rect(cell, static_cast<std::int64_t>(index) == combo.candidate ? theme.hover : theme.panel);
        painter.push_clip({cell.x + 8, cell.y, std::max(0.0, cell.width - 20), cell.height});
        tx::ui::text_layout text(*root.font, combo.items[index], root.font_size, std::max(1.0, cell.width - 20), false);
        text.draw(painter, {cell.x + 8, cell.y + std::max(0.0, (cell.height - text.height()) / 2)}, theme.foreground);
        painter.pop_clip();
    }
    if (count && combo.items.size() > count)
    {
        const double height = std::max(8.0, bounds.height * count / combo.items.size());
        const double y = bounds.y + (bounds.height - height) * combo.first / (combo.items.size() - count);
        painter.fill_rounded_rect({bounds.x + bounds.width - 5, y, 4, height}, 2, theme.accent);
    }
    painter.pop_clip();
}
}
