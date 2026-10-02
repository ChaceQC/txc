#include "stdlib/native_gui/core/bidi_internal.hpp"

#include <algorithm>
#include <stdexcept>

namespace tx::ui
{
namespace
{
using type = bidi_type;

std::vector<std::size_t> matching_isolates(std::span<const type> types)
{
    std::vector<std::size_t> result(types.size(), types.size()), stack;
    for (std::size_t i = 0; i < types.size(); ++i)
    {
        if (bidi_isolate(types[i]))
        {
            stack.push_back(i);
        }
        else if (types[i] == type::pdi && !stack.empty())
        {
            result[stack.back()] = i;
            stack.pop_back();
        }
    }
    return result;
}

unsigned base_direction(std::span<const type> types, const std::vector<std::size_t>& matches,
    std::size_t start, std::size_t end)
{
    for (auto i = start; i < end; ++i)
    {
        if (types[i] == type::l)
        {
            return 0;
        }
        if (types[i] == type::r || types[i] == type::al)
        {
            return 1;
        }
        if (bidi_isolate(types[i]))
        {
            i = matches[i];
        }
    }
    return 0;
}

void explicit_levels(bidi_paragraph& result, std::vector<type>& types,
    const std::vector<std::size_t>& matches)
{
    struct status
    {
        unsigned level;
        type override;
        bool isolate;
    };
    std::vector<status> stack{{result.base, type::on, false}};
    unsigned overflow_isolate = 0, overflow_embedding = 0, valid_isolate = 0;
    for (std::size_t i = 0; i < types.size(); ++i)
    {
        auto current = types[i];
        result.levels[i] = stack.back().level;
        const bool isolate = bidi_isolate(current);
        if (isolate || current == type::rle || current == type::lre || current == type::rlo || current == type::lro)
        {
            const bool right = current == type::rle || current == type::rlo || current == type::rli ||
                (current == type::fsi && base_direction(result.original, matches, i + 1, matches[i]) == 1);
            const unsigned next = (stack.back().level + (right ? 1 : 2)) & ~1u;
            const unsigned level = right ? next + 1 : next;
            if (isolate && stack.back().override != type::on)
            {
                types[i] = stack.back().override;
            }
            if (level <= 125 && !overflow_isolate && !overflow_embedding)
            {
                stack.push_back({level, current == type::rlo ? type::r : current == type::lro ? type::l : type::on, isolate});
                valid_isolate += isolate;
            }
            else if (isolate)
            {
                ++overflow_isolate;
            }
            else if (!overflow_isolate)
            {
                ++overflow_embedding;
            }
        }
        else if (current == type::pdi)
        {
            if (overflow_isolate)
            {
                --overflow_isolate;
            }
            else if (valid_isolate)
            {
                overflow_embedding = 0;
                while (!stack.back().isolate)
                {
                    stack.pop_back();
                }
                stack.pop_back();
                --valid_isolate;
            }
            result.levels[i] = stack.back().level;
            if (stack.back().override != type::on)
            {
                types[i] = stack.back().override;
            }
        }
        else if (current == type::pdf)
        {
            if (!overflow_isolate)
            {
                if (overflow_embedding)
                {
                    --overflow_embedding;
                }
                else if (stack.size() > 1 && !stack.back().isolate)
                {
                    stack.pop_back();
                }
            }
        }
        else if (current == type::b)
        {
            result.levels[i] = result.base;
        }
        else if (current != type::bn && stack.back().override != type::on)
        {
            types[i] = stack.back().override;
        }
    }
}

void sequences(bidi_paragraph& result, std::vector<type>& types,
    const std::vector<std::size_t>& matches, std::u32string_view text)
{
    std::vector<std::size_t> active, run_of(types.size(), types.size());
    std::vector<std::vector<std::size_t>> runs;
    const auto explicit_level = result.levels;
    for (std::size_t i = 0; i < types.size(); ++i)
    {
        if (bidi_removed(result.original[i]))
        {
            continue;
        }
        if (active.empty() || explicit_level[active.back()] != explicit_level[i])
        {
            runs.emplace_back();
        }
        active.push_back(i);
        run_of[i] = runs.size() - 1;
        runs.back().push_back(i);
    }
    std::vector<bool> visited(runs.size());
    for (std::size_t r = 0; r < runs.size(); ++r)
    {
        if (visited[r])
        {
            continue;
        }
        std::vector<std::size_t> sequence;
        auto next = r;
        while (next < runs.size() && !visited[next])
        {
            visited[next] = true;
            sequence.insert(sequence.end(), runs[next].begin(), runs[next].end());
            const auto last = sequence.back();
            next = bidi_isolate(result.original[last]) && matches[last] < types.size() ? run_of[matches[last]] : runs.size();
        }
        const auto first = std::lower_bound(active.begin(), active.end(), sequence.front());
        const auto last = std::lower_bound(active.begin(), active.end(), sequence.back());
        const auto before = first == active.begin() ? result.base : explicit_level[*(first - 1)];
        const auto after = last + 1 == active.end() || bidi_isolate(result.original[*last]) ?
            result.base : explicit_level[*(last + 1)];
        resolve_bidi_sequence(types, result.levels, sequence,
            bidi_direction(std::max(before, explicit_level[sequence.front()])),
            bidi_direction(std::max(after, explicit_level[sequence.back()])), text);
    }
}
}

bidi_paragraph resolve_bidi_types(std::span<const bidi_type> types, int direction, std::u32string_view text)
{
    if (direction < -1 || direction > 1 || (!text.empty() && text.size() != types.size()))
    {
        throw std::invalid_argument("双向文本方向或属性长度非法");
    }
    bidi_paragraph result;
    result.original.assign(types.begin(), types.end());
    const auto matches = matching_isolates(types);
    result.base = direction < 0 ? base_direction(types, matches, 0, types.size()) : unsigned(direction);
    result.levels.resize(types.size(), result.base);
    auto resolved = result.original;
    explicit_levels(result, resolved, matches);
    sequences(result, resolved, matches, text);
    return result;
}

bidi_paragraph resolve_bidi(std::u32string_view text, int direction)
{
    std::vector<bidi_type> types;
    types.reserve(text.size());
    for (const auto scalar : text)
    {
        types.push_back(bidi_property(scalar));
    }
    return resolve_bidi_types(types, direction, text);
}

std::vector<unsigned> bidi_paragraph::line_levels(std::size_t begin, std::size_t end) const
{
    if (begin > end || end > levels.size())
    {
        throw std::out_of_range("双向文本行范围非法");
    }
    std::vector<unsigned> result(levels.begin() + begin, levels.begin() + end);
    const auto whitespace = [&](std::size_t i)
    {
        return original[i] == type::ws || bidi_isolate(original[i]) || original[i] == type::pdi || bidi_removed(original[i]);
    };
    for (auto i = begin; i <= end; ++i)
    {
        if (i == end || original[i] == type::b || original[i] == type::s)
        {
            if (i < end)
            {
                result[i - begin] = base;
            }
            auto previous = i;
            while (previous > begin && whitespace(previous - 1))
            {
                result[--previous - begin] = base;
            }
        }
    }
    return result;
}

std::vector<std::size_t> bidi_paragraph::visual_order(std::size_t begin, std::size_t end) const
{
    const auto resolved = line_levels(begin, end);
    std::vector<std::size_t> result;
    unsigned maximum = 0;
    for (auto i = begin; i < end; ++i)
    {
        if (!bidi_removed(original[i]))
        {
            result.push_back(i);
            maximum = std::max(maximum, resolved[i - begin]);
        }
    }
    for (unsigned level = maximum; level > 0; --level)
    {
        for (std::size_t start = 0; start < result.size();)
        {
            if (resolved[result[start] - begin] < level)
            {
                ++start;
                continue;
            }
            auto stop = start + 1;
            while (stop < result.size() && resolved[result[stop] - begin] >= level)
            {
                ++stop;
            }
            std::reverse(result.begin() + start, result.begin() + stop);
            start = stop;
        }
    }
    return result;
}
}
