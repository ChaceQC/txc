#include "stdlib/gui/windows/state.hpp"

#include <algorithm>

namespace tx_generated::gui
{

std::wstring wide_text(const std::string& text)
{
    if (text.size() > 65536 || text.find('\0') != std::string::npos)
    {
        fail("invalid_argument", "GUI 文本不能包含 NUL 或超过 65536 UTF-8 字节");
    }
    if (text.empty())
    {
        return {};
    }
    const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (!size)
    {
        fail("invalid_argument", "GUI 文本必须为有效 UTF-8");
    }
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
        static_cast<int>(text.size()), result.data(), size);
    return result;
}

std::vector<DWORD> scalar_offsets(const std::wstring& text)
{
    std::vector<DWORD> offsets;
    for (DWORD index = 0; index < text.size(); ++index)
    {
        offsets.push_back(index);
        const auto unit = static_cast<unsigned>(text[index]);
        if (unit >= 0xd800 && unit <= 0xdbff)
        {
            if (++index >= text.size() || text[index] < 0xdc00 || text[index] > 0xdfff)
            {
                fail("invalid_argument", "GUI 文本含无效 UTF-16 代理对");
            }
        }
        else if (unit >= 0xdc00 && unit <= 0xdfff)
        {
            fail("invalid_argument", "GUI 文本含孤立 UTF-16 代理项");
        }
    }
    offsets.push_back(static_cast<DWORD>(text.size()));
    return offsets;
}

std::string read_text(node& state)
{
    const auto count = GetWindowTextLengthW(state.hwnd);
    std::wstring text(static_cast<std::size_t>(count) + 1, L'\0');
    const auto copied = GetWindowTextW(state.hwnd, text.data(), count + 1);
    text.resize(copied);
    if (text.empty())
    {
        return {};
    }
    const auto bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        text.data(), copied, nullptr, 0, nullptr, nullptr);
    if (!bytes)
    {
        fail("invalid_argument", "输入文本不是有效的 Unicode 标量序列");
    }
    std::string result(bytes, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
        copied, result.data(), bytes, nullptr, nullptr);
    return result;
}

void set_text(node& state, const std::string& text)
{
    const auto wide = wide_text(text);
    if (state.kind == tx::graphics_kind::text_box &&
        scalar_offsets(wide).size() - 1 > static_cast<std::size_t>(state.text_limit))
    {
        fail("invalid_argument", "文本超过输入框 Unicode 标量长度上限");
    }
    if (state.text == text)
    {
        return;
    }
    auto replacement = text;
    ++state.suppress;
    const auto success = SetWindowTextW(state.hwnd, wide.c_str());
    --state.suppress;
    if (!success)
    {
        platform_fail(owner_window(state).owner.lock().get(), "设置控件文本", GetLastError());
    }
    state.text.swap(replacement);
    ++state.revision;
    dirty(state);
}

void set_selection(node& state, std::int64_t start, std::int64_t end)
{
    const auto offsets = scalar_offsets(wide_text(state.text));
    if (start < 0 || end < start || static_cast<std::uint64_t>(end) >= offsets.size())
    {
        fail("invalid_argument", "选择范围必须为文本内的 Unicode 标量半开区间");
    }
    SendMessageW(state.hwnd, EM_SETSEL, offsets[start], offsets[end]);
}

std::int64_t selection(node& state, bool end)
{
    DWORD first = 0;
    DWORD last = 0;
    SendMessageW(state.hwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&first),
        reinterpret_cast<LPARAM>(&last));
    const auto offsets = scalar_offsets(wide_text(state.text));
    const auto position = end ? last : first;
    const auto found = std::lower_bound(offsets.begin(), offsets.end(), position);
    if (found == offsets.end() || *found != position)
    {
        fail("invalid_argument", "原生选择位于 Unicode 代理对中间");
    }
    return found - offsets.begin();
}

void set_check(node& state, int value)
{
    if (value < 0 || value > 2 || (!state.three_state && value == 2))
    {
        fail("invalid_argument", "二态复选框不能设置 mixed 状态");
    }
    if (state.check != value)
    {
        SendMessageW(state.hwnd, BM_SETCHECK, value, 0);
        state.check = value;
        ++state.revision;
    }
}

void set_password(node& state, bool value)
{
    if (value && state.multiline)
    {
        fail("invalid_argument", "多行输入框不支持密码模式");
    }
    if (value != state.password)
    {
        SendMessageW(state.hwnd, EM_SETPASSWORDCHAR, value ? 0x25cf : 0, 0);
        state.password = value;
        ++state.revision;
        InvalidateRect(state.hwnd, nullptr, TRUE);
    }
}

} // namespace tx_generated::gui
