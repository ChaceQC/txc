#include "stdlib/native_gui/state.hpp"
#include "stdlib/native_gui/containers.hpp"
#include "stdlib/native_gui/combo.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
void update_editor_viewport(node& state)
{
    if (state.bounds.width <= 0 || state.bounds.height <= 0)
    {
        return;
    }
    ensure_text(state, std::max(1.0, state.bounds.width - 24));
    const auto caret_index = state.composing ? state.editor->selection().first + state.composition_caret : state.editor->caret();
    const auto caret = state.text_layout->caret({caret_index,
        state.composing ? tx::ui::caret_affinity::upstream : state.caret_affinity});
    const double width = std::max(1.0, state.bounds.width - 24), height = std::max(1.0, state.bounds.height - 16);
    if (caret.x < state.text_scroll_x)
    {
        state.text_scroll_x = caret.x;
    }
    else if (caret.x + 1 > state.text_scroll_x + width)
    {
        state.text_scroll_x = caret.x + 1 - width;
    }
    if (caret.y < state.text_scroll_y)
    {
        state.text_scroll_y = caret.y;
    }
    else if (caret.y + caret.height > state.text_scroll_y + height)
    {
        state.text_scroll_y = caret.y + caret.height - height;
    }
    state.text_scroll_x = std::max(0.0, state.text_scroll_x);
    state.text_scroll_y = std::max(0.0, state.text_scroll_y);
    const auto focus = root_node(state).focused.lock();
    if (focus.get() == &state && !state.editor->read_only)
    {
        try
        {
            owner_window(state).host->set_ime_rect({state.bounds.x + 12 + caret.x - state.text_scroll_x,
                state.bounds.y + 8 + caret.y - state.text_scroll_y, 1, caret.height});
        }
        catch (const std::runtime_error& error)
        {
            auto& root = root_node(state);
            if (!root.ime_warning_sent)
            {
                event notification{"input_method_error"};
                notification.source_id = state.id;
                notification.text = error.what();
                enqueue(owner_window(state), std::move(notification));
                root.ime_warning_sent = true;
            }
        }
    }
}

void selection_changed(node& state)
{
    event notification{"selection_changed"};
    notification.source_id = state.id;
    notification.revision = state.revision;
    enqueue(owner_window(state), std::move(notification));
    update_editor_viewport(state);
    owner_window(state).repaint = true;
}

std::u32string display_text(const node& state)
{
    if (!state.editor)
    {
        return state.text;
    }
    auto text = state.editor->text();
    if (state.composing)
    {
        const auto [begin, end] = state.editor->selection();
        text.replace(begin, end - begin, state.composition);
    }
    if (state.password)
    {
        for (auto& scalar : text)
        {
            if (scalar != U'\n')
            {
                scalar = U'•';
            }
        }
    }
    return text;
}

void refresh_editor(node& state, bool notify)
{
    const bool changed = state.text != state.editor->text();
    state.text = state.editor->text();
    state.text_layout.reset();
    state.text_width = -1;
    state.preferred_x.reset();
    state.caret_affinity = notify ? tx::ui::caret_affinity::upstream : tx::ui::caret_affinity::downstream;
    if (changed)
    {
        ++state.revision;
        if (notify)
        {
            event notification{"text_changed"};
            notification.source_id = state.id;
            notification.text = state.password ? "" : tx::ui::encode_utf8(state.text);
            notification.revision = state.revision;
            enqueue(owner_window(state), std::move(notification));
        }
    }
    dirty(state);
    update_editor_viewport(state);
}

void focus_node(node& root, const std::shared_ptr<node>& target)
{
    const auto previous = root.focused.lock();
    if (previous == target)
    {
        return;
    }
    close_combo(root);
    owner_window(root).host->enable_ime(false);
    if (previous && previous->editor && previous->composing)
    {
        previous->composing = false;
        previous->composition.clear();
        previous->text_layout.reset();
        dirty(*previous);
    }
    root.focused = target;
    if (target)
    {
        reveal_node(*target);
    }
    owner_window(root).host->enable_ime(target && target->editor && !target->editor->read_only && !target->password);
    if (target && target->editor)
    {
        update_editor_viewport(*target);
    }
    owner_window(root).repaint = true;
}

void text_pointer(node& state, double x, double y, bool extend)
{
    if (state.composing)
    {
        owner_window(state).host->cancel_composition();
        state.composing = false;
        state.composition.clear();
        state.text_layout.reset();
    }
    ensure_text(state, std::max(1.0, state.bounds.width - 24));
    const auto position = state.text_layout->hit_position({x - state.bounds.x - 12 + state.text_scroll_x,
        y - state.bounds.y - 8 + state.text_scroll_y});
    state.editor->select(extend ? state.editor->anchor() : position.index, position.index);
    state.caret_affinity = state.editor->caret() == position.index ? position.affinity : tx::ui::caret_affinity::downstream;
    state.preferred_x.reset();
    selection_changed(state);
}

void text_input(node& state, const tx::ui::window_event& event)
{
    if (state.editor->read_only)
    {
        return;
    }
    if (event.kind == tx::ui::event_kind::composition)
    {
        state.composing = event.composing;
        state.composition = event.text;
        state.composition_feedback = event.feedback;
        state.composition_caret = std::min(event.caret, state.composition.size());
        state.composition_selection = std::min(event.selection_start, state.composition.size());
        state.composition_length = std::min(event.selection_length, state.composition.size() - state.composition_selection);
        state.text_layout.reset();
        update_editor_viewport(state);
        owner_window(state).repaint = true;
    }
    else if (event.kind == tx::ui::event_kind::text_input)
    {
        state.composing = false;
        state.composition.clear();
        if (state.editor->insert(event.text))
        {
            refresh_editor(state, true);
        }
    }
}

}
