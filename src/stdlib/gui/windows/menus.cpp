#include "stdlib/gui/windows/commands.hpp"

namespace tx_generated::gui
{

std::shared_ptr<menu> create_menu(graphics::window& window, menu* parent, const std::string& text)
{
    require_system_idle(window);
    const auto label = wide_text(text);
    const auto owner = window.owner.lock();
    auto value = graphics::own<menu>(*owner);
    value->hosted_window = window.shared_from_this();
    value->handle = parent ? CreatePopupMenu() : CreateMenu();
    if (!value->handle)
    {
        platform_fail(owner.get(), "创建菜单", GetLastError());
    }
    if (parent)
    {
        parent->children.push_back(value);
        if (!AppendMenuW(parent->handle, MF_POPUP, reinterpret_cast<UINT_PTR>(value->handle), label.c_str()))
        {
            parent->children.pop_back();
            graphics::close_owned(*value);
            platform_fail(owner.get(), "添加子菜单", GetLastError());
        }
        value->parent = parent->shared_from_this();
    }
    return value;
}

void menu::release_native() noexcept
{
    if (const auto window = hosted_window.lock(); attached && window && window->hwnd)
    {
        SetMenu(window->hwnd, nullptr);
        DrawMenuBar(window->hwnd);
        window->menu_bar.reset();
    }
    if (const auto parent_menu = parent.lock(); parent_menu && parent_menu->handle)
    {
        for (int index = GetMenuItemCount(parent_menu->handle) - 1; index >= 0; --index)
        {
            if (GetSubMenu(parent_menu->handle, index) == handle)
            {
                RemoveMenu(parent_menu->handle, index, MF_BYPOSITION);
                break;
            }
        }
    }
    for (const auto& child : children)
    {
        graphics::close_owned(*child);
    }
    children.clear();
    commands.clear();
    if (handle)
    {
        DestroyMenu(handle);
        handle = nullptr;
    }
}

void require_system_idle(graphics::window& window)
{
    if (window.owner.lock()->frame)
    {
        fail("invalid_frame", "活动绘图帧内不能执行系统交互");
    }
    if (window.gui_root && window.gui_root->notifying)
    {
        fail("modal_conflict", "原生通知期间不能重入系统交互");
    }
}

void show_modal(graphics::window& child, graphics::window& owner)
{
    require_system_idle(child);
    if (&child == &owner || child.owner.lock() != owner.owner.lock() || !child.modal_owner.expired())
    {
        fail("modal_conflict", "模态 owner 无效或窗口已经处于模态关系");
    }
    for (auto parent = owner.shared_from_this(); parent; parent = parent->modal_owner.lock())
    {
        if (parent.get() == &child)
        {
            fail("modal_conflict", "模态窗口关系不能形成环");
        }
    }
    for (const auto& window : owner.owner.lock()->windows)
    {
        if (!window->closed && window->modal_owner.lock().get() == &owner)
        {
            fail("modal_conflict", "owner 已有活动模态子窗口");
        }
    }
    child.owner_was_enabled = IsWindowEnabled(owner.hwnd) != FALSE;
    child.modal_owner = owner.shared_from_this();
    EnableWindow(owner.hwnd, FALSE);
    try
    {
        graphics::show_window(child);
        SetForegroundWindow(child.hwnd);
    }
    catch (...)
    {
        EnableWindow(owner.hwnd, child.owner_was_enabled);
        child.modal_owner.reset();
        throw;
    }
}

void close_interactions(graphics::window& state) noexcept
{
    const auto owner = state.owner.lock();
    if (!owner)
    {
        return;
    }
    for (const auto& window : owner->windows)
    {
        if (window->modal_owner.lock().get() == &state)
        {
            graphics::close_window(*window);
        }
    }
    if (const auto parent = state.modal_owner.lock(); parent && !parent->closed)
    {
        EnableWindow(parent->hwnd, state.owner_was_enabled);
    }
    state.modal_owner.reset();
    // 在 HWND 和 COM 会话销毁之前释放该窗口关联的命令、菜单及模型。
    for (const auto& value : owner->resources)
    {
        if (value->hosted_window.lock().get() == &state)
        {
            graphics::close_owned(*value);
        }
    }
    state.menu_bar.reset();
}

} // namespace tx_generated::gui
