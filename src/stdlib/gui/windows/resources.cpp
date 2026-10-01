#include "stdlib/gui/windows/state.hpp"

#include <algorithm>

namespace tx_generated::gui
{

graphics::window& owner_window(node& state)
{
    const auto window = state.window.lock();
    if (!window || window->closed)
    {
        fail("closed_resource", "控件所属窗口已关闭");
    }
    return *window;
}

node& require_node(graphics::resource* value, bool allow_closed)
{
    auto& state = *static_cast<node*>(value);
    graphics::require_thread(state.thread);
    if (const auto window = state.window.lock())
    {
        if (const auto app = window->owner.lock())
        {
            graphics::drain(*app);
        }
    }
    if (!allow_closed && (state.closed || !state.hwnd))
    {
        fail("closed_resource", "GUI 控件已经关闭");
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

void dirty(node& state)
{
    root_node(state).dirty = true;
}

void require_idle(node& state)
{
    const auto app = owner_window(state).owner.lock();
    if (app->frame)
    {
        fail("invalid_frame", "活动绘图帧内不能修改控件树或刷新布局");
    }
    if (root_node(state).notifying)
    {
        fail("invalid_layout", "原生通知期间不能重入布局");
    }
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

void create_native(node& state, HWND parent)
{
    using tx::graphics_kind;
    const wchar_t* name = L"STATIC";
    DWORD style = WS_CHILD | WS_VISIBLE;
    DWORD extended = 0;
    if (state.toolbar)
    {
        name = TOOLBARCLASSNAMEW;
        style |= WS_TABSTOP | TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER;
    }
    else if (state.kind == graphics_kind::container)
    {
        style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        extended = WS_EX_CONTROLPARENT;
    }
    else if (state.kind == graphics_kind::label)
    {
        style |= SS_LEFT | SS_NOPREFIX;
    }
    else if (state.kind == graphics_kind::text_box)
    {
        name = L"EDIT";
        style |= WS_TABSTOP | (state.multiline ?
            ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL : ES_AUTOHSCROLL);
        extended = WS_EX_CLIENTEDGE;
    }
    else if (state.kind == graphics_kind::list_view || state.kind == graphics_kind::table_view)
    {
        name = WC_LISTVIEWW;
        style |= WS_TABSTOP | LVS_OWNERDATA | LVS_SHOWSELALWAYS |
            (state.kind == graphics_kind::table_view ? LVS_REPORT : LVS_LIST) |
            (state.multiple ? 0 : LVS_SINGLESEL);
        extended = WS_EX_CLIENTEDGE;
    }
    else if (state.kind == graphics_kind::tree_view)
    {
        name = WC_TREEVIEWW;
        style |= WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS;
        extended = WS_EX_CLIENTEDGE;
    }
    else
    {
        name = L"BUTTON";
        style |= WS_TABSTOP | (state.kind == graphics_kind::button ? BS_PUSHBUTTON :
            state.three_state ? BS_AUTO3STATE : BS_AUTOCHECKBOX);
    }
    const auto text = wide_text(state.text);
    state.hwnd = CreateWindowExW(extended, name, text.c_str(), style, 0, 0, 0, 0,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!state.hwnd)
    {
        platform_fail(owner_window(state).owner.lock().get(), "创建 GUI 控件", GetLastError());
    }
    if (!SetWindowSubclass(state.hwnd, control_proc, 1, reinterpret_cast<DWORD_PTR>(&state)))
    {
        const auto code = GetLastError();
        DestroyWindow(state.hwnd);
        state.hwnd = nullptr;
        platform_fail(owner_window(state).owner.lock().get(), "安装控件通知", code);
    }
    if (state.kind == graphics_kind::text_box)
    {
        // Windows 按 UTF-16 单元限长，标量及 UTF-8 字节限额在通知边界再次检查。
        SendMessageW(state.hwnd, EM_SETLIMITTEXT, 32768, 0);
    }
    if (state.toolbar)
    {
        SendMessageW(state.hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    }
}

} // namespace

std::shared_ptr<node> root(graphics::window& window)
{
    if (window.gui_root)
    {
        return window.gui_root;
    }
    if (window.owner.lock()->frame)
    {
        fail("invalid_frame", "活动帧内不能创建 GUI 根容器");
    }
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES |
        ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES};
    if (!InitCommonControlsEx(&controls))
    {
        platform_fail(window.owner.lock().get(), "初始化系统控件", GetLastError());
    }
    auto result = std::make_shared<node>(tx::graphics_kind::container);
    result->window = window.shared_from_this();
    result->id = window.owner.lock()->next_id++;
    create_native(*result, window.hwnd);
    window.gui_root = result;
    // 顶层绘图不得覆盖原生子控件。
    SetWindowLongPtrW(window.hwnd, GWL_STYLE,
        GetWindowLongPtrW(window.hwnd, GWL_STYLE) | WS_CLIPCHILDREN);
    return result;
}

std::shared_ptr<node> create(node& parent, tx::graphics_kind kind,
    const std::string& text, bool option)
{
    require_idle(parent);
    if (parent.toolbar)
    {
        fail("invalid_layout", "toolbar 只接受 add_tool 命令入口");
    }
    if (parent.depth + 1 >= depth_limit || count_nodes(root_node(parent)) >= node_limit)
    {
        fail("resource_limit", "GUI 控件树超过 4096 节点或 64 层限制");
    }
    wide_text(text);
    auto result = std::make_shared<node>(kind);
    result->window = parent.window;
    result->parent = parent.shared_from_this();
    result->depth = parent.depth + 1;
    result->text = text;
    result->multiline = kind == tx::graphics_kind::text_box && option;
    result->three_state = kind == tx::graphics_kind::check_box && option;
    result->toolbar = kind == tx::graphics_kind::container && option;
    result->multiple = option && (kind == tx::graphics_kind::list_view || kind == tx::graphics_kind::table_view);
    result->id = owner_window(parent).owner.lock()->next_id++;
    // 先获得树的拥有权，再创建会同步触发 Windows 消息的对象。
    parent.children.push_back(result);
    try
    {
        create_native(*result, parent.hwnd);
        if (const auto font = root_node(parent).font)
        {
            SendMessageW(result->hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
    }
    catch (...)
    {
        parent.children.pop_back();
        throw;
    }
    dirty(parent);
    return result;
}

void close(node& state) noexcept
{
    if (state.closed)
    {
        return;
    }
    auto keep_alive = state.shared_from_this();
    if (state.hwnd && (GetFocus() == state.hwnd || IsChild(state.hwnd, GetFocus())))
    {
        try
        {
            advance_focus(root_node(state), &state);
        }
        catch (...)
        {
            if (const auto window = state.window.lock())
            {
                SetFocus(window->hwnd);
            }
        }
    }
    state.closed = true;
    state.bound_command.reset();
    state.tools.clear();
    state.model.reset();
    while (!state.children.empty())
    {
        close(*state.children.back());
    }
    if (state.hwnd)
    {
        DestroyWindow(state.hwnd);
        state.hwnd = nullptr;
    }
    if (state.font)
    {
        DeleteObject(state.font);
        state.font = nullptr;
    }
    if (const auto parent = state.parent.lock())
    {
        std::erase_if(parent->children, [&](const auto& child)
        {
            return child.get() == &state;
        });
        dirty(*parent);
    }
    else if (const auto window = state.window.lock(); window && window->gui_root.get() == &state)
    {
        window->gui_root.reset();
    }
}

void close_root(graphics::window& window) noexcept
{
    if (window.gui_root)
    {
        close(*window.gui_root);
    }
}

void window_changed(graphics::window& window, bool font_changed) noexcept
{
    if (window.gui_root)
    {
        window.gui_root->dirty = true;
        if (font_changed)
        {
            window.gui_root->font_dpi = 0;
        }
    }
}

} // namespace tx_generated::gui
