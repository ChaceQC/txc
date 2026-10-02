#include "stdlib/native_gui/core/bidi_internal.hpp"

#include <algorithm>
#include <string>

namespace tx::ui
{
namespace
{
using type = bidi_type;

type strong(type value)
{
    return value == type::en || value == type::an ? type::r : value;
}

void weak_types(std::vector<type>& types, type sos, type eos)
{
    auto previous = sos;
    for (auto& value : types)
    {
        if (value == type::nsm)
        {
            value = bidi_isolate(previous) || previous == type::pdi ? type::on : previous;
        }
        previous = value;
    }
    auto preceding_strong = sos;
    for (auto& value : types)
    {
        if (value == type::en && preceding_strong == type::al)
        {
            value = type::an;
        }
        if (value == type::l || value == type::r || value == type::al)
        {
            preceding_strong = value;
        }
    }
    std::replace(types.begin(), types.end(), type::al, type::r);
    for (std::size_t i = 1; i + 1 < types.size(); ++i)
    {
        if ((types[i] == type::es && types[i - 1] == type::en && types[i + 1] == type::en) ||
            (types[i] == type::cs && types[i - 1] == types[i + 1] &&
                (types[i - 1] == type::en || types[i - 1] == type::an)))
        {
            types[i] = types[i - 1];
        }
    }
    for (std::size_t i = 0; i < types.size();)
    {
        if (types[i] != type::et)
        {
            ++i;
            continue;
        }
        auto end = i + 1;
        while (end < types.size() && types[end] == type::et)
        {
            ++end;
        }
        if ((i ? types[i - 1] : sos) == type::en || (end < types.size() ? types[end] : eos) == type::en)
        {
            std::fill(types.begin() + i, types.begin() + end, type::en);
        }
        i = end;
    }
    preceding_strong = sos;
    for (auto& value : types)
    {
        if (value == type::es || value == type::et || value == type::cs)
        {
            value = type::on;
        }
        if (value == type::en && preceding_strong == type::l)
        {
            value = type::l;
        }
        if (value == type::l || value == type::r)
        {
            preceding_strong = value;
        }
    }
}

std::vector<std::pair<std::size_t, std::size_t>> bracket_pairs(const std::vector<type>& types,
    std::u32string_view text)
{
    std::vector<std::pair<char32_t, std::size_t>> stack;
    std::vector<std::pair<std::size_t, std::size_t>> result;
    const auto canonical = [](char32_t value)
    {
        return value == 0x232a ? char32_t(0x3009) : value;
    };
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        const auto* entry = bidi_bracket(text[i]);
        if (!entry || types[i] != type::on)
        {
            continue;
        }
        if (entry->opening)
        {
            // BD16：溢出使本序列的全部括号配对失效，不保留部分结果。
            if (stack.size() == 63)
            {
                return {};
            }
            stack.emplace_back(canonical(entry->paired), i);
        }
        else
        {
            for (auto j = stack.size(); j > 0; --j)
            {
                if (stack[j - 1].first == canonical(text[i]))
                {
                    result.emplace_back(stack[j - 1].second, i);
                    stack.resize(j - 1);
                    break;
                }
            }
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

void brackets(std::vector<type>& types, const std::vector<type>& original,
    std::u32string_view text, type embedding, type sos)
{
    for (const auto& [begin, end] : bracket_pairs(types, text))
    {
        bool same = false, opposite = false;
        for (auto i = begin + 1; i < end; ++i)
        {
            const auto value = strong(types[i]);
            same |= value == embedding;
            opposite |= (value == type::l || value == type::r) && value != embedding;
        }
        if (!same && !opposite)
        {
            continue;
        }
        auto resolved = embedding;
        if (!same)
        {
            auto preceding = sos;
            for (auto i = begin; i > 0; --i)
            {
                const auto value = strong(types[i - 1]);
                if (value == type::l || value == type::r)
                {
                    preceding = value;
                    break;
                }
            }
            resolved = preceding;
        }
        types[begin] = types[end] = resolved;
        for (const auto bracket : {begin, end})
        {
            for (auto i = bracket + 1; i < types.size() && original[i] == type::nsm; ++i)
            {
                types[i] = resolved;
            }
        }
    }
}

void neutral_types(std::vector<type>& types, type embedding, type sos, type eos)
{
    const auto neutral = [](type value)
    {
        return value == type::b || value == type::s || value == type::ws || value == type::on ||
            bidi_isolate(value) || value == type::pdi;
    };
    for (std::size_t i = 0; i < types.size();)
    {
        if (!neutral(types[i]))
        {
            ++i;
            continue;
        }
        auto end = i + 1;
        while (end < types.size() && neutral(types[end]))
        {
            ++end;
        }
        const auto before = i ? strong(types[i - 1]) : sos;
        const auto after = end < types.size() ? strong(types[end]) : eos;
        std::fill(types.begin() + i, types.begin() + end, before == after ? before : embedding);
        i = end;
    }
}
}

void resolve_bidi_sequence(std::vector<bidi_type>& types, std::vector<unsigned>& levels,
    const std::vector<std::size_t>& sequence, bidi_type sos, bidi_type eos, std::u32string_view text)
{
    std::vector<type> local;
    std::u32string characters;
    for (auto index : sequence)
    {
        local.push_back(types[index]);
        if (!text.empty())
        {
            characters.push_back(text[index]);
        }
    }
    const auto original = local;
    const auto embedding = bidi_direction(levels[sequence.front()]);
    weak_types(local, sos, eos);
    brackets(local, original, characters, embedding, sos);
    neutral_types(local, embedding, sos, eos);
    for (std::size_t i = 0; i < sequence.size(); ++i)
    {
        auto& level = levels[sequence[i]];
        if (level % 2 == 0)
        {
            level += local[i] == type::r ? 1 : local[i] == type::an || local[i] == type::en ? 2 : 0;
        }
        else
        {
            level += local[i] == type::l || local[i] == type::en || local[i] == type::an ? 1 : 0;
        }
        types[sequence[i]] = local[i];
    }
}
}
