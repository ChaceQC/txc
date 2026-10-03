#include "stdlib/native_gui/platform/windows_accessibility.hpp"

#include <algorithm>
#include <cwctype>

namespace tx_generated::native_gui
{
// Windows SDK 的 TextAttributeId；旧版 MinGW 未声明该常量。
constexpr TEXTATTRIBUTEID read_only_attribute = 40015;
uia_text_range::uia_text_range(uia_provider& value, std::size_t begin, std::size_t finish)
    : owner(value), start(begin), end(finish)
{
    owner.AddRef();
}

uia_text_range::~uia_text_range()
{
    owner.Release();
}

HRESULT STDMETHODCALLTYPE uia_text_range::QueryInterface(REFIID iid, void** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    if (iid != __uuidof(IUnknown) && iid != __uuidof(ITextRangeProvider))
    {
        return E_NOINTERFACE;
    }
    *result = static_cast<ITextRangeProvider*>(this);
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE uia_text_range::AddRef()
{
    return ++references_;
}

ULONG STDMETHODCALLTYPE uia_text_range::Release()
{
    const auto count = --references_;
    if (!count)
    {
        delete this;
    }
    return count;
}

HRESULT STDMETHODCALLTYPE uia_text_range::Clone(ITextRangeProvider** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        *result = new uia_text_range(owner, start, end);
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_text_range::Compare(ITextRangeProvider* other, BOOL* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    const auto value = dynamic_cast<uia_text_range*>(other);
    *result = value && value->owner.endpoint == owner.endpoint && value->owner.id == owner.id && value->start == start && value->end == end;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE uia_text_range::CompareEndpoints(TextPatternRangeEndpoint endpoint, ITextRangeProvider* other,
    TextPatternRangeEndpoint other_endpoint, int* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    const auto value = dynamic_cast<uia_text_range*>(other);
    if (!value || value->owner.endpoint != owner.endpoint || value->owner.id != owner.id)
    {
        return E_INVALIDARG;
    }
    *result = static_cast<int>(endpoint == TextPatternRangeEndpoint_Start ? start : end) -
        static_cast<int>(other_endpoint == TextPatternRangeEndpoint_Start ? value->start : value->end);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE uia_text_range::GetText(int maximum, BSTR* result)
{
    if (!result || maximum < -1)
    {
        return E_INVALIDARG;
    }
    *result = nullptr;
    return uia_call([&]() -> HRESULT
    {
        const auto state = owner.read();
        if (!state.text_capable || state.password)
        {
            return UIA_E_NOTSUPPORTED;
        }
        const auto begin = std::min(start, state.text.size()), finish = std::min(std::max(start, end), state.text.size());
        auto text = tx::ui::utf16_text(std::u32string_view(state.text).substr(begin, finish - begin));
        if (maximum >= 0 && static_cast<std::size_t>(maximum) < text.size())
        {
            auto length = static_cast<std::size_t>(maximum);
            if (length && text[length - 1] >= 0xd800 && text[length - 1] <= 0xdbff)
            {
                --length;
            }
            text.resize(length);
        }
        *result = SysAllocStringLen(text.data(), static_cast<UINT>(text.size()));
        return *result ? S_OK : E_OUTOFMEMORY;
    });
}

HRESULT STDMETHODCALLTYPE uia_text_range::GetBoundingRectangles(SAFEARRAY** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        std::vector<double> values;
        for (const auto& bounds : owner.endpoint->text_bounds(owner.id, start, end))
        {
            if (bounds.width > 0 && bounds.height > 0)
            {
                values.insert(values.end(), {bounds.x, bounds.y, bounds.width, bounds.height});
            }
        }
        *result = uia_numbers(values);
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_text_range::GetEnclosingElement(IRawElementProviderSimple** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = &owner;
    owner.AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE uia_text_range::GetChildren(SAFEARRAY** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        *result = uia_objects({});
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_text_range::Select()
{
    accessible_action action{owner.id, "text_selection"};
    action.start = start;
    action.end = end;
    return owner.act(action);
}

HRESULT STDMETHODCALLTYPE uia_text_range::AddToSelection()
{
    return Select();
}

HRESULT STDMETHODCALLTYPE uia_text_range::RemoveFromSelection()
{
    accessible_action action{owner.id, "text_selection"};
    action.start = action.end = end;
    return owner.act(action);
}

HRESULT STDMETHODCALLTYPE uia_text_range::ScrollIntoView(BOOL align_top)
{
    accessible_action action{owner.id, "scroll_text"};
    action.start = align_top ? start : end;
    return owner.act(action);
}

HRESULT STDMETHODCALLTYPE uia_text_range::FindAttribute(TEXTATTRIBUTEID attribute, VARIANT value, BOOL, ITextRangeProvider** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    if (attribute != read_only_attribute || value.vt != VT_BOOL)
    {
        return S_OK;
    }
    return uia_call([&]() -> HRESULT
    {
        if ((!owner.read().editable) == (value.boolVal != VARIANT_FALSE))
        {
            *result = new uia_text_range(owner, start, end);
        }
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_text_range::GetAttributeValue(TEXTATTRIBUTEID attribute, VARIANT* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    VariantInit(result);
    return uia_call([&]() -> HRESULT
    {
        if (attribute == read_only_attribute)
        {
            result->vt = VT_BOOL;
            result->boolVal = owner.read().editable ? VARIANT_FALSE : VARIANT_TRUE;
        }
        else
        {
            result->vt = VT_UNKNOWN;
            return UiaGetReservedNotSupportedValue(&result->punkVal);
        }
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_text_range::FindText(BSTR query, BOOL backward, BOOL ignore_case, ITextRangeProvider** result)
{
    if (!result || !query)
    {
        return E_INVALIDARG;
    }
    *result = nullptr;
    return uia_call([&]() -> HRESULT
    {
        auto state = owner.read();
        auto needle = tx::ui::utf32_text(std::wstring_view(query, SysStringLen(query)));
        if (ignore_case)
        {
            for (auto& scalar : state.text)
            {
                scalar = std::towlower(scalar);
            }
            for (auto& scalar : needle)
            {
                scalar = std::towlower(scalar);
            }
        }
        const auto begin = std::min(start, state.text.size()), finish = std::min(end, state.text.size());
        const auto haystack = state.text.substr(begin, finish - begin);
        const auto found = backward ? haystack.rfind(needle) : haystack.find(needle);
        if (found != std::u32string::npos)
        {
            *result = new uia_text_range(owner, begin + found, begin + found + needle.size());
        }
        return S_OK;
    });
}
}
