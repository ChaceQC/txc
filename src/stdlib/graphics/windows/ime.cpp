#include "stdlib/graphics/windows/assets.hpp"
#include <imm.h>
#include <algorithm>

namespace tx_generated::graphics
{
namespace
{

struct input_context
{
    explicit input_context(HWND window) : hwnd(window), value(ImmGetContext(window))
    {
    }
    ~input_context()
    {
        if (value)
        {
            ImmReleaseContext(hwnd, value);
        }
    }
    HWND hwnd;
    HIMC value;
};

std::wstring composition_text(HIMC context, DWORD index)
{
    const auto bytes = ImmGetCompositionStringW(context, index, nullptr, 0);
    if (bytes < 0 || bytes > 1024 * 1024 || bytes % sizeof(wchar_t))
    {
        fail("invalid_argument", "输入法组合文本非法或超限");
    }
    std::wstring text(static_cast<std::size_t>(bytes) / sizeof(wchar_t), 0);
    if (bytes && ImmGetCompositionStringW(context, index, text.data(), bytes) != bytes)
    {
        fail("platform_error", "读取输入法组合文本失败");
    }
    return text;
}

} // namespace

void position_ime(window& state)
{
    input_context context(state.hwnd);
    if (!context.value)
    {
        return;
    }
    COMPOSITIONFORM composition{};
    composition.dwStyle = CFS_POINT;
    composition.ptCurrentPos = {MulDiv(state.ime_rect.left, state.dpi, 96),
        MulDiv(state.ime_rect.bottom, state.dpi, 96)};
    ImmSetCompositionWindow(context.value, &composition);
    CANDIDATEFORM candidate{};
    candidate.dwStyle = CFS_EXCLUDE;
    candidate.ptCurrentPos = composition.ptCurrentPos;
    candidate.rcArea = {MulDiv(state.ime_rect.left, state.dpi, 96),
        MulDiv(state.ime_rect.top, state.dpi, 96), MulDiv(state.ime_rect.right, state.dpi, 96),
        MulDiv(state.ime_rect.bottom, state.dpi, 96)};
    ImmSetCandidateWindow(context.value, &candidate);
}

bool ime_message(window& state, UINT message, WPARAM, LPARAM lparam, LRESULT& result)
{
    if (!state.text_input)
    {
        return false;
    }
    result = 0;
    if (message == WM_IME_STARTCOMPOSITION)
    {
        state.composing = true;
        state.pending_surrogate = 0;
        position_ime(state);
        return true;
    }
    if (message == WM_IME_COMPOSITION)
    {
        input_context context(state.hwnd);
        if (!context.value)
        {
            return true;
        }
        if (lparam & GCS_RESULTSTR)
        {
            event item;
            item.kind = "text_input";
            item.text = event::text_data{utf8(composition_text(context.value, GCS_RESULTSTR)), 0, 0};
            enqueue_event(state, std::move(item));
        }
        if (lparam & (GCS_COMPSTR | GCS_CURSORPOS))
        {
            const auto text = composition_text(context.value, GCS_COMPSTR);
            const auto offsets = unicode_offsets(text);
            const auto cursor = std::max<LONG>(0, ImmGetCompositionStringW(context.value, GCS_CURSORPOS, nullptr, 0));
            const auto index = std::min<std::size_t>(offsets.size() - 1,
                std::lower_bound(offsets.begin(), offsets.end(), static_cast<UINT32>(cursor)) - offsets.begin());
            event item;
            item.kind = "composition";
            item.text = event::text_data{utf8(text), static_cast<std::int64_t>(index), 0};
            enqueue_event(state, std::move(item));
        }
        // 不交给 DefWindowProc，避免 RESULTSTR 再转换为 WM_IME_CHAR/WM_CHAR。
        return true;
    }
    if (message == WM_IME_ENDCOMPOSITION)
    {
        state.composing = false;
        event item;
        item.kind = "composition";
        item.text = event::text_data{};
        enqueue_event(state, std::move(item));
        return true;
    }
    return message == WM_IME_CHAR;
}

} // namespace tx_generated::graphics
