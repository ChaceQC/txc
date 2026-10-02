#include "stdlib/native_gui/core/text_layout.hpp"

#include <algorithm>
#include <stdexcept>

namespace tx::ui
{
namespace
{
bool before(rect first, rect second)
{
    return first.y < second.y || (first.y == second.y && first.x < second.x - 0.000001);
}

void require_direction(int direction)
{
    if (direction != -1 && direction != 1)
    {
        throw std::invalid_argument("视觉移动方向必须为 -1 或 1");
    }
}
}

void text_layout::build_carets()
{
    // 相邻字素在同一像素可能对应不同逻辑端点，不能按索引或横坐标去重。
    std::size_t cursor = 0;
    for (const auto& line : lines_)
    {
        const auto start = stops_.size();
        while (cursor < clusters_.size() && clusters_[cursor].bounds.y == line.y)
        {
            const auto& cluster = clusters_[cursor++];
            if (cluster.begin >= line.end)
            {
                continue;
            }
            const text_position leading{cluster.begin, caret_affinity::downstream};
            const text_position trailing{cluster.end, caret_affinity::upstream};
            stops_.push_back({cluster.right_to_left ? trailing : leading,
                {cluster.bounds.x, line.y, 1, line_height_}});
            stops_.push_back({cluster.right_to_left ? leading : trailing,
                {cluster.bounds.x + cluster.bounds.width, line.y, 1, line_height_}});
        }
        if (stops_.size() == start)
        {
            stops_.push_back({{line.begin}, {0, line.y, 1, line_height_}});
        }
    }
}

rect text_layout::caret(text_position position) const
{
    if (position.index > length_)
    {
        throw std::out_of_range("光标索引超过文本长度");
    }
    const caret_stop* fallback = nullptr;
    for (const auto& stop : stops_)
    {
        if (stop.position.index == position.index)
        {
            if (stop.position.affinity == position.affinity)
            {
                return stop.bounds;
            }
            fallback = &stop;
        }
    }
    if (fallback)
    {
        return fallback->bounds;
    }
    for (const auto& cluster : clusters_)
    {
        if (position.index >= cluster.begin && position.index < cluster.end)
        {
            return {cluster.bounds.x + (cluster.right_to_left ? cluster.bounds.width : 0),
                cluster.bounds.y, 1, line_height_};
        }
    }
    return stops_.back().bounds;
}

std::optional<rect> text_layout::alternate_caret(text_position position) const
{
    const auto primary = caret(position);
    const auto other = caret({position.index, position.affinity == caret_affinity::upstream ?
        caret_affinity::downstream : caret_affinity::upstream});
    return before(primary, other) || before(other, primary) ? std::optional<rect>(other) : std::nullopt;
}

text_position text_layout::hit_position(point position) const
{
    if (!std::isfinite(position.x) || !std::isfinite(position.y))
    {
        throw std::invalid_argument("文本命中坐标必须为有限数");
    }
    const auto row = static_cast<std::size_t>(std::clamp(std::floor(position.y / line_height_),
        0.0, static_cast<double>(lines_.size() - 1)));
    const auto y = lines_[row].y;
    const caret_stop* previous = nullptr;
    for (const auto& stop : stops_)
    {
        if (stop.bounds.y != y)
        {
            continue;
        }
        if (position.x < stop.bounds.x)
        {
            return previous && position.x < (previous->bounds.x + stop.bounds.x) / 2 ?
                previous->position : stop.position;
        }
        previous = &stop;
    }
    return previous->position;
}

text_position text_layout::move_visual(text_position position, int direction) const
{
    require_direction(direction);
    const auto current = caret(position);
    const caret_stop* previous = nullptr;
    for (const auto& stop : stops_)
    {
        if (direction > 0 && before(current, stop.bounds))
        {
            return stop.position;
        }
        if (direction < 0 && before(stop.bounds, current))
        {
            previous = &stop;
        }
    }
    return previous ? previous->position : position;
}

text_position text_layout::line_edge(text_position position, int direction) const
{
    require_direction(direction);
    const auto current = caret(position);
    const caret_stop* last = nullptr;
    for (const auto& stop : stops_)
    {
        if (stop.bounds.y == current.y)
        {
            if (direction < 0)
            {
                return stop.position;
            }
            last = &stop;
        }
    }
    return last->position;
}

text_position text_layout::selection_edge(std::size_t begin, std::size_t end, int direction) const
{
    require_direction(direction);
    if (begin > end || end > length_)
    {
        throw std::out_of_range("文本选区非法");
    }
    if (begin == end)
    {
        return {begin};
    }
    std::optional<caret_stop> result;
    for (const auto& cluster : clusters_)
    {
        if (cluster.begin >= end || cluster.end <= begin)
        {
            continue;
        }
        const bool leading = (direction < 0) != cluster.right_to_left;
        const caret_stop edge{{leading ? cluster.begin : cluster.end,
            leading ? caret_affinity::downstream : caret_affinity::upstream},
            {cluster.bounds.x + (direction > 0 ? cluster.bounds.width : 0), cluster.bounds.y, 1, line_height_}};
        if (!result || (direction < 0 ? before(edge.bounds, result->bounds) : before(result->bounds, edge.bounds)))
        {
            result = edge;
        }
    }
    return result ? result->position : text_position{begin};
}
}
