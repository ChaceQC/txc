#include "stdlib/native_gui/platform/windows_window.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace tx::ui
{
namespace
{
class clipboard_guard
{
public:
    explicit clipboard_guard(HWND owner)
    {
        if (!OpenClipboard(owner))
        {
            throw std::runtime_error("剪贴板暂时被其他程序占用");
        }
    }
    ~clipboard_guard()
    {
        CloseClipboard();
    }
};

struct global_memory
{
    HGLOBAL value = nullptr;
    ~global_memory()
    {
        if (value)
        {
            GlobalFree(value);
        }
    }
};
}

std::u32string windows_window::clipboard_text()
{
    clipboard_guard guard(hwnd);
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
        return {};
    }
    const auto value = GetClipboardData(CF_UNICODETEXT);
    const auto bytes = GlobalSize(value);
    if (!value || bytes > 16 * 1024 * 1024 || bytes % sizeof(wchar_t))
    {
        throw std::runtime_error("剪贴板文本大小非法");
    }
    const auto* data = static_cast<const wchar_t*>(GlobalLock(value));
    if (!data)
    {
        throw std::runtime_error("无法读取剪贴板文本");
    }
    std::wstring text;
    try
    {
        const auto count = bytes / sizeof(wchar_t);
        const auto end = std::find(data, data + count, wchar_t(0));
        if (end == data + count)
        {
            throw std::runtime_error("剪贴板文本缺少终止符");
        }
        text.assign(data, end);
    }
    catch (...)
    {
        GlobalUnlock(value);
        throw;
    }
    GlobalUnlock(value);
    return utf32_text(text);
}

bool windows_window::owns_clipboard()
{
    return GetClipboardOwner() == hwnd;
}

void windows_window::set_clipboard_text(std::u32string_view text)
{
    if (text.size() > 4 * 1024 * 1024 || text.find(U'\0') != std::u32string_view::npos)
    {
        throw std::invalid_argument("剪贴板文本过长或包含 NUL");
    }
    const auto wide = utf16_text(text);
    const auto bytes = (wide.size() + 1) * sizeof(wchar_t);
    global_memory memory{GlobalAlloc(GMEM_MOVEABLE, bytes)};
    if (!memory.value)
    {
        throw std::bad_alloc();
    }
    const auto pointer = GlobalLock(memory.value);
    if (!pointer)
    {
        throw std::runtime_error("无法分配剪贴板文本");
    }
    std::memcpy(pointer, wide.c_str(), bytes);
    GlobalUnlock(memory.value);
    clipboard_guard guard(hwnd);
    if (!EmptyClipboard() || !SetClipboardData(CF_UNICODETEXT, memory.value))
    {
        throw std::runtime_error("写入剪贴板失败");
    }
    memory.value = nullptr;
}
}
