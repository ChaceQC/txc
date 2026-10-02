#include "stdlib/native_gui/core/font.hpp"

namespace tx::ui
{
const glyph_outline& font_face::cached_outline(std::uint16_t glyph) const
{
    const auto count_points = [](const glyph_outline& outline)
    {
        std::size_t count = 0;
        for (const auto& contour : outline)
        {
            count += contour.size();
        }
        return count;
    };
    auto found = outline_cache_.find(glyph);
    if (found == outline_cache_.end())
    {
        std::vector<std::uint16_t> chain;
        std::size_t budget = 65536;
        auto source = read_outline(glyph, chain, budget);
        const auto points = count_points(source);
        while (!cache_order_.empty() && (outline_cache_.size() >= 512 || cached_points_ + points > 1048576))
        {
            const auto oldest = outline_cache_.find(cache_order_.front());
            cached_points_ -= count_points(oldest->second);
            outline_cache_.erase(oldest);
            cache_order_.pop_front();
        }
        cache_order_.push_back(glyph);
        try
        {
            found = outline_cache_.emplace(glyph, std::move(source)).first;
        }
        catch (...)
        {
            cache_order_.pop_back();
            throw;
        }
        cached_points_ += points;
    }
    return found->second;
}
}
