#include "stdlib/gui/windows/accessibility.hpp"

#include <cmath>

namespace tx_generated::gui
{
HRESULT STDMETHODCALLTYPE accessibility_provider::QueryInterface(REFIID iid, void** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    if (iid == __uuidof(IUnknown) || iid == __uuidof(IRawElementProviderSimple))
    {
        *result = static_cast<IRawElementProviderSimple*>(this);
    }
    else if (iid == __uuidof(IRangeValueProvider) && split)
    {
        *result = static_cast<IRangeValueProvider*>(this);
    }
    else if (iid == __uuidof(IInvokeProvider) && !split)
    {
        *result = static_cast<IInvokeProvider*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE accessibility_provider::AddRef()
{
    return ++references;
}

ULONG STDMETHODCALLTYPE accessibility_provider::Release()
{
    const auto remaining = --references;
    if (!remaining)
    {
        delete this;
    }
    return remaining;
}

HRESULT STDMETHODCALLTYPE accessibility_provider::get_ProviderOptions(ProviderOptions* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = ProviderOptions_ServerSideProvider;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE accessibility_provider::GetPatternProvider(PATTERNID id, IUnknown** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    std::lock_guard lock(mutex);
    if (!hwnd)
    {
        return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (split && id == UIA_RangeValuePatternId)
    {
        *result = static_cast<IRangeValueProvider*>(this);
    }
    else if (!split && id == UIA_InvokePatternId)
    {
        *result = static_cast<IInvokeProvider*>(this);
    }
    if (*result)
    {
        AddRef();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE accessibility_provider::GetPropertyValue(PROPERTYID id, VARIANT* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    VariantInit(result);
    std::lock_guard lock(mutex);
    if (!hwnd)
    {
        return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (id == UIA_NamePropertyId || id == UIA_HelpTextPropertyId || id == UIA_AutomationIdPropertyId ||
        id == UIA_LocalizedControlTypePropertyId)
    {
        result->vt = VT_BSTR;
        const auto* text = id == UIA_NamePropertyId ? name.c_str() : id == UIA_HelpTextPropertyId ? help.c_str() :
            id == UIA_AutomationIdPropertyId ? identifier.c_str() : split ? L"分隔条" : L"画布";
        result->bstrVal = SysAllocString(text);
        return result->bstrVal ? S_OK : E_OUTOFMEMORY;
    }
    if (id == UIA_ControlTypePropertyId || id == UIA_NativeWindowHandlePropertyId)
    {
        result->vt = VT_I4;
        result->lVal = id == UIA_ControlTypePropertyId ?
            (split ? UIA_ThumbControlTypeId : UIA_CustomControlTypeId) : static_cast<LONG>(reinterpret_cast<INT_PTR>(hwnd));
    }
    else if (id == UIA_IsEnabledPropertyId || id == UIA_IsOffscreenPropertyId || id == UIA_HasKeyboardFocusPropertyId ||
        id == UIA_IsKeyboardFocusablePropertyId || id == UIA_IsControlElementPropertyId || id == UIA_IsContentElementPropertyId)
    {
        result->vt = VT_BOOL;
        const auto flag = id == UIA_IsEnabledPropertyId ? enabled : id == UIA_IsOffscreenPropertyId ? !visible :
            id == UIA_HasKeyboardFocusPropertyId ? focused : true;
        result->boolVal = flag ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else if (id == UIA_BoundingRectanglePropertyId)
    {
        result->parray = SafeArrayCreateVector(VT_R8, 0, 4);
        if (!result->parray)
        {
            return E_OUTOFMEMORY;
        }
        result->vt = VT_ARRAY | VT_R8;
        double values[]{static_cast<double>(rectangle.left), static_cast<double>(rectangle.top),
            static_cast<double>(rectangle.right - rectangle.left), static_cast<double>(rectangle.bottom - rectangle.top)};
        for (LONG index = 0; index < 4; ++index)
        {
            SafeArrayPutElement(result->parray, &index, &values[index]);
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE accessibility_provider::get_HostRawElementProvider(IRawElementProviderSimple** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    HWND handle;
    {
        std::lock_guard lock(mutex);
        handle = hwnd;
    }
    *result = nullptr;
    return handle ? UiaHostProviderFromHwnd(handle, result) : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT STDMETHODCALLTYPE accessibility_provider::SetValue(double requested)
{
    if (!std::isfinite(requested) || requested < 0 || requested > 1)
    {
        return E_INVALIDARG;
    }
    std::lock_guard lock(mutex);
    if (!hwnd)
    {
        return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (!enabled)
    {
        return UIA_E_ELEMENTNOTENABLED;
    }
    return PostMessageW(hwnd, accessibility_action, static_cast<WPARAM>(std::round(requested * 1000000)), 0) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE accessibility_provider::get_Value(double* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    std::lock_guard lock(mutex);
    *result = value;
    return hwnd ? S_OK : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT STDMETHODCALLTYPE accessibility_provider::get_IsReadOnly(BOOL* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    std::lock_guard lock(mutex);
    *result = !enabled;
    return hwnd ? S_OK : UIA_E_ELEMENTNOTAVAILABLE;
}

#define TX_UIA_RANGE(name, value) \
HRESULT STDMETHODCALLTYPE accessibility_provider::name(double* result) \
{ \
    if (!result) \
    { \
        return E_POINTER; \
    } \
    std::lock_guard lock(mutex); \
    *result = value; \
    return hwnd ? S_OK : UIA_E_ELEMENTNOTAVAILABLE; \
}
TX_UIA_RANGE(get_Maximum, 1)
TX_UIA_RANGE(get_Minimum, 0)
TX_UIA_RANGE(get_LargeChange, 0.1)
TX_UIA_RANGE(get_SmallChange, 0.02)
#undef TX_UIA_RANGE

HRESULT STDMETHODCALLTYPE accessibility_provider::Invoke()
{
    std::lock_guard lock(mutex);
    if (!hwnd)
    {
        return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (!enabled)
    {
        return UIA_E_ELEMENTNOTENABLED;
    }
    return PostMessageW(hwnd, accessibility_action, 0, 0) ? S_OK : E_FAIL;
}
} // namespace tx_generated::gui
