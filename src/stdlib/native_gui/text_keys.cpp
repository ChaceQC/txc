#include "stdlib/native_gui/state.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
struct key_result
{
    bool handled = false, changed = false, selection = false;
};

key_result shortcut(node& state, const tx::ui::window_event& input)
{
    auto& editor = *state.editor;
    if (input.key == "a")
    {
        editor.select(0, editor.text().size());
        state.caret_affinity = tx::ui::caret_affinity::upstream;
        state.preferred_x.reset();
        return {true, false, true};
    }
    if (input.key == "c" || input.key == "x")
    {
        const auto [begin, end] = editor.selection();
        if (!state.password && begin != end)
        {
            owner_window(state).host->set_clipboard_text(std::u32string_view(editor.text()).substr(begin, end - begin));
            return {true, input.key == "x" && editor.erase(false), false};
        }
        return {true};
    }
    if (input.key == "v")
    {
        return {true, !editor.read_only && editor.insert(clipboard_text(owner_window(state))), false};
    }
    if (input.key == "z" || input.key == "y")
    {
        return {true, (input.key == "y" || input.shift) ? editor.redo() : editor.undo(), false};
    }
    return {};
}

void move_horizontal(node& state, const tx::ui::window_event& input)
{
    auto& editor = *state.editor;
    const int direction = input.key == "left" ? -1 : 1;
    if (input.ctrl)
    {
        editor.move_word(direction, input.shift);
        state.caret_affinity = direction < 0 ? tx::ui::caret_affinity::downstream : tx::ui::caret_affinity::upstream;
        return;
    }
    const auto [begin, end] = editor.selection();
    auto target = !input.shift && begin != end ? state.text_layout->selection_edge(begin, end, direction) :
        state.text_layout->move_visual({editor.caret(), state.caret_affinity}, direction);
    if (state.password)
    {
        // 掩码按标量绘制，编辑位置仍只能落到原文本的字素边界。
        const auto boundaries = tx::ui::grapheme_boundaries(editor.text());
        while (!std::binary_search(boundaries.begin(), boundaries.end(), target.index))
        {
            target = state.text_layout->move_visual(target, direction);
        }
    }
    editor.select(input.shift ? editor.anchor() : target.index, target.index);
    state.caret_affinity = target.affinity;
}

key_result navigate(node& state, const tx::ui::window_event& input)
{
    auto& editor = *state.editor;
    ensure_text(state, std::max(1.0, state.bounds.width - 24));
    if (input.key == "left" || input.key == "right")
    {
        move_horizontal(state, input);
        state.preferred_x.reset();
        return {true, false, true};
    }
    tx::ui::text_position target{editor.caret(), state.caret_affinity};
    const auto caret = state.text_layout->caret(target);
    if (input.key == "home" || input.key == "end")
    {
        target = input.ctrl ? tx::ui::text_position{input.key == "home" ? 0 : editor.text().size()} :
            state.text_layout->line_edge(target, input.key == "home" ? -1 : 1);
        state.preferred_x.reset();
    }
    else if (input.key == "up" || input.key == "down" || input.key == "page_up" || input.key == "page_down")
    {
        if (!state.preferred_x)
        {
            state.preferred_x = caret.x;
        }
        const double distance = input.key.starts_with("page_") ? std::max(caret.height, state.bounds.height - 16) : caret.height;
        const bool up = input.key == "up" || input.key == "page_up";
        target = state.text_layout->hit_position({*state.preferred_x, caret.y + caret.height / 2 + (up ? -distance : distance)});
    }
    else
    {
        return {};
    }
    editor.select(input.shift ? editor.anchor() : target.index, target.index);
    state.caret_affinity = editor.caret() == target.index ? target.affinity : tx::ui::caret_affinity::downstream;
    return {true, false, true};
}

key_result edit(node& state, const tx::ui::window_event& input)
{
    auto& editor = *state.editor;
    if (input.key == "backspace" || input.key == "delete")
    {
        return {true, input.ctrl ? editor.erase_word(input.key == "backspace") :
            editor.erase(input.key == "backspace"), false};
    }
    if (input.key == "enter")
    {
        if (input.repeat && (!editor.multiline || input.ctrl))
        {
            return {true};
        }
        if (editor.multiline && !input.ctrl)
        {
            return {true, editor.insert(U"\n"), false};
        }
        event notification{"text_committed"};
        notification.source_id = state.id;
        notification.text = state.password ? "" : tx::ui::encode_utf8(editor.text());
        notification.revision = state.revision;
        enqueue(owner_window(state), std::move(notification));
        return {true};
    }
    return navigate(state, input);
}
}

bool text_key(node& state, const tx::ui::window_event& input)
{
    if (input.kind != tx::ui::event_kind::key_down || input.composing || state.composing || input.alt || input.meta)
    {
        return false;
    }
    auto result = input.ctrl ? shortcut(state, input) : key_result{};
    if (!result.handled)
    {
        result = edit(state, input);
    }
    if (result.changed)
    {
        refresh_editor(state, true);
    }
    else if (result.selection)
    {
        selection_changed(state);
    }
    return result.handled;
}
}
