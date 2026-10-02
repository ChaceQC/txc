#include "stdlib/gui/windows/state.hpp"

namespace tx_generated::gui
{
namespace
{
constexpr COLORREF background = RGB(242, 245, 249);
constexpr COLORREF surface = RGB(255, 255, 255);
constexpr COLORREF foreground = RGB(35, 45, 65);
constexpr COLORREF muted = RGB(110, 122, 140);
constexpr COLORREF accent = RGB(59, 89, 190);
constexpr COLORREF border = RGB(214, 222, 232);

COLORREF panel_color(node& state)
{
    return state.parent.expired() ? background : surface;
}

HBRUSH brush(HDC dc, COLORREF color)
{
    SetDCBrushColor(dc, color);
    return static_cast<HBRUSH>(GetStockObject(DC_BRUSH));
}

node* control_node(HWND hwnd)
{
    DWORD_PTR reference = 0;
    return GetWindowSubclass(hwnd, control_proc, 1, &reference)
        ? reinterpret_cast<node*>(reference) : nullptr;
}

void draw_button(node& state, DRAWITEMSTRUCT& item)
{
    const bool modern = light_theme_active(state);
    const bool disabled = (item.itemState & ODS_DISABLED) != 0;
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    const bool primary = state.primary_button;
    COLORREF fill = GetSysColor(COLOR_BTNFACE);
    COLORREF ink = GetSysColor(disabled ? COLOR_GRAYTEXT : COLOR_BTNTEXT);
    COLORREF edge = GetSysColor(COLOR_BTNSHADOW);
    if (modern)
    {
        fill = disabled ? background : primary ? accent : surface;
        if (!disabled && (state.hovered || pressed))
        {
            fill = primary ? (pressed ? RGB(38, 62, 145) : RGB(48, 76, 169))
                : (pressed ? RGB(220, 227, 241) : RGB(237, 242, 252));
        }
        ink = disabled ? muted : primary ? surface : foreground;
        edge = primary && !disabled ? fill : border;
    }
    const auto dc = item.hDC;
    const auto saved = SaveDC(dc);
    auto rect = item.rcItem;
    FillRect(dc, &rect, brush(dc, modern ? surface : GetSysColor(COLOR_BTNFACE)));
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SelectObject(dc, GetStockObject(DC_PEN));
    SetDCBrushColor(dc, fill);
    SetDCPenColor(dc, edge);
    const auto radius = MulDiv(12, owner_window(state).dpi, 96);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, root_node(state).font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, ink);
    auto label = wide_text(state.text);
    if (pressed)
    {
        OffsetRect(&rect, 0, 1);
    }
    DrawTextW(dc, label.c_str(), static_cast<int>(label.size()), &rect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    if ((item.itemState & ODS_FOCUS) && !(item.itemState & ODS_NOFOCUSRECT))
    {
        InflateRect(&rect, -4, -4);
        DrawFocusRect(dc, &rect);
    }
    RestoreDC(dc, saved);
}

void apply(node& state, bool modern)
{
    using tx::graphics_kind;
    if (state.kind == graphics_kind::button)
    {
        const auto style = GetWindowLongPtrW(state.hwnd, GWL_STYLE);
        SetWindowLongPtrW(state.hwnd, GWL_STYLE,
            (style & ~BS_TYPEMASK) | (modern ? BS_OWNERDRAW : BS_PUSHBUTTON));
    }
    if (state.kind == graphics_kind::text_box && modern)
    {
        SetWindowLongPtrW(state.hwnd, GWL_EXSTYLE,
            GetWindowLongPtrW(state.hwnd, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
        SetWindowLongPtrW(state.hwnd, GWL_STYLE,
            GetWindowLongPtrW(state.hwnd, GWL_STYLE) | WS_BORDER);
        const auto margin = MulDiv(10, owner_window(state).dpi, 96);
        SendMessageW(state.hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(margin, margin));
        SetWindowPos(state.hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    if (state.kind == graphics_kind::list_view || state.kind == graphics_kind::table_view)
    {
        ListView_SetBkColor(state.hwnd, modern ? surface : GetSysColor(COLOR_WINDOW));
        ListView_SetTextBkColor(state.hwnd, modern ? surface : GetSysColor(COLOR_WINDOW));
        ListView_SetTextColor(state.hwnd, modern ? foreground : GetSysColor(COLOR_WINDOWTEXT));
    }
    for (const auto& child : state.children)
    {
        apply(*child, modern);
    }
}
}

bool light_theme_active(node& state)
{
    if (!root_node(state).light_theme)
    {
        return false;
    }
    HIGHCONTRASTW contrast{};
    contrast.cbSize = sizeof(contrast);
    return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0)
        && !(contrast.dwFlags & HCF_HIGHCONTRASTON);
}

void set_theme(node& state, const std::string& theme)
{
    require_idle(state);
    if (theme != "light" && theme != "system")
    {
        fail("invalid_argument", "GUI 主题仅支持 light 或 system");
    }
    auto& root = root_node(state);
    root.light_theme = theme == "light";
    root.font_dpi = 0;
    apply(root, light_theme_active(root));
    dirty(root);
    RedrawWindow(root.hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void set_button_appearance(node& state, const std::string& appearance)
{
    require_idle(state);
    if (appearance != "primary" && appearance != "secondary")
    {
        fail("invalid_argument", "按钮样式仅支持 primary 或 secondary");
    }
    state.primary_button = appearance == "primary";
    InvalidateRect(state.hwnd, nullptr, FALSE);
}

bool theme_message(node& state, UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result)
{
    if (message == WM_DRAWITEM)
    {
        auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (item && item->CtlType == ODT_BUTTON)
        {
            if (auto* child = control_node(item->hwndItem); child && root_node(*child).light_theme)
            {
                draw_button(*child, *item);
                result = TRUE;
                return true;
            }
        }
    }
    if (!light_theme_active(state))
    {
        return false;
    }
    if (state.kind == tx::graphics_kind::button && (message == WM_MOUSEMOVE || message == WM_MOUSELEAVE))
    {
        const bool hover = message == WM_MOUSEMOVE;
        if (state.hovered != hover)
        {
            state.hovered = hover;
            InvalidateRect(state.hwnd, nullptr, FALSE);
            if (hover)
            {
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, state.hwnd, 0};
                TrackMouseEvent(&track);
            }
        }
    }
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT || message == WM_CTLCOLORBTN)
    {
        const auto dc = reinterpret_cast<HDC>(wparam);
        auto* child = control_node(reinterpret_cast<HWND>(lparam));
        const bool edit = child && child->kind == tx::graphics_kind::text_box;
        const auto color = edit ? surface : panel_color(state);
        SetTextColor(dc, child && !child->enabled ? muted : foreground);
        SetBkColor(dc, color);
        result = reinterpret_cast<LRESULT>(brush(dc, color));
        return true;
    }
    if (state.kind == tx::graphics_kind::container && !state.toolbar && !state.split &&
        (message == WM_PAINT || message == WM_ERASEBKGND))
    {
        PAINTSTRUCT paint{};
        const auto dc = message == WM_PAINT ? BeginPaint(state.hwnd, &paint) : reinterpret_cast<HDC>(wparam);
        RECT rect{};
        GetClientRect(state.hwnd, &rect);
        FillRect(dc, &rect, brush(dc, panel_color(state)));
        if (message == WM_PAINT)
        {
            EndPaint(state.hwnd, &paint);
        }
        result = 1;
        return true;
    }
    return false;
}
}
