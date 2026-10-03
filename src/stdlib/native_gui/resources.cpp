#include "stdlib/native_gui/state.hpp"
#include "stdlib/native_gui/containers.hpp"
#include "stdlib/native_gui/combo.hpp"
#include "stdlib/native_gui/commands.hpp"

#include <algorithm>
#include <cmath>

namespace tx_generated::native_gui
{
[[noreturn]] void fail(const char* code, const std::string& message)
{
    throw runtime_failure({tx::error_kind::graphics, code, message});
}

void require_thread(std::thread::id thread)
{
    if (thread != std::this_thread::get_id())
    {
        fail("wrong_thread", "GUI 资源只能在所属 UI 线程操作");
    }
}

window& owner_window(node& state)
{
    const auto value = state.window.lock();
    if (!value || value->closed || !value->host->is_open())
    {
        fail("closed_resource", "自绘控件所属窗口已经关闭");
    }
    return *value;
}

node& require_node(graphics::resource* value, bool allow_closed)
{
    auto& state = *static_cast<node*>(value);
    require_thread(state.thread);
    if (!allow_closed)
    {
        if (state.closed)
        {
            fail("closed_resource", "自绘控件已经关闭");
        }
        owner_window(state);
    }
    return state;
}

node& root_node(node& state)
{
    auto* current = &state;
    while (const auto parent = current->parent.lock())
    {
        current = parent.get();
    }
    return *current;
}

void require_idle(node& state)
{
    owner_window(state);
}

double checked_size(double value)
{
    if (!std::isfinite(value) || value < 0 || value > 16384)
    {
        fail("invalid_argument", "自绘布局尺寸必须是 0–16384 之间的有限数");
    }
    return value;
}

bool interactive(const node& state)
{
    return state.kind == tx::graphics_kind::native_button ||
        state.kind == tx::graphics_kind::native_check_box ||
        state.kind == tx::graphics_kind::native_radio_button ||
        state.kind == tx::graphics_kind::native_text_box ||
        state.kind == tx::graphics_kind::native_slider ||
        state.kind == tx::graphics_kind::native_combo_box ||
        state.kind == tx::graphics_kind::native_list_view ||
        state.kind == tx::graphics_kind::native_table_view ||
        state.kind == tx::graphics_kind::native_tree_view ||
        state.kind == tx::graphics_kind::native_tabs || state.kind == tx::graphics_kind::native_scroll ||
        state.kind == tx::graphics_kind::native_split || state.kind == tx::graphics_kind::native_canvas;
}

gui::length checked_length(const std::string& mode, double value)
{
    checked_size(value);
    if (mode == "auto")
    {
        return {};
    }
    if (mode == "fixed")
    {
        return {gui::length_mode::fixed, value};
    }
    if (mode == "stretch" && value > 0)
    {
        return {gui::length_mode::stretch, value};
    }
    fail("invalid_argument", "长度策略必须为 fixed/auto/stretch，伸展权重必须大于零");
}

bool available(const node& state)
{
    const auto* current = &state;
    while (current)
    {
        if (current->closed || !current->visible || !current->layout_visible || !current->enabled)
        {
            return false;
        }
        if (current->action && (current->action->closed || !current->action->enabled))
        {
            return false;
        }
        const auto parent = current->parent.lock();
        current = parent.get();
    }
    return true;
}

void dirty(node& state)
{
    root_node(state).dirty = true;
    owner_window(state).repaint = true;
}

namespace
{
std::size_t count_nodes(const node& state)
{
    std::size_t count = 1;
    for (const auto& child : state.children)
    {
        count += count_nodes(*child);
    }
    return count;
}

void release_tree(node& state) noexcept
{
    for (const auto& child : state.children)
    {
        release_tree(*child);
    }
    state.children.clear();
    state.text_layout.reset();
    state.editor.reset();
    state.composition.clear();
    state.font.reset();
    state.scroll.reset();
    state.tabs.reset();
    state.split.reset();
    state.canvas.reset();
    state.data.reset();
    state.combo.reset();
    state.closed = true;
}
}

std::shared_ptr<node> root(window& window)
{
    if (window.native_gui_root)
    {
        return window.native_gui_root;
    }
    auto result = std::make_shared<node>(tx::graphics_kind::native_panel);
    result->window = window.shared_from_this();
    result->id = window.owner.lock()->next_id++;
    result->font = window.owner.lock()->font;
    window.native_gui_root = result;
    dirty(*result);
    return result;
}

std::shared_ptr<node> create(node& parent, tx::graphics_kind kind, const std::string& text)
{
    require_idle(parent);
    if (parent.depth >= 63 || count_nodes(root_node(parent)) >= 4096)
    {
        fail("resource_limit", "自绘控件树超过 64 层或 4096 个节点");
    }
    if (text.size() > 65536)
    {
        fail("resource_limit", "控件文字超过 65536 字节");
    }
    auto value = std::make_shared<node>(kind);
    value->text = tx::ui::decode_utf8(text).scalars;
    if (kind == tx::graphics_kind::native_text_box)
    {
        value->editor = std::make_unique<tx::ui::text_buffer>();
        value->editor->set_text(value->text);
        value->height = {gui::length_mode::fixed, 42};
    }
    else if (kind == tx::graphics_kind::native_combo_box)
    {
        value->combo = std::make_shared<combo_state>();
        value->height = {gui::length_mode::fixed, 40};
    }
    else if (kind == tx::graphics_kind::native_progress_bar || kind == tx::graphics_kind::native_slider)
    {
        value->height = {gui::length_mode::fixed, kind == tx::graphics_kind::native_slider ? 40.0 : 16.0};
    }
    value->window = parent.window;
    value->parent = parent.shared_from_this();
    value->depth = parent.depth + 1;
    value->id = owner_window(parent).owner.lock()->next_id++;
    parent.children.push_back(value);
    dirty(parent);
    return value;
}

void close_root(window& window) noexcept
{
    if (const auto root = window.native_gui_root)
    {
        try
        {
            window.host->enable_ime(false);
        }
        catch (...)
        {
        }
        if (const auto focus = root->focused.lock(); focus && focus->composing)
        {
            try
            {
                window.host->cancel_composition();
            }
            catch (...)
            {
            }
        }
        reset_interaction(*root);
        release_tree(*root);
        window.native_gui_root.reset();
    }
}

void close(node& state)
{
    if (state.closed)
    {
        return;
    }
    require_idle(state);
    auto& window = owner_window(state);
    reset_interaction(root_node(state));
    if (const auto parent = state.parent.lock())
    {
        release_tree(state);
        std::erase_if(parent->children, [&](const auto& child)
        {
            return child.get() == &state;
        });
        normalize_tabs(*parent);
        reset_interaction(root_node(*parent));
        dirty(*parent);
    }
    else
    {
        close_root(window);
        window.repaint = true;
    }
}

void set_text(node& state, const std::string& text)
{
    require_idle(state);
    if (text.size() > (state.editor ? 4 * state.editor->limit : 65536))
    {
        fail("resource_limit", "控件文字超过长度上限");
    }
    auto replacement = tx::ui::decode_utf8(text).scalars;
    if (state.editor)
    {
        state.editor->set_text(std::move(replacement));
        state.composition.clear();
        state.composing = false;
        refresh_editor(state, false);
        return;
    }
    if (replacement != state.text)
    {
        state.text.swap(replacement);
        state.text_layout.reset();
        ++state.revision;
        dirty(state);
    }
}

void set_visible(node& state, bool value)
{
    require_idle(state);
    if (state.visible != value)
    {
        state.visible = value;
        if (const auto parent = state.parent.lock())
        {
            normalize_tabs(*parent);
        }
        reset_interaction(root_node(state));
        ++state.revision;
        dirty(state);
    }
}

void set_enabled(node& state, bool value)
{
    require_idle(state);
    if (state.enabled != value)
    {
        state.enabled = value;
        if (const auto parent = state.parent.lock())
        {
            normalize_tabs(*parent);
        }
        reset_interaction(root_node(state));
        ++state.revision;
        dirty(state);
    }
}
}
