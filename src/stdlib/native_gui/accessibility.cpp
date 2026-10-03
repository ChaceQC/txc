#include "stdlib/native_gui/accessibility.hpp"
#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/combo.hpp"
#include "stdlib/native_gui/commands.hpp"
#include "stdlib/native_gui/containers.hpp"

#include <algorithm>
#include <cmath>

namespace tx_generated::native_gui
{
accessibility_endpoint::accessibility_endpoint(std::weak_ptr<window> owner)
    : owner_(owner), thread_(std::this_thread::get_id())
{
    root_id = "w" + std::to_string(owner.lock()->id);
}

void accessibility_endpoint::poll()
{
    std::deque<std::function<void()>> requests;
    {
        std::lock_guard lock(mutex_);
        requests.swap(pending_);
    }
    for (const auto& run : requests)
    {
        run();
    }
}

void accessibility_endpoint::disconnect()
{
    std::lock_guard lock(mutex_);
    connected_ = false;
    pending_.clear();
}

accessible_tree accessibility_endpoint::tree()
{
    return call([](window& owner)
    {
        return build_accessible_tree(owner);
    });
}

bool accessibility_endpoint::action(const accessible_action& request)
{
    return call([request](window& owner)
    {
        return perform_accessible_action(owner, request);
    });
}

std::shared_ptr<node> find_node(window& owner, std::int64_t id)
{
    std::function<std::shared_ptr<node>(const std::shared_ptr<node>&)> find = [&](const auto& value) -> std::shared_ptr<node>
    {
        if (!value || value->closed)
        {
            return {};
        }
        if (value->id == id)
        {
            return value;
        }
        for (const auto& child : value->children)
        {
            if (const auto found = find(child))
            {
                return found;
            }
        }
        return {};
    };
    return find(owner.native_gui_root);
}

std::vector<rectangle> accessibility_endpoint::text_bounds(const std::string& id, std::size_t begin, std::size_t end)
{
    return call([id, begin, end](window& owner)
    {
        const auto tree = build_accessible_tree(owner);
        const auto found = tree.find(id);
        std::vector<rectangle> result;
        if (found == tree.end() || found->second.password)
        {
            return result;
        }
        const auto state = find_node(owner, found->second.node_id);
        if (!state || !state->editor)
        {
            return result;
        }
        ensure_text(*state, std::max(1.0, state->bounds.width - 24));
        result = begin == end ? std::vector<rectangle>{state->text_layout->caret(begin)} : state->text_layout->selection(begin, end);
        const auto origin = screen_origin(owner);
        const auto scale = owner.dpi / 96;
        for (auto& box : result)
        {
            box.x += state->bounds.x + 12 - state->text_scroll_x;
            box.y += state->bounds.y + 8 - state->text_scroll_y;
            box = tx::ui::intersect(box, state->clip);
            box = {origin.x + box.x * scale, origin.y + box.y * scale, box.width * scale, box.height * scale};
        }
        return result;
    });
}

void set_accessibility(node& state, const std::string& name, const std::string& help)
{
    if (name.size() > 65536 || help.size() > 65536)
    {
        fail("resource_limit", "辅助名称或帮助文字过长");
    }
    tx::ui::decode_utf8(name);
    tx::ui::decode_utf8(help);
    state.accessible_name = name;
    state.accessible_help = help;
    dirty(state);
}

bool perform_accessible_action(window& owner, const accessible_action& request)
{
    const auto tree = build_accessible_tree(owner);
    const auto found = tree.find(request.target);
    if (found == tree.end() || !found->second.enabled || owner.system_modal || !owner.modal_child.expired())
    {
        return false;
    }
    const auto& semantic = found->second;
    if (semantic.item_kind == 4)
    {
        for (const auto& weak : owner.menus)
        {
            const auto menu = weak.lock();
            if (!menu || menu->closed || menu->id != semantic.node_id ||
                semantic.item_id < 0 || static_cast<std::size_t>(semantic.item_id) >= menu->items.size())
            {
                continue;
            }
            const auto item = menu->items[semantic.item_id];
            if (request.operation != "invoke" && request.operation != "focus")
            {
                return false;
            }
            if (item.submenu)
            {
                show_menu(*item.submenu, 0, menu == owner.menu_bar ? 32 : 0);
            }
            else if (item.action && request.operation == "invoke")
            {
                close_menu(owner);
                invoke_command(*item.action);
            }
            return true;
        }
        return false;
    }
    const auto target = find_node(owner, semantic.node_id);
    if (!target)
    {
        return false;
    }
    if (request.operation == "focus" && semantic.focusable)
    {
        owner.host->focus();
        focus_node(*owner.native_gui_root, target);
    }
    else if (request.operation == "invoke" && semantic.invoke)
    {
        activate(*target);
    }
    else if (request.operation == "value" && semantic.editable && target->editor && !target->password)
    {
        if (target->composing)
        {
            owner.host->cancel_composition();
        }
        target->composing = false;
        target->composition.clear();
        target->editor->set_text(request.text);
        refresh_editor(*target, true);
    }
    else if (request.operation == "range" && semantic.range && semantic.editable && std::isfinite(request.value) &&
        request.value >= semantic.minimum && request.value <= semantic.maximum)
    {
        target->value = request.value;
        event notification{"value_changed"};
        notification.source_id = target->id;
        notification.number = request.value;
        notification.revision = ++target->revision;
        enqueue(owner, std::move(notification));
    }
    else if (request.operation == "text_selection" && semantic.text_capable && target->editor &&
        request.start >= 0 && request.end >= 0 && std::uint64_t(request.start) <= semantic.text.size() &&
        std::uint64_t(request.end) <= semantic.text.size())
    {
        target->editor->select(request.start, request.end);
        selection_changed(*target);
    }
    else if (request.operation == "scroll_text" && semantic.text_capable && target->editor && request.start >= 0 &&
        static_cast<std::size_t>(request.start) <= semantic.text.size())
    {
        ensure_text(*target, std::max(1.0, target->bounds.width - 24));
        const auto caret = target->text_layout->caret(request.start);
        target->text_scroll_y = std::max(0.0, caret.y);
        target->text_scroll_x = std::max(0.0, caret.x - std::max(0.0, target->bounds.width - 24));
    }
    else if ((request.operation == "select" || request.operation == "add_selection" || request.operation == "remove_selection") && semantic.selectable)
    {
        if (target->data)
        {
            auto ids = target->data->selected;
            if (request.operation == "select" || !target->data->multiple)
            {
                ids.clear();
            }
            if (request.operation == "remove_selection")
            {
                ids.erase(semantic.item_id);
            }
            else
            {
                ids.insert(semantic.item_id);
            }
            set_data_selection(*target, {ids.begin(), ids.end()}, true);
            target->data->cursor = semantic.item_id;
            reveal_data_cursor(*target);
        }
        else if (target->combo)
        {
            select_combo(*target, request.operation == "remove_selection" ? -1 : semantic.item_id, true);
        }
        else if (target->tabs)
        {
            select_tab(*target, semantic.item_id, true);
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    owner.repaint = true;
    return true;
}
}
