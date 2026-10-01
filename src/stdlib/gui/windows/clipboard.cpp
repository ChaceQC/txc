#include "stdlib/gui/windows/dialogs.hpp"
#include <cstring>

namespace tx_generated::gui
{
namespace
{

struct clipboard_guard
{
    explicit clipboard_guard(HWND owner)
    {
        if (!OpenClipboard(owner))
        {
            graphics::record_native_error(nullptr, GetLastError());
            fail("clipboard_busy", "剪贴板正被占用");
        }
    }
    ~clipboard_guard()
    {
        CloseClipboard();
    }
};

struct global_buffer
{
    HGLOBAL handle = nullptr;
    void* locked = nullptr;
    ~global_buffer()
    {
        if (locked)
        {
            GlobalUnlock(handle);
        }
        if (handle)
        {
            GlobalFree(handle);
        }
    }
};

} // namespace

std::optional<std::string> clipboard_text(graphics::window& owner)
{
    require_system_idle(owner);
    clipboard_guard clipboard(owner.hwnd);
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
        return {};
    }
    const auto handle = GetClipboardData(CF_UNICODETEXT);
    if (!handle)
    {
        platform_fail(owner.owner.lock().get(), "读取剪贴板", GetLastError());
    }
    const auto bytes = GlobalSize(handle);
    if (bytes > 2 * 1024 * 1024 || bytes < sizeof(wchar_t))
    {
        fail("resource_limit", "剪贴板文本为空缓冲或超过 2 MiB");
    }
    const auto* data = static_cast<const wchar_t*>(GlobalLock(handle));
    if (!data)
    {
        platform_fail(owner.owner.lock().get(), "锁定剪贴板", GetLastError());
    }
    try
    {
        std::size_t count = 0;
        while (count < bytes / sizeof(wchar_t) && data[count])
        {
            ++count;
        }
        if (count == bytes / sizeof(wchar_t))
        {
            fail("invalid_argument", "剪贴板文本缺少终止符");
        }
        auto result = graphics::utf8(std::wstring(data, count));
        GlobalUnlock(handle);
        return result;
    }
    catch (...)
    {
        GlobalUnlock(handle);
        throw;
    }
}

void set_clipboard_text(graphics::window& owner, const std::string& text)
{
    require_system_idle(owner);
    const auto wide = graphics::utf16(text);
    global_buffer buffer;
    const auto bytes = (wide.size() + 1) * sizeof(wchar_t);
    buffer.handle = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!buffer.handle || !(buffer.locked = GlobalLock(buffer.handle)))
    {
        platform_fail(owner.owner.lock().get(), "分配剪贴板文本", GetLastError());
    }
    std::memcpy(buffer.locked, wide.c_str(), bytes);
    GlobalUnlock(buffer.handle);
    buffer.locked = nullptr;
    clipboard_guard clipboard(owner.hwnd);
    if (!EmptyClipboard() || !SetClipboardData(CF_UNICODETEXT, buffer.handle))
    {
        platform_fail(owner.owner.lock().get(), "写入剪贴板文本", GetLastError());
    }
    buffer.handle = nullptr;
}

} // namespace tx_generated::gui
