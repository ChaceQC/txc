#include "stdlib/native_gui/platform/windows_window.hpp"

#include <imm.h>
#include <windowsx.h>
#include <stdexcept>

namespace tx::ui
{
std::wstring utf16_text(std::u32string_view text)
{
    std::wstring result;
    for (const auto scalar : text)
    {
        if (scalar <= 0xffff && !(scalar >= 0xd800 && scalar <= 0xdfff))
        {
            result.push_back(static_cast<wchar_t>(scalar));
        }
        else if (scalar >= 0x10000 && scalar <= 0x10ffff)
        {
            result.push_back(static_cast<wchar_t>(0xd800 + ((scalar - 0x10000) >> 10)));
            result.push_back(static_cast<wchar_t>(0xdc00 + ((scalar - 0x10000) & 0x3ff)));
        }
        else
        {
            throw std::invalid_argument("窗口文本包含非法 Unicode 标量");
        }
    }
    return result;
}

std::u32string utf32_text(std::wstring_view text)
{
    std::u32string result;
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        const auto first = static_cast<unsigned>(text[index]);
        if (first >= 0xd800 && first <= 0xdbff)
        {
            if (++index == text.size() || text[index] < 0xdc00 || text[index] > 0xdfff)
            {
                throw std::invalid_argument("系统 UTF-16 文本包含未配对代理项");
            }
            result.push_back(0x10000 + ((first - 0xd800) << 10) + text[index] - 0xdc00);
        }
        else if (first >= 0xdc00 && first <= 0xdfff)
        {
            throw std::invalid_argument("系统 UTF-16 文本包含未配对代理项");
        }
        else
        {
            result.push_back(first);
        }
    }
    return result;
}

namespace
{
std::string key_name(WPARAM value)
{
    if (value >= VK_F1 && value <= VK_F12)
    {
        return "f" + std::to_string(value - VK_F1 + 1);
    }
    if (value >= 'A' && value <= 'Z')
    {
        return std::string(1, static_cast<char>(value - 'A' + 'a'));
    }
    if (value >= '0' && value <= '9')
    {
        return std::string(1, static_cast<char>(value));
    }
    switch (value)
    {
    case VK_LEFT: return "left";
    case VK_RIGHT: return "right";
    case VK_UP: return "up";
    case VK_DOWN: return "down";
    case VK_HOME: return "home";
    case VK_END: return "end";
    case VK_BACK: return "backspace";
    case VK_DELETE: return "delete";
    case VK_RETURN: return "enter";
    case VK_TAB: return "tab";
    case VK_ESCAPE: return "escape";
    case VK_SPACE: return "space";
    case VK_PRIOR: return "page_up";
    case VK_NEXT: return "page_down";
    case VK_F4: return "f4";
    default: return "unknown";
    }
}

void modifiers(window_event& event)
{
    event.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    event.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    event.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    event.meta = ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) != 0;
}

struct ime_context
{
    explicit ime_context(HWND window) : hwnd(window), handle(ImmGetContext(window))
    {
    }
    ~ime_context()
    {
        if (handle)
        {
            ImmReleaseContext(hwnd, handle);
        }
    }
    HWND hwnd;
    HIMC handle;
};

std::u32string composition_text(HIMC context, DWORD mode)
{
    const auto bytes = ImmGetCompositionStringW(context, mode, nullptr, 0);
    if (bytes < 0 || bytes > 1024 * 1024 || bytes % 2)
    {
        throw std::runtime_error("输入法组合文本长度非法");
    }
    std::wstring value(static_cast<std::size_t>(bytes) / 2, 0);
    if (bytes && ImmGetCompositionStringW(context, mode, value.data(), bytes) != bytes)
    {
        throw std::runtime_error("无法读取输入法组合文本");
    }
    return utf32_text(value);
}
}

void windows_window::set_ime_rect(rect bounds)
{
    ime_ = bounds;
    ime_context context(hwnd);
    if (!context.handle)
    {
        return;
    }
    COMPOSITIONFORM composition{};
    composition.dwStyle = CFS_POINT;
    composition.ptCurrentPos = {static_cast<LONG>(bounds.x * scale_),
        static_cast<LONG>((bounds.y + bounds.height) * scale_)};
    ImmSetCompositionWindow(context.handle, &composition);
    CANDIDATEFORM candidate{};
    candidate.dwStyle = CFS_EXCLUDE;
    candidate.ptCurrentPos = composition.ptCurrentPos;
    candidate.rcArea = {static_cast<LONG>(bounds.x * scale_), static_cast<LONG>(bounds.y * scale_),
        static_cast<LONG>((bounds.x + bounds.width) * scale_), static_cast<LONG>((bounds.y + bounds.height) * scale_)};
    ImmSetCandidateWindow(context.handle, &candidate);
}

