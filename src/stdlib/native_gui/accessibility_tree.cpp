#include "stdlib/native_gui/accessibility.hpp"
#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/commands.hpp"
#include "stdlib/native_gui/combo.hpp"

namespace tx_generated::native_gui
{
namespace
{
std::string role(const node& state)
{
    if (!state.semantic_role.empty())
    {
        return state.semantic_role;
    }
    switch (state.kind)
    {
    case tx::graphics_kind::native_button: return "button";
    case tx::graphics_kind::native_label: return "label";
    case tx::graphics_kind::native_check_box: return "check_box";
    case tx::graphics_kind::native_radio_button: return "radio_button";
    case tx::graphics_kind::native_text_box: return "text";
    case tx::graphics_kind::native_slider: return "slider";
    case tx::graphics_kind::native_progress_bar: return "progress";
    case tx::graphics_kind::native_combo_box: return "combo_box";
    case tx::graphics_kind::native_list_view: return "list";
    case tx::graphics_kind::native_table_view: return "table";
    case tx::graphics_kind::native_tree_view: return "tree";
    case tx::graphics_kind::native_tabs: return "tabs";
    case tx::graphics_kind::native_scroll: return "scroll";
    default: return "panel";
    }
}

rectangle screen_bounds(window& owner, rectangle bounds, tx::ui::point origin)
{
    const double scale = owner.dpi / 96;
    return {origin.x + bounds.x * scale, origin.y + bounds.y * scale, bounds.width * scale, bounds.height * scale};
}

void items(node& state, accessible_node& parent, accessible_tree& tree, window& owner, tx::ui::point origin)
{
    auto add = [&](std::int64_t item, const std::u32string& text, bool selected, int kind, bool visible, rectangle bounds)
    {
        accessible_node child;
        child.id = parent.id + "_i" + std::to_string(item);
        if (kind == 2)
        {
            child.id += "_g" + std::to_string(state.combo->generation);
        }
        child.parent = parent.id;
        child.node_id = state.id;
        child.item_id = item;
        child.item_kind = kind;
        child.name = tx::ui::encode_utf8(text);
        child.role = kind == 3 ? "tab" : state.data && state.data->model.kind() == tx::ui::data_kind::tree ? "tree_item" : "list_item";
        child.enabled = parent.enabled;
        child.visible = parent.visible && visible;
        child.focusable = child.selectable = true;
        child.selected = selected;
        child.bounds = screen_bounds(owner, bounds, origin);
        child.focused = parent.focused && selected;
        parent.children.push_back(child.id);
        tree.emplace(child.id, std::move(child));
    };
    if (state.data)
    {
        arrange_data(state);
        for (const auto& row : state.data->model.snapshot().rows)
        {
            const auto position = state.data->positions.find(row.id);
            const bool visible = position != state.data->positions.end() && position->second >= state.data->first && position->second < state.data->last;
            std::u32string text;
            for (const auto& cell : row.cells)
            {
                if (!text.empty())
                {
                    text += U" | ";
                }
                text += cell;
            }
            rectangle bounds;
            if (visible)
            {
                bounds = tx::ui::intersect({state.scroll->viewport.x,
                    state.scroll->viewport.y + position->second * state.data->row_height - state.scroll->y,
                    state.scroll->viewport.width, state.data->row_height}, state.clip);
            }
            add(row.id, text, state.data->selected.contains(row.id), 1, visible, bounds);
        }
        parent.multiple = state.data->multiple;
    }
    if (state.combo)
    {
        for (std::size_t index = 0; index < state.combo->items.size(); ++index)
        {
            const auto popup = combo_popup(state);
            const auto bounds = tx::ui::intersect({popup.x, popup.y + (static_cast<double>(index) - state.combo->first) * 32,
                popup.width, 32}, popup);
            add(index, state.combo->items[index], state.combo->selected == static_cast<std::int64_t>(index), 2,
                state.combo->open && bounds.height > 0, bounds);
        }
    }
    if (state.tabs)
    {
        for (const auto& child : state.children)
        {
            rectangle bounds;
            for (const auto& header : state.tabs->headers)
            {
                if (header.first == child)
                {
                    bounds = tx::ui::intersect(header.second, state.clip);
                }
            }
            add(child->id, child->text, state.tabs->selected == child->id, 3, child->visible, bounds);
        }
    }
}

void visit(node& state, const std::string& parent, window& owner, tx::ui::point origin, accessible_tree& tree)
{
    if (state.closed)
    {
        return;
    }
    accessible_node value;
    value.id = "n" + std::to_string(state.id);
    value.parent = parent;
    value.node_id = state.id;
    value.role = role(state);
    value.name = state.accessible_name.empty() ? (state.editor ? "" : tx::ui::encode_utf8(state.text)) : state.accessible_name;
    value.help = state.accessible_help;
    value.enabled = available(state) && owner.modal_child.expired() && !owner.system_modal;
    value.visible = owner.visible && !owner.minimized && state.visible && state.layout_visible && state.clip.width > 0 && state.clip.height > 0;
    value.focused = root_node(state).window_focused && root_node(state).focused.lock().get() == &state;
    value.focusable = interactive(state);
    value.bounds = screen_bounds(owner, tx::ui::intersect(state.bounds, state.clip), origin);
    value.invoke = state.kind == tx::graphics_kind::native_button || state.kind == tx::graphics_kind::native_check_box || state.kind == tx::graphics_kind::native_radio_button;
    value.checkable = state.kind == tx::graphics_kind::native_check_box || state.kind == tx::graphics_kind::native_radio_button || (state.action && state.action->checkable);
    value.checked = state.checked;
    value.range = state.kind == tx::graphics_kind::native_slider || state.kind == tx::graphics_kind::native_progress_bar;
    value.value = state.value;
    value.minimum = state.minimum;
    value.maximum = state.maximum;
    value.step = state.step;
    value.editable = value.enabled && state.kind == tx::graphics_kind::native_slider;
    value.password = state.password;
    if (state.editor && !state.password)
    {
        value.text = state.editor->text();
        value.text_capable = true;
        value.editable = value.enabled && !state.editor->read_only;
        value.selection_start = state.editor->selection().first;
        value.selection_end = state.editor->selection().second;
        value.caret = state.editor->caret();
    }
    for (auto ancestor = state.parent.lock(); ancestor; ancestor = ancestor->parent.lock())
    {
        value.visible = value.visible && ancestor->visible && ancestor->layout_visible && !ancestor->closed;
    }
    items(state, value, tree, owner, origin);
    tree[parent].children.push_back(value.id);
    tree.emplace(value.id, std::move(value));
    const auto id = "n" + std::to_string(state.id);
    for (const auto& child : state.children)
    {
        visit(*child, id, owner, origin, tree);
    }
}
}

accessible_tree build_accessible_tree(window& owner)
{
    accessible_tree result;
    const auto origin = screen_origin(owner);
    accessible_node root;
    root.id = "w" + std::to_string(owner.id);
    root.name = owner.title;
    root.role = owner.modal_owner.expired() ? "window" : "dialog";
    root.enabled = owner.modal_child.expired() && !owner.system_modal;
    root.visible = owner.visible && !owner.minimized;
    root.bounds = {origin.x, origin.y, static_cast<double>(owner.width), static_cast<double>(owner.height)};
    result.emplace(root.id, root);
    if (owner.native_gui_root)
    {
        layout(*owner.native_gui_root);
        visit(*owner.native_gui_root, root.id, owner, origin, result);
    }
    accessible_menu_tree(owner, result, origin);
    return result;
}
}
