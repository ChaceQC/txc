#include "stdlib/native_gui/accessibility.hpp"
#include "stdlib/native_gui/dialogs.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
std::vector<std::pair<std::size_t, std::size_t>> accessibility_endpoint::visible_text_ranges(const std::string& id)
{
    return call([id](window& owner)
    {
        const auto tree = build_accessible_tree(owner);
        const auto& semantic = tree.at(id);
        const auto state = find_node(owner, semantic.node_id);
        std::vector<std::pair<std::size_t, std::size_t>> result;
        if (!state || !state->editor || state->password || !semantic.visible)
        {
            return result;
        }
        ensure_text(*state, std::max(1.0, state->bounds.width - 24));
        const auto viewport = tx::ui::intersect(state->clip, {state->bounds.x + 12, state->bounds.y + 8,
            std::max(0.0, state->bounds.width - 24), std::max(0.0, state->bounds.height - 16)});
        const double x = state->bounds.x + 12 - state->text_scroll_x;
        const double y = state->bounds.y + 8 - state->text_scroll_y;
        for (const auto& line : state->text_layout->lines())
        {
            const auto caret = state->text_layout->caret(line.begin);
            const auto bounds = tx::ui::intersect({x, y + line.y, std::max(1.0, line.width), caret.height}, viewport);
            if (bounds.width <= 0 || bounds.height <= 0)
            {
                continue;
            }
            auto first = line.begin, last = line.end;
            if (x < viewport.x || x + line.width > viewport.x + viewport.width)
            {
                const auto left = state->text_layout->hit({bounds.x - x, line.y + caret.height / 2});
                const auto right = state->text_layout->hit({bounds.x + bounds.width - x, line.y + caret.height / 2});
                first = std::min(left, right);
                last = std::max(left, right);
            }
            result.emplace_back(first, last);
        }
        return result;
    });
}

std::size_t accessibility_endpoint::text_at_point(const std::string& id, tx::ui::point point)
{
    return call([id, point](window& owner)
    {
        const auto tree = build_accessible_tree(owner);
        const auto& semantic = tree.at(id);
        const auto state = find_node(owner, semantic.node_id);
        if (!state || !state->editor || state->password)
        {
            throw std::invalid_argument("该辅助对象不公开文本");
        }
        ensure_text(*state, std::max(1.0, state->bounds.width - 24));
        const auto origin = screen_origin(owner);
        const double scale = owner.dpi / 96;
        return state->text_layout->hit({(point.x - origin.x) / scale - state->bounds.x - 12 + state->text_scroll_x,
            (point.y - origin.y) / scale - state->bounds.y - 8 + state->text_scroll_y});
    });
}

std::vector<std::size_t> accessibility_endpoint::text_units(const std::string& id, unsigned unit)
{
    return call([id, unit](window& owner)
    {
        const auto tree = build_accessible_tree(owner);
        const auto& semantic = tree.at(id);
        const auto state = find_node(owner, semantic.node_id);
        if (!state || !state->editor || state->password)
        {
            throw std::invalid_argument("该辅助对象不公开文本");
        }
        const auto& text = state->editor->text();
        if (unit == 0)
        {
            return tx::ui::grapheme_boundaries(text);
        }
        if (unit == 1)
        {
            return tx::ui::word_boundaries(text);
        }
        std::vector<std::size_t> result{0};
        if (unit == 2)
        {
            ensure_text(*state, std::max(1.0, state->bounds.width - 24));
            for (const auto& line : state->text_layout->lines())
            {
                result.push_back(line.begin);
            }
        }
        else if (unit == 3)
        {
            for (std::size_t index = 0; index < text.size(); ++index)
            {
                if (text[index] == U'\n')
                {
                    result.push_back(index + 1);
                }
            }
        }
        result.push_back(text.size());
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        return result;
    });
}
}
