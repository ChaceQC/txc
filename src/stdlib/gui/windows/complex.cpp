#include "stdlib/gui/windows/complex.hpp"

#include <algorithm>
#include <cmath>

namespace tx_generated::gui
{

void value_event(node& state, const char* action, double value)
{
    graphics::event::control_data data;
    data.source_id = state.id;
    data.action = action;
    data.number = value;
    data.revision = state.revision;
    graphics::enqueue_control(owner_window(state), std::move(data));
}

std::shared_ptr<node> add_tab(node& state, const std::string& title)
{
    require_idle(state);
    if (state.children.size() >= 128)
    {
        fail("resource_limit", "分页控件最多包含 128 页");
    }
    auto text = wide_text(title);
    auto page = create(state, tx::graphics_kind::container);
    TCITEMW item{};
    item.mask = TCIF_TEXT;
    item.pszText = text.data();
    const auto index = static_cast<int>(state.children.size() - 1);
    if (SendMessageW(state.hwnd, TCM_INSERTITEMW, index, reinterpret_cast<LPARAM>(&item)) == -1)
    {
        close(*page);
        platform_fail(owner_window(state).owner.lock().get(), "添加分页", GetLastError());
    }
    page->text = title;
    page->page_active = false;
    ShowWindow(page->hwnd, SW_HIDE);
    if (state.selected_page.expired())
    {
        select_tab(state, *page);
    }
    ++state.revision;
    return page;
}

void select_tab(node& state, node& page, bool user)
{
    if (page.parent.lock().get() != &state || page.closed)
    {
        fail("invalid_argument", "分页只能选择自己的直属页面");
    }
    const auto previous = state.selected_page.lock();
    if (previous.get() == &page)
    {
        return;
    }
    const bool move_focus = previous && (GetFocus() == previous->hwnd || IsChild(previous->hwnd, GetFocus()));
    int index = 0;
    for (const auto& child : state.children)
    {
        child->page_active = child.get() == &page;
        ShowWindow(child->hwnd, child->page_active && child->visible ? SW_SHOWNA : SW_HIDE);
        if (child.get() == &page)
        {
            SendMessageW(state.hwnd, TCM_SETCURSEL, index, 0);
        }
        ++index;
    }
    state.selected_page = page.shared_from_this();
    ++state.revision;
    dirty(state);
    if (move_focus)
    {
        SetFocus(state.hwnd);
    }
    if (user)
    {
        graphics::event::control_data data;
        data.source_id = state.id;
        data.action = "selection_changed";
        data.item_id = page.id;
        data.revision = state.revision;
        graphics::enqueue_control(owner_window(state), std::move(data));
    }
}

void remove_page(node& state, node& page) noexcept
{
    if (state.kind != tx::graphics_kind::tabs || state.closed)
    {
        return;
    }
    const auto found = std::find_if(state.children.begin(), state.children.end(), [&](const auto& child)
    {
        return child.get() == &page;
    });
    const auto index = found - state.children.begin();
    SendMessageW(state.hwnd, TCM_DELETEITEM, index, 0);
    if (state.selected_page.lock().get() == &page)
    {
        state.selected_page.reset();
        if (state.children.size() > 1)
        {
            const auto adjacent = static_cast<std::size_t>(index) + 1 < state.children.size() ? index + 1 : index - 1;
            const auto& child = state.children[adjacent];
            child->page_active = true;
            state.selected_page = child;
            ShowWindow(child->hwnd, child->visible ? SW_SHOWNA : SW_HIDE);
        }
    }
    int native_index = 0;
    for (const auto& child : state.children)
    {
        if (child.get() == &page)
        {
            continue;
        }
        if (state.selected_page.lock() == child)
        {
            SendMessageW(state.hwnd, TCM_SETCURSEL, native_index, 0);
        }
        ++native_index;
    }
    ++state.revision;
}

std::shared_ptr<node> create_scroll(node& parent, bool horizontal, bool vertical)
{
    if (!horizontal && !vertical)
    {
        fail("invalid_argument", "滚动容器必须启用至少一个方向");
    }
    auto result = create(parent, tx::graphics_kind::container);
    result->scroll_horizontal = horizontal;
    result->scroll_vertical = vertical;
    SetWindowLongPtrW(result->hwnd, GWL_STYLE, GetWindowLongPtrW(result->hwnd, GWL_STYLE) |
        WS_TABSTOP | (horizontal ? WS_HSCROLL : 0) | (vertical ? WS_VSCROLL : 0));
    SetWindowPos(result->hwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    return result;
}

void set_scroll_position(node& state, double x, double y)
{
    checked_dimension(x);
    checked_dimension(y);
    if ((!state.scroll_horizontal && x != 0) || (!state.scroll_vertical && y != 0) ||
        (!state.scroll_horizontal && !state.scroll_vertical))
    {
        fail("invalid_layout", "滚动位置必须属于已启用的滚动方向");
    }
    state.scroll_x = std::min(x, state.scroll_extent.width);
    state.scroll_y = std::min(y, state.scroll_extent.height);
    ++state.revision;
    dirty(state);
}

std::shared_ptr<node> create_split(node& parent, bool vertical, double position,
    double minimum_first, double minimum_second)
{
    checked_dimension(minimum_first);
    checked_dimension(minimum_second);
    if (!std::isfinite(position) || position < 0 || position > 1)
    {
        fail("invalid_argument", "分栏比例必须位于 [0,1]");
    }
    auto result = create(parent, tx::graphics_kind::container);
    try
    {
        create(*result, tx::graphics_kind::container);
        create(*result, tx::graphics_kind::container);
    }
    catch (...)
    {
        close(*result);
        throw;
    }
    result->split = true;
    result->layout = vertical ? layout_mode::column : layout_mode::row;
    result->split_vertical = vertical;
    result->split_position = position;
    result->split_minimum_first = minimum_first;
    result->split_minimum_second = minimum_second;
    SetWindowLongPtrW(result->hwnd, GWL_STYLE,
        GetWindowLongPtrW(result->hwnd, GWL_STYLE) | WS_TABSTOP | SS_NOTIFY);
    return result;
}

void set_split_position(node& state, double value, bool user)
{
    if (!state.split || !std::isfinite(value) || value < 0 || value > 1)
    {
        fail("invalid_argument", "分栏控件的比例必须位于 [0,1]");
    }
    if (state.split_position == value)
    {
        return;
    }
    state.split_position = value;
    ++state.revision;
    dirty(state);
    if (user)
    {
        value_event(state, "value_changed", value);
    }
    update_accessibility(state);
}

void set_progress(node& state, std::int64_t minimum, std::int64_t maximum, std::int64_t value)
{
    if (minimum < 0 || maximum > INT_MAX || maximum <= minimum || value < minimum || value > maximum)
    {
        fail("invalid_argument", "进度范围必须满足 0 <= minimum < maximum <= 2147483647，且值在范围内");
    }
    SendMessageW(state.hwnd, PBM_SETRANGE32, minimum, maximum);
    SendMessageW(state.hwnd, PBM_SETPOS, value, 0);
    state.range_minimum = minimum;
    state.range_maximum = maximum;
    state.range_value = value;
    ++state.revision;
}

void set_slider_value(node& state, std::int64_t value, bool user)
{
    if (value < state.range_minimum || value > state.range_maximum)
    {
        fail("invalid_argument", "滑块值超出范围");
    }
    SendMessageW(state.hwnd, TBM_SETPOS, TRUE, value);
    if (state.range_value != value)
    {
        state.range_value = value;
        ++state.revision;
        if (user)
        {
            value_event(state, "value_changed", static_cast<double>(value));
        }
    }
}
} // namespace tx_generated::gui