bool windows_window::ime(UINT message, LPARAM lparam)
{
    if (!ime_enabled_)
    {
        return false;
    }
    if (message == WM_IME_STARTCOMPOSITION || message == WM_IME_ENDCOMPOSITION)
    {
        composing_ = message == WM_IME_STARTCOMPOSITION;
        surrogate_ = 0;
        window_event event{event_kind::composition};
        event.composing = composing_;
        enqueue(std::move(event));
        set_ime_rect(ime_);
        return true;
    }
    if (message == WM_IME_COMPOSITION)
    {
        ime_context context(hwnd);
        if (context.handle)
        {
            if (lparam & GCS_RESULTSTR)
            {
                window_event event{event_kind::text_input};
                event.text = composition_text(context.handle, GCS_RESULTSTR);
                enqueue(std::move(event));
            }
            if (lparam & (GCS_COMPSTR | GCS_CURSORPOS))
            {
                window_event event{event_kind::composition};
                event.text = composition_text(context.handle, GCS_COMPSTR);
                const auto cursor = std::max<LONG>(0, ImmGetCompositionStringW(context.handle, GCS_CURSORPOS, nullptr, 0));
                std::size_t units = 0;
                for (const auto scalar : event.text)
                {
                    const auto count = scalar > 0xffff ? 2 : 1;
                    if (units + count > static_cast<std::size_t>(cursor))
                    {
                        break;
                    }
                    units += count;
                    ++event.caret;
                }
                event.composing = true;
                enqueue(std::move(event));
            }
        }
        return true;
    }
    return message == WM_IME_CHAR;
}

void windows_window::cancel_composition()
{
    ime_context context(hwnd);
    if (context.handle && composing_)
    {
        ImmNotifyIME(context.handle, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    }
    composing_ = false;
}

void windows_window::enable_ime(bool enabled)
{
    if (!enabled)
    {
        cancel_composition();
    }
    ime_enabled_ = enabled;
    ImmAssociateContextEx(hwnd, nullptr, enabled ? IACE_DEFAULT : 0);
}

bool windows_window::input(UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result)
{
    result = 0;
    if (message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ||
        message == WM_RBUTTONDOWN || message == WM_RBUTTONUP || message == WM_MOUSEWHEEL)
    {
        if (message == WM_MOUSEMOVE)
        {
            TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tracking);
        }
        window_event event{message == WM_MOUSEMOVE ? event_kind::pointer_moved :
            message == WM_MOUSEWHEEL ? event_kind::wheel :
            message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ? event_kind::pointer_down : event_kind::pointer_up};
        POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        if (message == WM_MOUSEWHEEL)
        {
            ScreenToClient(hwnd, &point);
            event.wheel = GET_WHEEL_DELTA_WPARAM(wparam) / 120.0;
        }
        event.x = point.x / scale_;
        event.y = point.y / scale_;
        event.button = message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ? 1 :
            message == WM_RBUTTONDOWN || message == WM_RBUTTONUP ? 2 : 0;
        modifiers(event);
        enqueue(std::move(event));
        return true;
    }
    if (message == WM_KEYDOWN || message == WM_KEYUP || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)
    {
        const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
        window_event event{down ? event_kind::key_down : event_kind::key_up};
        event.key = key_name(wparam);
        event.repeat = down && (lparam & (1ll << 30));
        event.composing = composing_;
        modifiers(event);
        enqueue(std::move(event));
        return false;
    }
    if (message == WM_CHAR)
    {
        const auto character = static_cast<wchar_t>(wparam);
        if (character >= 0xd800 && character <= 0xdbff)
        {
            surrogate_ = character;
        }
        else if (character >= 32 && character != 127)
        {
            std::wstring text;
            if (surrogate_ && character >= 0xdc00 && character <= 0xdfff)
            {
                text.push_back(surrogate_);
            }
            surrogate_ = 0;
            text.push_back(character);
            window_event event{event_kind::text_input};
            event.text = utf32_text(text);
            enqueue(std::move(event));
        }
        return true;
    }
    if (message == WM_SETFOCUS || message == WM_KILLFOCUS)
    {
        enqueue({message == WM_SETFOCUS ? event_kind::focus_gained : event_kind::focus_lost});
        if (message == WM_KILLFOCUS)
        {
            surrogate_ = 0;
            composing_ = false;
        }
    }
    if (message == WM_MOUSELEAVE)
    {
        enqueue({event_kind::pointer_left});
    }
    if ((message == WM_CAPTURECHANGED && !releasing_capture_) || message == WM_CANCELMODE)
    {
        enqueue({event_kind::capture_lost});
    }
    return ime(message, lparam);
}
}
