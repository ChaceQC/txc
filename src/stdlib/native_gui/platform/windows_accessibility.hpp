#pragma once

#include "stdlib/native_gui/platform/windows_window.hpp"
#include "stdlib/native_gui/accessibility.hpp"

#include <uiautomation.h>
#include <atomic>

namespace tx_generated::native_gui
{
template<class operation>
HRESULT uia_call(operation run) noexcept
{
    try
    {
        return run();
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return UIA_E_ELEMENTNOTAVAILABLE;
    }
}

SAFEARRAY* uia_objects(const std::vector<IUnknown*>& values);
SAFEARRAY* uia_numbers(const std::vector<double>& values);
class uia_provider final : public IRawElementProviderSimple, public IRawElementProviderFragment, public IRawElementProviderFragmentRoot,
    public IInvokeProvider, public IValueProvider, public IRangeValueProvider,
    public ISelectionProvider, public ISelectionItemProvider, public IToggleProvider, public ITextProvider
{
public:
    uia_provider(std::shared_ptr<accessibility_endpoint> endpoint, std::string id, HWND hwnd);
    std::shared_ptr<accessibility_endpoint> endpoint;
    std::string id;
    HWND hwnd;
    accessible_node read();
    HRESULT act(accessible_action action);
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* result) override;
    HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID pattern, IUnknown** result) override;
    HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID property, VARIANT* result) override;
    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** result) override;
    HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment** result) override;
    HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** result) override;
    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* result) override;
    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** result) override;
    HRESULT STDMETHODCALLTYPE SetFocus() override;
    HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** result) override;
    HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** result) override;
    HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** result) override;
    HRESULT STDMETHODCALLTYPE Invoke() override;
    HRESULT STDMETHODCALLTYPE SetValue(LPCWSTR value) override;
    HRESULT STDMETHODCALLTYPE get_Value(BSTR* result) override;
    HRESULT STDMETHODCALLTYPE get_IsReadOnly(BOOL* result) override;
    HRESULT STDMETHODCALLTYPE SetValue(double value) override;
    HRESULT STDMETHODCALLTYPE get_Value(double* result) override;
    HRESULT STDMETHODCALLTYPE get_Maximum(double* result) override;
    HRESULT STDMETHODCALLTYPE get_Minimum(double* result) override;
    HRESULT STDMETHODCALLTYPE get_LargeChange(double* result) override;
    HRESULT STDMETHODCALLTYPE get_SmallChange(double* result) override;
    HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY** result) override;
    HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(BOOL* result) override;
    HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(BOOL* result) override;
    HRESULT STDMETHODCALLTYPE Select() override;
    HRESULT STDMETHODCALLTYPE AddToSelection() override;
    HRESULT STDMETHODCALLTYPE RemoveFromSelection() override;
    HRESULT STDMETHODCALLTYPE get_IsSelected(BOOL* result) override;
    HRESULT STDMETHODCALLTYPE get_SelectionContainer(IRawElementProviderSimple** result) override;
    HRESULT STDMETHODCALLTYPE Toggle() override;
    HRESULT STDMETHODCALLTYPE get_ToggleState(ToggleState* result) override;
    HRESULT STDMETHODCALLTYPE GetVisibleRanges(SAFEARRAY** result) override;
    HRESULT STDMETHODCALLTYPE RangeFromChild(IRawElementProviderSimple* child, ITextRangeProvider** result) override;
    HRESULT STDMETHODCALLTYPE RangeFromPoint(UiaPoint point, ITextRangeProvider** result) override;
    HRESULT STDMETHODCALLTYPE get_DocumentRange(ITextRangeProvider** result) override;
    HRESULT STDMETHODCALLTYPE get_SupportedTextSelection(SupportedTextSelection* result) override;
private:
    std::atomic<ULONG> references_{1};
};

class uia_text_range final : public ITextRangeProvider
{
public:
    uia_text_range(uia_provider& owner, std::size_t start, std::size_t end);
    ~uia_text_range();
    uia_provider& owner;
    std::size_t start, end;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE Clone(ITextRangeProvider** result) override;
    HRESULT STDMETHODCALLTYPE Compare(ITextRangeProvider* other, BOOL* result) override;
    HRESULT STDMETHODCALLTYPE CompareEndpoints(TextPatternRangeEndpoint endpoint, ITextRangeProvider* other,
        TextPatternRangeEndpoint other_endpoint, int* result) override;
    HRESULT STDMETHODCALLTYPE ExpandToEnclosingUnit(TextUnit unit) override;
    HRESULT STDMETHODCALLTYPE FindAttribute(TEXTATTRIBUTEID attribute, VARIANT value, BOOL backward, ITextRangeProvider** result) override;
    HRESULT STDMETHODCALLTYPE FindText(BSTR text, BOOL backward, BOOL ignore_case, ITextRangeProvider** result) override;
    HRESULT STDMETHODCALLTYPE GetAttributeValue(TEXTATTRIBUTEID attribute, VARIANT* result) override;
    HRESULT STDMETHODCALLTYPE GetBoundingRectangles(SAFEARRAY** result) override;
    HRESULT STDMETHODCALLTYPE GetEnclosingElement(IRawElementProviderSimple** result) override;
    HRESULT STDMETHODCALLTYPE GetText(int maximum, BSTR* result) override;
    HRESULT STDMETHODCALLTYPE Move(TextUnit unit, int count, int* moved) override;
    HRESULT STDMETHODCALLTYPE MoveEndpointByUnit(TextPatternRangeEndpoint endpoint, TextUnit unit, int count, int* moved) override;
    HRESULT STDMETHODCALLTYPE MoveEndpointByRange(TextPatternRangeEndpoint endpoint, ITextRangeProvider* other,
        TextPatternRangeEndpoint other_endpoint) override;
    HRESULT STDMETHODCALLTYPE Select() override;
    HRESULT STDMETHODCALLTYPE AddToSelection() override;
    HRESULT STDMETHODCALLTYPE RemoveFromSelection() override;
    HRESULT STDMETHODCALLTYPE ScrollIntoView(BOOL align_top) override;
    HRESULT STDMETHODCALLTYPE GetChildren(SAFEARRAY** result) override;
private:
    std::atomic<ULONG> references_{1};
};
}
