#include "stdlib/native_gui/platform/windows_accessibility.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
HRESULT STDMETHODCALLTYPE uia_provider::Navigate(NavigateDirection direction, IRawElementProviderFragment** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    return uia_call([&]() -> HRESULT
    {
        const auto tree = endpoint->tree();
        const auto& value = tree.at(id);
        std::string target;
        if (direction == NavigateDirection_Parent)
        {
            target = value.parent;
        }
        else if ((direction == NavigateDirection_FirstChild || direction == NavigateDirection_LastChild) && !value.children.empty())
        {
            target = direction == NavigateDirection_FirstChild ? value.children.front() : value.children.back();
        }
        else if (!value.parent.empty() && (direction == NavigateDirection_NextSibling || direction == NavigateDirection_PreviousSibling))
        {
            const auto& siblings = tree.at(value.parent).children;
            const auto found = std::find(siblings.begin(), siblings.end(), id);
            if (found != siblings.end())
            {
                const auto index = found - siblings.begin() + (direction == NavigateDirection_NextSibling ? 1 : -1);
                if (index >= 0 && index < static_cast<std::ptrdiff_t>(siblings.size()))
                {
                    target = siblings[index];
                }
            }
        }
        if (!target.empty())
        {
            *result = new uia_provider(endpoint, target, hwnd);
        }
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::GetRuntimeId(SAFEARRAY** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = SafeArrayCreateVector(VT_I4, 0, static_cast<ULONG>(id.size() + 1));
    if (!*result)
    {
        return E_OUTOFMEMORY;
    }
    LONG index = 0, value = UiaAppendRuntimeId;
    SafeArrayPutElement(*result, &index, &value);
    for (const unsigned char byte : id)
    {
        ++index;
        value = byte;
        SafeArrayPutElement(*result, &index, &value);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE uia_provider::get_BoundingRectangle(UiaRect* result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        const auto value = read().bounds;
        *result = {value.x, value.y, value.width, value.height};
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::GetEmbeddedFragmentRoots(SAFEARRAY** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE uia_provider::get_FragmentRoot(IRawElementProviderFragmentRoot** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    return uia_call([&]() -> HRESULT
    {
        *result = new uia_provider(endpoint, endpoint->root_id, hwnd);
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    return uia_call([&]() -> HRESULT
    {
        const auto tree = endpoint->tree();
        std::string best;
        double area = std::numeric_limits<double>::max();
        for (const auto& [key, value] : tree)
        {
            const double candidate = value.bounds.width * value.bounds.height;
            if (value.visible && value.bounds.contains({x, y}) && candidate <= area)
            {
                best = key;
                area = candidate;
            }
        }
        if (!best.empty())
        {
            *result = new uia_provider(endpoint, best, hwnd);
        }
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::GetFocus(IRawElementProviderFragment** result)
{
    if (!result)
    {
        return E_POINTER;
    }
    *result = nullptr;
    return uia_call([&]() -> HRESULT
    {
        for (const auto& [key, value] : endpoint->tree())
        {
            if (value.focused)
            {
                *result = new uia_provider(endpoint, key, hwnd);
                break;
            }
        }
        return S_OK;
    });
}

HRESULT STDMETHODCALLTYPE uia_provider::SetFocus()
{
    return act({id, "focus"});
}

HRESULT STDMETHODCALLTYPE uia_provider::Invoke()
{
    return act({id, "invoke"});
}

HRESULT STDMETHODCALLTYPE uia_provider::Select()
{
    return act({id, "select"});
}

HRESULT STDMETHODCALLTYPE uia_provider::AddToSelection()
{
    return act({id, "add_selection"});
}

HRESULT STDMETHODCALLTYPE uia_provider::RemoveFromSelection()
{
    return act({id, "remove_selection"});
}

HRESULT STDMETHODCALLTYPE uia_provider::Toggle()
{
    return act({id, "invoke"});
}

}
