#pragma once

#include "stdlib/gui/windows/complex.hpp"
#include <uiautomation.h>
#include <mutex>

namespace tx_generated::gui
{
// provider 不保存 node 指针；所有跨线程读取都限制在这份原生快照中。
class accessibility_provider final : public IRawElementProviderSimple,
    public IRangeValueProvider, public IInvokeProvider
{
public:
    std::mutex mutex;
    HWND hwnd = nullptr;
    bool split = false, enabled = false, visible = false, focused = false;
    double value = 0;
    RECT rectangle{};
    std::wstring name, help, identifier;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* result) override;
    HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID id, IUnknown** result) override;
    HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id, VARIANT* result) override;
    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** result) override;
    HRESULT STDMETHODCALLTYPE SetValue(double value) override;
    HRESULT STDMETHODCALLTYPE get_Value(double* result) override;
    HRESULT STDMETHODCALLTYPE get_IsReadOnly(BOOL* result) override;
    HRESULT STDMETHODCALLTYPE get_Maximum(double* result) override;
    HRESULT STDMETHODCALLTYPE get_Minimum(double* result) override;
    HRESULT STDMETHODCALLTYPE get_LargeChange(double* result) override;
    HRESULT STDMETHODCALLTYPE get_SmallChange(double* result) override;
    HRESULT STDMETHODCALLTYPE Invoke() override;
private:
    std::atomic<ULONG> references{1};
};
} // namespace tx_generated::gui
