#include "stdlib/native_gui/state.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
void draw_editor(node& state, tx::ui::rasterizer& painter, tx::ui::color foreground, tx::ui::color accent)
{
    if (!state.text_layout)
    {
        return;
    }
    const tx::ui::rect viewport{state.bounds.x + 12, state.bounds.y + 8,
        std::max(0.0, state.bounds.width - 24), std::max(0.0, state.bounds.height - 16)};
    painter.push_clip(viewport);
    const tx::ui::point origin{viewport.x - state.text_scroll_x, viewport.y - state.text_scroll_y};
    const auto& editor = *state.editor;
    const auto& root = root_node(state);
    if (!state.composing)
    {
        const auto [begin, end] = editor.selection();
        auto selection_color = accent;
        selection_color.alpha = 80;
        for (const auto& bounds : state.text_layout->selection(begin, end))
        {
            painter.fill_rect({origin.x + bounds.x, origin.y + bounds.y, bounds.width, bounds.height}, selection_color);
        }
    }
    else if (state.composition_length)
    {
        const auto begin = editor.selection().first + state.composition_selection;
        auto color = accent;
        color.alpha = 100;
        for (const auto& bounds : state.text_layout->selection(begin, begin + state.composition_length))
        {
            painter.fill_rect({origin.x + bounds.x, origin.y + bounds.y, bounds.width, bounds.height}, color);
        }
    }
    state.text_layout->draw(painter, origin, foreground);
    if (state.composing)
    {
        const auto begin = editor.selection().first;
        for (const auto& bounds : state.text_layout->selection(begin, begin + state.composition.size()))
        {
            const double y = origin.y + bounds.y + bounds.height - 2;
            painter.line({origin.x + bounds.x, y}, {origin.x + bounds.x + bounds.width, y}, 1, accent);
        }
    }
    if (root.window_focused && root.focused.lock().get() == &state)
    {
        const auto index = state.composing ? editor.selection().first + state.composition_caret : editor.caret();
        const auto caret = state.text_layout->caret(index);
        painter.fill_rect({origin.x + caret.x, origin.y + caret.y, 1, caret.height}, foreground);
    }
    painter.pop_clip();
}
}
