#include "stdlib/native_gui/state.hpp"
#include "stdlib/native_gui/containers.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
void rebuild_visible(data_view_state& data)
{
    if (!data.rebuild && data.cached_revision == data.model.revision())
    {
        return;
    }
    data.visible.clear();
    data.positions.clear();
    const auto& snapshot = data.model.snapshot();
    if (data.model.kind() == tx::ui::data_kind::tree)
    {
        const auto visit = [&](auto&& self, std::int64_t parent, unsigned depth) -> void
        {
            const auto found = snapshot.children.find(parent);
            if (found == snapshot.children.end())
            {
                return;
            }
            for (const auto index : found->second)
            {
                const auto id = snapshot.rows[index].id;
                data.positions.emplace(id, data.visible.size());
                data.visible.push_back({index, depth});
                if (data.expanded.contains(id))
                {
                    self(self, id, depth + 1);
                }
            }
        };
        visit(visit, 0, 0);
    }
    else
    {
        for (std::size_t index = 0; index < snapshot.rows.size(); ++index)
        {
            data.positions.emplace(snapshot.rows[index].id, index);
            data.visible.push_back({index, 0});
        }
    }
    data.cached_revision = data.model.revision();
    data.rebuild = false;
}
}

void arrange_data(node& state)
{
    auto& data = *state.data;
    rebuild_visible(data);
    const bool table = data.model.kind() == tx::ui::data_kind::table;
    double width = state.bounds.width;
    if (table)
    {
        width = 0;
        for (const auto index : data.model.column_order())
        {
            const auto& column = data.model.columns()[index];
            width += column.visible ? column.width : 0;
        }
    }
    const double header = table ? std::min(32.0, state.bounds.height) : 0;
    update_scroll_geometry(*state.scroll, {state.bounds.x, state.bounds.y + header, state.bounds.width,
        std::max(0.0, state.bounds.height - header)}, width, data.visible.size() * data.row_height);
    const auto& scroll = *state.scroll;
    const auto visible = tx::ui::intersect(scroll.viewport, state.clip);
    const double top = scroll.y + std::max(0.0, visible.y - scroll.viewport.y);
    data.first = std::min(data.visible.size(), static_cast<std::size_t>(top / data.row_height));
    data.last = std::min(data.visible.size(), static_cast<std::size_t>(std::ceil(
        (top + visible.height) / data.row_height)));
    if (visible.width <= 0 || visible.height <= 0)
    {
        data.last = data.first;
    }
    // 缓存只保留当前视口中的行；十万行模型不会生成十万个文字布局。
    std::erase_if(data.text_cache, [&](const auto& item)
    {
        if (item.first.first == 0)
        {
            return false;
        }
        const auto found = data.positions.find(item.first.first);
        return found == data.positions.end() || found->second < data.first || found->second >= data.last;
    });
}
}
