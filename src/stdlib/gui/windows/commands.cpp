#include "stdlib/gui/windows/commands.hpp"

#include <algorithm>

namespace tx_generated::gui
{

command& require_command(graphics::resource* value)
{
    auto& result = graphics::asset<command>(value);
    const auto window = result.hosted_window.lock();
    if (!window || window->closed)
    {
        fail("closed_resource", "命令所属窗口已关闭");
    }
    return result;
}

menu& require_menu(graphics::resource* value)
{
    auto& result = graphics::asset<menu>(value);
    const auto window = result.hosted_window.lock();
    if (!window || window->closed)
    {
        fail("closed_resource", "菜单所属窗口已关闭");
    }
    return result;
}

std::shared_ptr<command> create_command(graphics::window& window, const std::string& text)
{
    wide_text(text);
    if (window.next_command_id > 65535)
    {
        fail("resource_limit", "窗口命令 ID 已耗尽");
    }
    const auto owner = window.owner.lock();
    auto result = graphics::own<command>(*owner);
    result->hosted_window = window.shared_from_this();
    result->id = owner->next_id++;
    result->native_id = window.next_command_id++;
    result->text = text;
    return result;
}

namespace
{

std::shared_ptr<command> share_command(command& value)
{
    for (const auto& resource : value.owner.lock()->resources)
    {
        if (resource.get() == &value)
        {
            return std::static_pointer_cast<command>(resource);
        }
    }
    fail("closed_resource", "命令已关闭");
}

void synchronize_node(node& state, command& value, const std::wstring& text)
{
    if (state.bound_command.get() == &value && !state.closed)
    {
        set_text(state, value.text);
        set_enabled(state, value.enabled);
        SendMessageW(state.hwnd, BM_SETCHECK, value.checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    if (state.toolbar && !state.closed)
    {
        TBBUTTONINFOW info{};
        info.cbSize = sizeof(info);
        info.dwMask = TBIF_STATE | TBIF_TEXT;
        info.fsState = static_cast<BYTE>((value.enabled ? TBSTATE_ENABLED : 0) | (value.checked ? TBSTATE_CHECKED : 0));
        info.pszText = const_cast<wchar_t*>(text.c_str());
        SendMessageW(state.hwnd, TB_SETBUTTONINFOW, value.native_id, reinterpret_cast<LPARAM>(&info));
        SendMessageW(state.hwnd, TB_AUTOSIZE, 0, 0);
    }
    for (const auto& child : state.children)
    {
        synchronize_node(*child, value, text);
    }
}

void synchronize(command& value)
{
    const auto window = value.hosted_window.lock();
    const auto wide = wide_text(value.text);
    for (const auto& resource : value.owner.lock()->resources)
    {
        if (resource->kind != tx::graphics_kind::menu || resource->closed)
        {
            continue;
        }
        auto& menu = *static_cast<gui::menu*>(resource.get());
        if (std::none_of(menu.commands.begin(), menu.commands.end(), [&](const auto& item)
            {
                return item.get() == &value;
            }))
        {
            continue;
        }
        MENUITEMINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_STRING | MIIM_STATE;
        info.dwTypeData = const_cast<wchar_t*>(wide.c_str());
        info.fState = (value.enabled ? MFS_ENABLED : MFS_DISABLED) | (value.checked ? MFS_CHECKED : MFS_UNCHECKED);
        if (!SetMenuItemInfoW(menu.handle, value.native_id, FALSE, &info))
        {
            platform_fail(value.owner.lock().get(), "更新菜单命令", GetLastError());
        }
    }
    if (window && window->gui_root)
    {
        synchronize_node(*window->gui_root, value, wide);
    }
    if (window && window->menu_bar)
    {
        DrawMenuBar(window->hwnd);
    }
}

} // namespace

void set_command(command& value, const std::string& text, bool enabled, bool checked)
{
    wide_text(text);
    auto old_text = value.text;
    const auto old_enabled = value.enabled, old_checked = value.checked;
    value.text = text;
    value.enabled = enabled;
    value.checked = checked;
    try
    {
        synchronize(value);
    }
    catch (...)
    {
        value.text = std::move(old_text);
        value.enabled = old_enabled;
        value.checked = old_checked;
        try
        {
            synchronize(value);
        }
        catch (...)
        {
        }
        throw;
    }
    ++value.revision;
}

void bind_button(node& button, command& value)
{
    if (button.window.lock() != value.hosted_window.lock())
    {
        fail("wrong_owner", "按钮与命令必须属于同一窗口");
    }
    button.bound_command = share_command(value);
    SendMessageW(button.hwnd, BM_SETSTYLE, BS_PUSHLIKE | BS_CHECKBOX, TRUE);
    synchronize(value);
}

void add_command(menu& target, command& value)
{
    if (target.hosted_window.lock() != value.hosted_window.lock())
    {
        fail("wrong_owner", "菜单与命令必须属于同一窗口");
    }
    if (std::any_of(target.commands.begin(), target.commands.end(), [&](const auto& item)
        {
            return item.get() == &value;
        }))
    {
        fail("duplicate_id", "同一菜单不能重复添加命令");
    }
    target.commands.push_back(share_command(value));
    const auto text = wide_text(value.text);
    if (!AppendMenuW(target.handle, MF_STRING | (value.enabled ? MF_ENABLED : MF_GRAYED) |
        (value.checked ? MF_CHECKED : MF_UNCHECKED), value.native_id, text.c_str()))
    {
        target.commands.pop_back();
        platform_fail(value.owner.lock().get(), "添加菜单命令", GetLastError());
    }
    DrawMenuBar(target.hosted_window.lock()->hwnd);
}

void add_tool(node& toolbar, command& value)
{
    if (!toolbar.toolbar || toolbar.window.lock() != value.hosted_window.lock())
    {
        fail("wrong_owner", "工具入口必须加入同窗口的 toolbar");
    }
    if (std::any_of(toolbar.tools.begin(), toolbar.tools.end(), [&](const auto& item)
        {
            return item.get() == &value;
        }))
    {
        fail("duplicate_id", "工具栏命令重复");
    }
    toolbar.tools.push_back(share_command(value));
    const auto text = wide_text(value.text);
    TBBUTTON button{};
    button.iBitmap = I_IMAGENONE;
    button.idCommand = static_cast<int>(value.native_id);
    button.fsState = static_cast<BYTE>((value.enabled ? TBSTATE_ENABLED : 0) | (value.checked ? TBSTATE_CHECKED : 0));
    button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    button.iString = reinterpret_cast<INT_PTR>(text.c_str());
    if (!SendMessageW(toolbar.hwnd, TB_ADDBUTTONSW, 1, reinterpret_cast<LPARAM>(&button)))
    {
        toolbar.tools.pop_back();
        platform_fail(value.owner.lock().get(), "添加工具栏命令", GetLastError());
    }
    dirty(toolbar);
}

bool activate_command(graphics::window& window, UINT native_id, std::int64_t source_id)
{
    for (const auto& resource : window.owner.lock()->resources)
    {
        if (resource->kind != tx::graphics_kind::command || resource->closed ||
            resource->hosted_window.lock().get() != &window)
        {
            continue;
        }
        auto& value = *static_cast<command*>(resource.get());
        if (value.native_id != native_id)
        {
            continue;
        }
        if (value.enabled && IsWindowEnabled(window.hwnd))
        {
            graphics::event::control_data data;
            data.source_id = source_id ? source_id : value.id;
            data.action = "activated";
            data.command_id = value.id;
            data.revision = value.revision;
            graphics::enqueue_control(window, std::move(data));
        }
        return true;
    }
    return false;
}

void command::release_native() noexcept
{
    enabled = false;
    shortcut.reset();
    try
    {
        synchronize(*this);
    }
    catch (...)
    {
    }
}

} // namespace tx_generated::gui
