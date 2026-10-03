#include "stdlib/native_gui/platform/windows_accessibility.hpp"

#include <algorithm>

// 旧版 MinGW 头文件缺少该系统导出的声明。
extern "C" BOOL WINAPI UiaClientsAreListening();

namespace tx_generated::native_gui
{
SAFEARRAY* uia_objects(const std::vector<IUnknown*>& values)
{
    const auto array = SafeArrayCreateVector(VT_UNKNOWN, 0, static_cast<ULONG>(values.size()));
    if (!array)
    {
        throw std::bad_alloc();
    }
    for (LONG index = 0; index < static_cast<LONG>(values.size()); ++index)
    {
        SafeArrayPutElement(array, &index, values[index]);
        values[index]->Release();
    }
    return array;
}

SAFEARRAY* uia_numbers(const std::vector<double>& values)
{
    const auto array = SafeArrayCreateVector(VT_R8, 0, static_cast<ULONG>(values.size()));
    if (!array)
    {
        throw std::bad_alloc();
    }
    for (LONG index = 0; index < static_cast<LONG>(values.size()); ++index)
    {
        auto value = values[index];
        SafeArrayPutElement(array, &index, &value);
    }
    return array;
}

uia_provider::uia_provider(std::shared_ptr<accessibility_endpoint> endpoint_value, std::string identifier, HWND handle)
    : endpoint(std::move(endpoint_value)), id(std::move(identifier)), hwnd(handle)
{
}

accessible_node uia_provider::read()
{
    const auto values = endpoint->tree();
    const auto found = values.find(id);
    if (found == values.end())
    {
        throw std::runtime_error("辅助对象已经关闭");
    }
    return found->second;
}

HRESULT uia_provider::act(accessible_action action)
{
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        if (!value.enabled)
        {
            return UIA_E_ELEMENTNOTENABLED;
        }
        action.target = id;
        return endpoint->action(action) ? S_OK : E_INVALIDARG;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::QueryInterface(REFIID iid, void** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
#define tx_uia_interface(type) \
    if (iid == __uuidof(type)) \
    { \
        *result = static_cast<type*>(this); \
    } \
    else
    tx_uia_interface(IRawElementProviderSimple)
    tx_uia_interface(IRawElementProviderFragment)
    tx_uia_interface(IRawElementProviderFragmentRoot)
    tx_uia_interface(IInvokeProvider)
    tx_uia_interface(IValueProvider)
    tx_uia_interface(IRangeValueProvider)
    tx_uia_interface(ISelectionProvider)
    tx_uia_interface(ISelectionItemProvider)
    tx_uia_interface(IToggleProvider)
    tx_uia_interface(ITextProvider)
    if (iid == __uuidof(IUnknown))
    {
        *result = static_cast<IRawElementProviderSimple*>(this);
    }
#undef tx_uia_interface
    if (!*result)
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE uia_provider::AddRef()
{
    return ++references_;
}

ULONG STDMETHODCALLTYPE uia_provider::Release()
{
    const auto count = --references_;
    if (!count)
    {
        delete this;
    }
    return count;
}

HRESULT STDMETHODCALLTYPE uia_provider::get_ProviderOptions(ProviderOptions* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = ProviderOptions_ServerSideProvider;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE uia_provider::GetPatternProvider(PATTERNID pattern, IUnknown** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        if (pattern == UIA_InvokePatternId && value.invoke)
        {
            *result = static_cast<IInvokeProvider*>(this);
        }
        else if (pattern == UIA_ValuePatternId && value.text_capable)
        {
            *result = static_cast<IValueProvider*>(this);
        }
        else if (pattern == UIA_RangeValuePatternId && value.range)
        {
            *result = static_cast<IRangeValueProvider*>(this);
        }
        else if (pattern == UIA_TextPatternId && value.text_capable)
        {
            *result = static_cast<ITextProvider*>(this);
        }
        else if (pattern == UIA_SelectionItemPatternId && value.selectable)
        {
            *result = static_cast<ISelectionItemProvider*>(this);
        }
        else if (pattern == UIA_SelectionPatternId && (value.role == "list" || value.role == "table" || value.role == "tree" || value.role == "tabs" || value.role == "combo_box"))
        {
            *result = static_cast<ISelectionProvider*>(this);
        }
        else if (pattern == UIA_TogglePatternId && value.checkable)
        {
            *result = static_cast<IToggleProvider*>(this);
        }
        if (*result)
        {
            AddRef();
        }
        return S_OK;
    });
}

namespace
{
int control_type(const std::string& role)
{
    const std::map<std::string, int> types{{"window", UIA_WindowControlTypeId}, {"dialog", UIA_WindowControlTypeId},
        {"button", UIA_ButtonControlTypeId}, {"label", UIA_TextControlTypeId}, {"text", UIA_EditControlTypeId},
        {"check_box", UIA_CheckBoxControlTypeId}, {"radio_button", UIA_RadioButtonControlTypeId},
        {"slider", UIA_SliderControlTypeId}, {"progress", UIA_ProgressBarControlTypeId},
        {"combo_box", UIA_ComboBoxControlTypeId}, {"list", UIA_ListControlTypeId}, {"list_item", UIA_ListItemControlTypeId},
        {"table", UIA_DataGridControlTypeId}, {"tree", UIA_TreeControlTypeId}, {"tree_item", UIA_TreeItemControlTypeId},
        {"tabs", UIA_TabControlTypeId}, {"tab", UIA_TabItemControlTypeId}, {"toolbar", UIA_ToolBarControlTypeId},
        {"status", UIA_StatusBarControlTypeId}, {"menu", UIA_MenuControlTypeId}, {"menu_item", UIA_MenuItemControlTypeId}};
    const auto found = types.find(role);
    return found == types.end() ? UIA_PaneControlTypeId : found->second;
}
}

HRESULT STDMETHODCALLTYPE uia_provider::GetPropertyValue(PROPERTYID property, VARIANT* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    VariantInit(result);
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        if (property == UIA_NamePropertyId || property == UIA_HelpTextPropertyId || property == UIA_AutomationIdPropertyId)
        {
            const auto& text = property == UIA_NamePropertyId ? value.name : property == UIA_HelpTextPropertyId ? value.help : id;
            const auto wide = tx::ui::utf16_text(tx::ui::decode_utf8(text).scalars);
            result->vt = VT_BSTR;
            result->bstrVal = SysAllocStringLen(wide.data(), static_cast<UINT>(wide.size()));
        }
        else if (property == UIA_ControlTypePropertyId || property == UIA_NativeWindowHandlePropertyId)
        {
            result->vt = VT_I4;
            result->lVal = property == UIA_ControlTypePropertyId ? control_type(value.role) :
                id == endpoint->root_id ? static_cast<LONG>(reinterpret_cast<INT_PTR>(hwnd)) : 0;
        }
        else if (property == UIA_BoundingRectanglePropertyId)
        {
            result->vt = VT_ARRAY | VT_R8;
            result->parray = uia_numbers({value.bounds.x, value.bounds.y, value.bounds.width, value.bounds.height});
        }
        else if (property == UIA_IsEnabledPropertyId || property == UIA_IsOffscreenPropertyId || property == UIA_HasKeyboardFocusPropertyId ||
            property == UIA_IsKeyboardFocusablePropertyId || property == UIA_IsPasswordPropertyId ||
            property == UIA_IsControlElementPropertyId || property == UIA_IsContentElementPropertyId)
        {
            result->vt = VT_BOOL;
            const bool flag = property == UIA_IsEnabledPropertyId ? value.enabled : property == UIA_IsOffscreenPropertyId ? !value.visible :
                property == UIA_HasKeyboardFocusPropertyId ? value.focused : property == UIA_IsKeyboardFocusablePropertyId ? value.focusable :
                property == UIA_IsPasswordPropertyId ? value.password : true;
            result->boolVal = flag ? VARIANT_TRUE : VARIANT_FALSE;
        }
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_HostRawElementProvider(IRawElementProviderSimple** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    return uia_call([&]() -> HRESULT
    {
        read();
        return id == endpoint->root_id ? UiaHostProviderFromHwnd(hwnd, result) : S_OK;
    });
}

namespace
{
class windows_accessibility_bridge final : public accessibility_bridge
{
public:
    explicit windows_accessibility_bridge(window& owner) : host_(static_cast<tx::ui::windows_window&>(*owner.host)), endpoint_(owner.accessibility)
    {
        provider_ = new uia_provider(endpoint_, endpoint_->root_id, host_.hwnd);
        host_.accessibility = [this](WPARAM wparam, LPARAM lparam)
        {
            return UiaReturnRawElementProvider(host_.hwnd, wparam, lparam, provider_);
        };
    }
    ~windows_accessibility_bridge() override
    {
        host_.accessibility = {};
        UiaDisconnectProvider(provider_);
        provider_->Release();
    }
    void poll(bool changed) override
    {
        if (!changed || !UiaClientsAreListening())
        {
            return;
        }
        const auto values = endpoint_->tree();
        for (const auto& [id, value] : values)
        {
            const auto previous = previous_.find(id);
            auto provider = new uia_provider(endpoint_, id, host_.hwnd);
            if (value.focused && (previous == previous_.end() || !previous->second.focused))
            {
                UiaRaiseAutomationEvent(provider, UIA_AutomationFocusChangedEventId);
            }
            if (value.text_capable && previous != previous_.end() && value.text != previous->second.text)
            {
                UiaRaiseAutomationEvent(provider, UIA_Text_TextChangedEventId);
            }
            if (previous != previous_.end())
            {
                const auto& old = previous->second;
                auto boolean = [&](PROPERTYID property, bool before, bool after)
                {
                    if (before == after)
                    {
                        return;
                    }
                    VARIANT first{}, second{};
                    first.vt = second.vt = VT_BOOL;
                    first.boolVal = before ? VARIANT_TRUE : VARIANT_FALSE;
                    second.boolVal = after ? VARIANT_TRUE : VARIANT_FALSE;
                    UiaRaiseAutomationPropertyChangedEvent(provider, property, first, second);
                };
                boolean(UIA_IsEnabledPropertyId, old.enabled, value.enabled);
                boolean(UIA_IsOffscreenPropertyId, !old.visible, !value.visible);
                boolean(UIA_HasKeyboardFocusPropertyId, old.focused, value.focused);
                if (old.selected != value.selected)
                {
                    UiaRaiseAutomationEvent(provider, value.selected ? UIA_SelectionItem_ElementSelectedEventId : UIA_SelectionItem_ElementRemovedFromSelectionEventId);
                }
                if (value.text_capable && (old.selection_start != value.selection_start || old.selection_end != value.selection_end))
                {
                    UiaRaiseAutomationEvent(provider, UIA_Text_TextSelectionChangedEventId);
                }
                if (old.checked != value.checked || old.value != value.value)
                {
                    VARIANT first{}, second{};
                    first.vt = second.vt = value.range ? VT_R8 : VT_I4;
                    if (value.range)
                    {
                        first.dblVal = old.value;
                        second.dblVal = value.value;
                    }
                    else
                    {
                        first.lVal = old.checked ? ToggleState_On : ToggleState_Off;
                        second.lVal = value.checked ? ToggleState_On : ToggleState_Off;
                    }
                    UiaRaiseAutomationPropertyChangedEvent(provider,
                        value.range ? UIA_RangeValueValuePropertyId : UIA_ToggleToggleStatePropertyId, first, second);
                }
                if (old.name != value.name)
                {
                    VARIANT first{}, second{};
                    first.vt = second.vt = VT_BSTR;
                    first.bstrVal = SysAllocString(tx::ui::utf16_text(tx::ui::decode_utf8(old.name).scalars).c_str());
                    second.bstrVal = SysAllocString(tx::ui::utf16_text(tx::ui::decode_utf8(value.name).scalars).c_str());
                    UiaRaiseAutomationPropertyChangedEvent(provider, UIA_NamePropertyId, first, second);
                    VariantClear(&first);
                    VariantClear(&second);
                }
                if (old.children != value.children)
                {
                    UiaRaiseStructureChangedEvent(provider, StructureChangeType_ChildrenInvalidated, nullptr, 0);
                }
            }
            provider->Release();
        }
        previous_ = values;
    }
private:
    tx::ui::windows_window& host_;
    std::shared_ptr<accessibility_endpoint> endpoint_;
    uia_provider* provider_;
    accessible_tree previous_;
};
}

std::shared_ptr<accessibility_bridge> connect_accessibility(window& owner)
{
    return std::make_shared<windows_accessibility_bridge>(owner);
}
}
