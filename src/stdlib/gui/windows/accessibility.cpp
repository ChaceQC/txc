#include "stdlib/gui/windows/accessibility.hpp"
#include <oleacc.h>

namespace tx_generated::gui
{
namespace
{
graphics::com_ptr<IAccPropServices> annotation_service(node& state)
{
    IAccPropServices* service = nullptr;
    const auto status = CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC_SERVER,
        IID_IAccPropServices, reinterpret_cast<void**>(&service));
    if (FAILED(status))
    {
        platform_fail(owner_window(state).owner.lock().get(), "创建辅助功能属性服务", status);
    }
    return graphics::com_ptr<IAccPropServices>(service);
}

std::wstring accessible_name(node& state)
{
    if (!state.accessible_name.empty())
    {
        return wide_text(state.accessible_name);
    }
    if (const auto label = state.associated_label.lock(); label && !label->closed)
    {
        return wide_text(label->text);
    }
    return wide_text(state.password ? std::string{} : state.text);
}

bool visible_in_viewport(node& state, RECT rectangle)
{
    if (!IsWindowVisible(state.hwnd) || owner_window(state).minimized)
    {
        return false;
    }
    for (HWND parent = GetParent(state.hwnd); parent; parent = GetParent(parent))
    {
        RECT client{};
        GetClientRect(parent, &client);
        MapWindowPoints(parent, nullptr, reinterpret_cast<POINT*>(&client), 2);
        RECT visible{};
        if (!IntersectRect(&visible, &rectangle, &client))
        {
            return false;
        }
        rectangle = visible;
        if (parent == owner_window(state).hwnd)
        {
            break;
        }
    }
    return rectangle.right > rectangle.left && rectangle.bottom > rectangle.top;
}
}

void refresh_accessibility(node& state)
{
    update_accessibility(state);
    for (const auto& child : state.children)
    {
        refresh_accessibility(*child);
    }
}

void update_accessibility(node& state)
{
    if (state.closed || !state.hwnd)
    {
        return;
    }
    if (state.split || state.kind == tx::graphics_kind::gui_canvas)
    {
        if (!state.accessibility)
        {
            auto* provider = new accessibility_provider;
            provider->split = state.split;
            state.accessibility.reset(static_cast<IRawElementProviderSimple*>(provider));
        }
        auto& provider = *static_cast<accessibility_provider*>(
            static_cast<IRawElementProviderSimple*>(state.accessibility.get()));
        auto name = accessible_name(state);
        auto help = wide_text(state.help_text);
        auto identifier = std::to_wstring(state.id);
        bool enabled = IsWindowEnabled(state.hwnd) && IsWindowEnabled(owner_window(state).hwnd);
        for (auto parent = state.parent.lock(); parent; parent = parent->parent.lock())
        {
            enabled = enabled && parent->enabled;
        }
        const bool focused = GetFocus() == state.hwnd;
        RECT rectangle{};
        GetWindowRect(state.hwnd, &rectangle);
        const bool visible = visible_in_viewport(state, rectangle);
        double old_value;
        bool old_focus;
        {
            std::lock_guard lock(provider.mutex);
            old_value = provider.value;
            old_focus = provider.focused;
            provider.hwnd = state.hwnd;
            provider.name.swap(name);
            provider.help.swap(help);
            provider.identifier.swap(identifier);
            provider.enabled = enabled;
            provider.visible = visible;
            provider.focused = focused;
            provider.value = state.split_position;
            provider.rectangle = rectangle;
        }
        if (old_focus != focused && focused)
        {
            UiaRaiseAutomationEvent(&provider, UIA_AutomationFocusChangedEventId);
        }
        if (state.split && old_value != state.split_position)
        {
            VARIANT previous{}, current{};
            previous.vt = current.vt = VT_R8;
            previous.dblVal = old_value;
            current.dblVal = state.split_position;
            UiaRaiseAutomationPropertyChangedEvent(&provider, UIA_RangeValueValuePropertyId, previous, current);
        }
        return;
    }
    if (!state.annotated)
    {
        return;
    }
    auto name = accessible_name(state);
    auto help = wide_text(state.help_text);
    if (state.annotated_name == name && state.annotated_help == help)
    {
        return;
    }
    auto service = annotation_service(state);
    auto status = service->SetHwndPropStr(state.hwnd, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_NAME, name.c_str());
    if (SUCCEEDED(status))
    {
        status = service->SetHwndPropStr(state.hwnd, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_HELP, help.c_str());
    }
    if (FAILED(status))
    {
        service->SetHwndPropStr(state.hwnd, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_NAME, state.annotated_name.c_str());
        platform_fail(owner_window(state).owner.lock().get(), "设置辅助功能属性", status);
    }
    state.annotated_name.swap(name);
    state.annotated_help.swap(help);
    NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, state.hwnd, OBJID_CLIENT, CHILDID_SELF);
}

void set_accessibility(node& state, const std::string& name, const std::string& help)
{
    wide_text(name);
    wide_text(help);
    auto previous_name = state.accessible_name;
    auto previous_help = state.help_text;
    const bool annotated = state.annotated;
    state.accessible_name = name;
    state.help_text = help;
    state.annotated = true;
    try
    {
        update_accessibility(state);
    }
    catch (...)
    {
        state.accessible_name.swap(previous_name);
        state.help_text.swap(previous_help);
        state.annotated = annotated;
        throw;
    }
    ++state.revision;
}

void set_label(node& state, node& label)
{
    if (state.window.lock() != label.window.lock())
    {
        fail("invalid_argument", "辅助标签必须与控件属于同一窗口");
    }
    const auto previous = state.associated_label;
    state.associated_label = label.shared_from_this();
    try
    {
        set_accessibility(state, state.accessible_name, state.help_text);
    }
    catch (...)
    {
        state.associated_label = previous;
        throw;
    }
}

void close_accessibility(node& state) noexcept
{
    if (state.accessibility)
    {
        auto* provider = static_cast<accessibility_provider*>(
            static_cast<IRawElementProviderSimple*>(state.accessibility.get()));
        {
            std::lock_guard lock(provider->mutex);
            provider->hwnd = nullptr;
        }
        UiaDisconnectProvider(provider);
        state.accessibility.reset();
    }
    if (state.annotated && state.hwnd)
    {
        try
        {
            auto service = annotation_service(state);
            const MSAAPROPID properties[]{PROPID_ACC_NAME, PROPID_ACC_HELP};
            service->ClearHwndProps(state.hwnd, OBJID_CLIENT, CHILDID_SELF, properties, 2);
        }
        catch (...)
        {
            // 关闭路径不可抛出；销毁 HWND 后系统也会清理关联属性。
        }
    }
    state.annotated = false;
}

LRESULT accessibility_object(node& state, WPARAM wparam, LPARAM lparam)
{
    update_accessibility(state);
    return UiaReturnRawElementProvider(state.hwnd, wparam, lparam,
        static_cast<IRawElementProviderSimple*>(state.accessibility.get()));
}
} // namespace tx_generated::gui
