#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include "stdlib/gui/windows/complex.hpp"
#include "stdlib/gui/windows/layout_internal.hpp"

#include <algorithm>
#include <cmath>

namespace tx_generated::gui
{
namespace
{
void scroll_axis(node& state, bool horizontal)
{
    const auto scale = owner_window(state).dpi / 96.0;
    const auto viewport = std::max(0.0, (horizontal ? state.arranged.width : state.arranged.height) -
        ((horizontal ? state.scroll_vertical : state.scroll_horizontal) ?
            GetSystemMetricsForDpi(horizontal ? SM_CXVSCROLL : SM_CYHSCROLL, owner_window(state).dpi) / scale : 0));
    const auto content = horizontal ? state.natural.width : state.natural.height;
    if (!std::isfinite(content) || content > 16384)
    {
        fail("resource_limit", "滚动内容每轴不能超过 16384 DIP");
    }
    auto& position = horizontal ? state.scroll_x : state.scroll_y;
    position = std::clamp(position, 0.0, std::max(0.0, content - viewport));
    (horizontal ? state.scroll_extent.width : state.scroll_extent.height) = std::max(0.0, content - viewport);
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
    info.nMax = std::max(0, static_cast<int>(std::ceil(content)) - 1);
    info.nPage = static_cast<UINT>(std::floor(viewport));
    info.nPos = static_cast<int>(std::round(position));
    SetScrollInfo(state.hwnd, horizontal ? SB_HORZ : SB_VERT, &info, TRUE);
}
}

bool complex_layout(node& state)
{
    if (state.scroll_horizontal)
    {
        scroll_axis(state, true);
    }
    if (state.scroll_vertical)
    {
        scroll_axis(state, false);
    }
    if (state.kind == tx::graphics_kind::tabs)
    {
        const auto scale = owner_window(state).dpi / 96.0;
        RECT rect{0, 0, static_cast<LONG>(std::round(state.arranged.width * scale)),
            static_cast<LONG>(std::round(state.arranged.height * scale))};
        SendMessageW(state.hwnd, TCM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&rect));
        for (const auto& child : state.children)
        {
            if (child->page_active)
            {
                arrange(*child, {rect.left / scale, rect.top / scale,
                    std::max(0L, rect.right - rect.left) / scale,
                    std::max(0L, rect.bottom - rect.top) / scale});
            }
        }
        return true;
    }
    if (!state.split)
    {
        return false;
    }
    const auto width = state.arranged.width;
    const auto height = state.arranged.height;
    const auto axis = state.split_vertical ? height : width;
    const auto bar = std::min(6.0, axis);
    const auto space = std::max(0.0, axis - bar);
    const auto minimum = state.split_minimum_first + state.split_minimum_second;
    const auto first = space < minimum ? space * state.split_minimum_first / minimum :
        std::clamp(space * state.split_position, state.split_minimum_first, space - state.split_minimum_second);
    state.divider = state.split_vertical ? bounds{0, first, width, bar} : bounds{first, 0, bar, height};
    if (!state.children.empty())
    {
        arrange(*state.children[0], state.split_vertical ? bounds{0, 0, width, first} : bounds{0, 0, first, height});
    }
    if (state.children.size() > 1)
    {
        arrange(*state.children[1], state.split_vertical ? bounds{0, first + bar, width, space - first} :
            bounds{first + bar, 0, space - first, height});
    }
    InvalidateRect(state.hwnd, nullptr, FALSE);
    return true;
}
} // namespace tx_generated::gui
