#include "stdlib/native_gui/platform/windows_accessibility.hpp"

namespace tx_generated::native_gui
{
HRESULT STDMETHODCALLTYPE uia_provider::SetValue(LPCWSTR value)
{
    if (!value)
    {
        return E_INVALIDARG;
    }
    return uia_call([&]() -> HRESULT
    {
        accessible_action action{id, "value"};
        action.text = tx::ui::utf32_text(value);
        return act(action);
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::SetValue(double value)
{
    accessible_action action{id, "range"};
    action.value = value;
    return act(action);
}

HRESULT STDMETHODCALLTYPE uia_provider::get_Value(BSTR* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto text = tx::ui::utf16_text(read().text);
        *result = SysAllocStringLen(text.data(), static_cast<UINT>(text.size()));
        return *result ? S_OK : E_OUTOFMEMORY;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_IsReadOnly(BOOL* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = !value.editable;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_Value(double* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = value.value;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_Maximum(double* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = value.maximum;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_Minimum(double* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = value.minimum;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_LargeChange(double* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = value.step * 10;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_SmallChange(double* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = value.step;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_CanSelectMultiple(BOOL* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = value.multiple;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_IsSelectionRequired(BOOL* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = false;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_IsSelected(BOOL* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = value.selected;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_ToggleState(ToggleState* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = value.checked ? ToggleState_On : ToggleState_Off;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_SupportedTextSelection(SupportedTextSelection* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        *result = SupportedTextSelection_Single;
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_SelectionContainer(IRawElementProviderSimple** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        if (!value.parent.empty())
        {
            *result = new uia_provider(endpoint, value.parent, hwnd);
        }
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::GetSelection(SAFEARRAY** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto tree = endpoint->tree();
        const auto& value = tree.at(id);
        std::vector<IUnknown*> selected;
        if (value.text_capable)
        {
            selected.push_back(new uia_text_range(*this, value.selection_start, value.selection_end));
        }
        else
        {
            for (const auto& key : value.children)
            {
                if (tree.at(key).selected)
                {
                    selected.push_back(static_cast<IRawElementProviderSimple*>(new uia_provider(endpoint, key, hwnd)));
                }
            }
        }
        *result = uia_objects(selected);
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::get_DocumentRange(ITextRangeProvider** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    return uia_call([&]() -> HRESULT
    {
        const auto value = read();
        if (!value.text_capable)
        {
            return UIA_E_NOTSUPPORTED;
        }
        *result = new uia_text_range(*this, 0, value.text.size());
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::GetVisibleRanges(SAFEARRAY** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        std::vector<IUnknown*> ranges;
        for (const auto& [begin, end] : endpoint->visible_text_ranges(id))
        {
            ranges.push_back(new uia_text_range(*this, begin, end));
        }
        *result = uia_objects(ranges);
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::RangeFromChild(IRawElementProviderSimple*, ITextRangeProvider** result)
{
    if (result)
    {
        *result = nullptr;
    }
    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE uia_provider::RangeFromPoint(UiaPoint point, ITextRangeProvider** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto best = endpoint->text_at_point(id, {point.x, point.y});
        *result = new uia_text_range(*this, best, best);
        return S_OK;
    });
}
}
