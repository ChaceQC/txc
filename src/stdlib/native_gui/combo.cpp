#include "stdlib/native_gui/combo.hpp"

#include <algorithm>
#include <cmath>

namespace tx_generated::native_gui
{
void close_combo(node& root) noexcept
{
    if (const auto state = root.popup.lock(); state && state->combo)
    {
        const bool captured = state->combo->pointer_pressed && root.pressed.lock() == state;
        state->combo->open = false;
        state->combo->pointer_pressed = false;
        if (captured)
        {
            root.pressed.reset();
            try
            {
                if (const auto window = state->window.lock(); window && window->host)
                {
                    window->host->capture_pointer(false);
                }
            }
            catch (...)
            {
            }
        }
        if (const auto window = state->window.lock())
        {
            window->repaint = true;
        }
    }
    root.popup.reset();
}

void set_combo_items(node& state, std::vector<std::u32string> items)
{
    require_idle(state);
    std::size_t bytes = 0;
    if (items.size() > 10000)
    {
        fail("resource_limit", "下拉框最多支持 10000 项");
    }
    for (const auto& item : items)
    {
        const auto size = tx::ui::encode_utf8(item).size();
        bytes += size;
        if (size > 65536 || bytes > 16 * 1024 * 1024)
        {
            fail("resource_limit", "下拉选项文字超过大小上限");
        }
    }
    close_combo(root_node(state));
    state.combo->items = std::move(items);
    ++state.combo->generation;
    state.combo->selected = -1;
    state.text.clear();
    state.text_layout.reset();
    ++state.revision;
    dirty(state);
}

void select_combo(node& state, std::int64_t index, bool notify)
{
    require_idle(state);
    auto& combo = *state.combo;
    if (index < -1 || index >= static_cast<std::int64_t>(combo.items.size()))
    {
        fail("invalid_argument", "下拉选项索引越界");
    }
    if (combo.selected == index)
    {
        return;
    }
    combo.selected = index;
    state.text = index < 0 ? U"" : combo.items[index];
    state.text_layout.reset();
    ++state.revision;
    dirty(state);
    if (notify)
    {
        event notification{"selection_changed"};
        notification.source_id = state.id;
        notification.item_id = index + 1;
        notification.number = static_cast<double>(index);
        notification.text = tx::ui::encode_utf8(state.text);
        notification.revision = state.revision;
        enqueue(owner_window(state), std::move(notification));
    }
}

rectangle combo_popup(node& state)
{
    const auto& window = owner_window(state);
    const double width = window.width * 96.0 / window.dpi, height = window.height * 96.0 / window.dpi;
    const double below = std::max(0.0, height - state.bounds.y - state.bounds.height);
    const double above = std::clamp(state.bounds.y, 0.0, height);
    const double wanted = std::min<std::size_t>(8, state.combo->items.size()) * 32.0;
    const bool upward = below < wanted && above > below;
    const double extent = std::min(wanted, std::floor((upward ? above : below) / 32) * 32);
    const double popup_width = std::min(width, std::max(120.0, state.bounds.width));
    return {std::clamp(state.bounds.x, 0.0, std::max(0.0, width - popup_width)),
        upward ? above - extent : std::clamp(state.bounds.y + state.bounds.height, 0.0, height), popup_width, extent};
}

namespace
{
void reveal_candidate(node& state)
{
    auto& combo = *state.combo;
    const auto count = std::max<std::size_t>(1, static_cast<std::size_t>(combo_popup(state).height / 32));
    const auto index = static_cast<std::size_t>(std::max<std::int64_t>(0, combo.candidate));
    if (index < combo.first)
    {
        combo.first = index;
    }
    else if (index >= combo.first + count)
    {
        combo.first = index - count + 1;
    }
    combo.first = std::min(combo.first, combo.items.size() > count ? combo.items.size() - count : 0);
    owner_window(state).repaint = true;
}
}

void open_combo(node& state)
{
    if (!available(state) || state.combo->items.empty())
    {
        return;
    }
    auto& root = root_node(state);
    reset_interaction(root);
    layout(root);
    if (combo_popup(state).height < 32)
    {
        return;
    }
    state.combo->open = true;
    state.combo->candidate = std::max<std::int64_t>(0, state.combo->selected);
    root.popup = state.shared_from_this();
    reveal_candidate(state);
}

bool combo_key(node& state, const tx::ui::window_event& input)
{
    auto& combo = *state.combo;
    if (input.ctrl || input.meta || input.shift || (input.alt && input.key != "down" && input.key != "up"))
    {
        return false;
    }
    const bool toggle = input.key == "f4" || input.key == "space" || input.key == "enter" ||
        (input.alt && (input.key == "down" || input.key == "up"));
    const bool navigate = input.key == "down" || input.key == "up" || input.key == "home" ||
        input.key == "end" || input.key == "page_up" || input.key == "page_down";
    if (!toggle && !navigate && !(combo.open && input.key == "escape"))
    {
        return false;
    }
    if (input.kind != tx::ui::event_kind::key_down || (toggle && input.repeat))
    {
        return true;
    }
    if (toggle || input.key == "escape")
    {
        if (combo.open)
        {
            if (input.key == "enter" || input.key == "space")
            {
                select_combo(state, combo.candidate, true);
            }
            close_combo(root_node(state));
        }
        else if (input.key != "escape")
        {
            open_combo(state);
        }
        return true;
    }
    if (combo.items.empty())
    {
        return true;
    }
    auto index = combo.open ? combo.candidate : combo.selected;
    const auto last = static_cast<std::int64_t>(combo.items.size()) - 1;
    const auto page = std::max<std::int64_t>(1, static_cast<std::int64_t>(combo_popup(state).height / 32));
    if (input.key == "home" || input.key == "end")
    {
        index = input.key == "home" ? 0 : last;
    }
    else
    {
        index += input.key == "down" ? 1 : input.key == "up" ? -1 : input.key == "page_down" ? page : -page;
    }
    index = std::clamp(index, std::int64_t{0}, last);
    if (combo.open)
    {
        combo.candidate = index;
        reveal_candidate(state);
    }
    else
    {
        select_combo(state, index, true);
    }
    return true;
}

bool combo_pointer(node& root, const tx::ui::window_event& input)
{
    const auto state = root.popup.lock();
    if (!state || !state->combo || !available(*state))
    {
        close_combo(root);
        return false;
    }
    auto& combo = *state->combo;
    const auto bounds = combo_popup(*state);
    const bool inside = contains(bounds, input.x, input.y);
    if (input.kind == tx::ui::event_kind::wheel)
    {
        if (std::isfinite(input.wheel))
        {
            const auto count = static_cast<std::int64_t>(bounds.height / 32);
            const auto max_first = std::max<std::int64_t>(0, static_cast<std::int64_t>(combo.items.size()) - count);
            combo.first = static_cast<std::size_t>(std::clamp(static_cast<double>(combo.first) - input.wheel * 3, 0.0,
                static_cast<double>(max_first)));
            owner_window(root).repaint = true;
        }
        return true;
    }
    if (inside)
    {
        combo.candidate = static_cast<std::int64_t>(combo.first + (input.y - bounds.y) / 32);
        owner_window(root).repaint = true;
    }
    if (input.kind == tx::ui::event_kind::pointer_down)
    {
        if (!inside || input.button != 1)
        {
            close_combo(root);
        }
        else
        {
            combo.pointer_pressed = true;
            owner_window(root).host->capture_pointer(true);
            root.pressed = state;
        }
    }
    else if (input.kind == tx::ui::event_kind::pointer_up && input.button == 1 && combo.pointer_pressed)
    {
        if (inside)
        {
            select_combo(*state, combo.candidate, true);
        }
        reset_interaction(root);
    }
    return true;
}
}
