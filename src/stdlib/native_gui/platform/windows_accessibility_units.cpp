#include "stdlib/native_gui/platform/windows_accessibility.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
std::vector<std::size_t> boundaries(uia_provider& owner, TextUnit unit)
{
    const unsigned selected = unit == TextUnit_Character ? 0 : unit == TextUnit_Word ? 1 :
        unit == TextUnit_Line ? 2 : unit == TextUnit_Paragraph ? 3 : 4;
    return owner.endpoint->text_units(owner.id, selected);
}
}

HRESULT STDMETHODCALLTYPE uia_text_range::ExpandToEnclosingUnit(TextUnit unit)
{
    return uia_call([&]() -> HRESULT
    {
        const auto value = owner.read();
        const auto stops = boundaries(owner, unit);
        auto after = std::upper_bound(stops.begin(), stops.end(), std::min(start, value.text.size()));
        if (after == stops.end() && after != stops.begin())
        {
            --after;
        }
        end = *after;
        start = after == stops.begin() ? 0 : *std::prev(after);
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_text_range::MoveEndpointByUnit(TextPatternRangeEndpoint endpoint, TextUnit unit, int count, int* moved)
{
    if (!moved)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = owner.read();
        const auto stops = boundaries(owner, unit);
        auto& position = endpoint == TextPatternRangeEndpoint_Start ? start : end;
        const auto found = std::lower_bound(stops.begin(), stops.end(), std::min(position, value.text.size()));
        const auto index = static_cast<std::int64_t>(found - stops.begin());
        const auto next = std::clamp(index + count, std::int64_t{0}, static_cast<std::int64_t>(stops.size() - 1));
        position = stops[next];
        *moved = static_cast<int>(next - index);
        if (start > end)
        {
            (endpoint == TextPatternRangeEndpoint_Start ? end : start) = position;
        }
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_text_range::Move(TextUnit unit, int count, int* moved)
{
    if (!moved)
    {
        return E_POINTER;
    }
    const bool empty = start == end;
    const auto status = MoveEndpointByUnit(TextPatternRangeEndpoint_Start, unit, count, moved);
    if (FAILED(status))
    {
        return status;
    }
    end = start;
    return empty ? S_OK : ExpandToEnclosingUnit(unit);
}

HRESULT STDMETHODCALLTYPE uia_text_range::MoveEndpointByRange(TextPatternRangeEndpoint endpoint,
    ITextRangeProvider* other, TextPatternRangeEndpoint other_endpoint)
{
    const auto value = dynamic_cast<uia_text_range*>(other);
    if (!value || value->owner.endpoint != owner.endpoint || value->owner.id != owner.id)
    {
        return E_INVALIDARG;
    }
    const auto position = other_endpoint == TextPatternRangeEndpoint_Start ? value->start : value->end;
    (endpoint == TextPatternRangeEndpoint_Start ? start : end) = position;
    if (start > end)
    {
        (endpoint == TextPatternRangeEndpoint_Start ? end : start) = position;
    }
    return S_OK;
}
}
