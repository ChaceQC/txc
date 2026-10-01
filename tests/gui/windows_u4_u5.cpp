#include "stdlib/gui/windows/accessibility.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <oleacc.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <thread>

using namespace tx_generated;
namespace gfx = tx_generated::graphics;
namespace gui = tx_generated::gui;

namespace
{
template<class operation>
void expect(const char* code, operation&& run)
{
    try
    {
        run();
        throw std::runtime_error("expected error");
    }
    catch (const runtime_failure& error)
    {
        assert(error.error().code == code);
    }
}

void tabs_and_scroll(gfx::window& window, gui::node& root)
{
    const auto tabs = gui::create(root, tx::graphics_kind::tabs);
    tabs->height = {gui::length_mode::stretch, 1};
    const auto first = gui::add_tab(*tabs, "一");
    const auto second = gui::add_tab(*tabs, "二");
    const auto third = gui::add_tab(*tabs, "三");
    gui::select_tab(*tabs, *second);
    gui::close(*second);
    assert(tabs->selected_page.lock() == third);
    gui::select_tab(*tabs, *first);
    const auto button = gui::create(*first, tx::graphics_kind::button, "按钮");
    gui::flush_layout(window);
    SetFocus(button->hwnd);
    gui::select_tab(*tabs, *third);
    assert(!first->page_active && !IsWindowVisible(first->hwnd));
    assert(GetFocus() != button->hwnd);
    expect("invalid_argument", [&]
    {
        gui::focus(*button);
    });
    SendMessageW(tabs->hwnd, TCM_SETCURSEL, 0, 0);
    NMHDR header{tabs->hwnd, 0, TCN_SELCHANGE};
    SendMessageW(root.hwnd, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&header));
    assert(tabs->selected_page.lock() == first);
    assert(window.owner.lock()->events.back().control->item_id == first->id);
    gui::close(*first);
    assert(tabs->selected_page.lock() == third && third->page_active);
    auto scroll = gui::create_scroll(*third, true, true);
    scroll->height = {gui::length_mode::stretch, 1};
    auto content = gui::create(*scroll, tx::graphics_kind::button, "滚动内容");
    content->height = {gui::length_mode::fixed, 900};
    content->width = {gui::length_mode::fixed, 900};
    gui::flush_layout(window);
    gui::set_scroll_position(*scroll, 40, 100);
    gui::flush_layout(window);
    RECT rect{};
    GetWindowRect(content->hwnd, &rect);
    POINT origin{rect.left, rect.top};
    MapWindowPoints(nullptr, scroll->hwnd, &origin, 1);
    assert(std::abs(origin.y + 100 * window.dpi / 96.0) <= 1);
    assert(std::abs(origin.x + 40 * window.dpi / 96.0) <= 1);
    SendMessageW(scroll->hwnd, WM_VSCROLL, SB_BOTTOM, 0);
    gui::flush_layout(window);
    assert(scroll->scroll_y > 100);
    gui::complex_key(*scroll, VK_HOME);
    gui::flush_layout(window);
    assert(scroll->scroll_y == 0);
    auto canvas = gui::create(*scroll, tx::graphics_kind::gui_canvas);
    canvas->height = {gui::length_mode::fixed, 80};
    gui::flush_layout(window);
    auto* provider = static_cast<IRawElementProviderSimple*>(canvas->accessibility.get());
    VARIANT offscreen{};
    assert(SUCCEEDED(provider->GetPropertyValue(UIA_IsOffscreenPropertyId, &offscreen)));
    assert(offscreen.boolVal == VARIANT_TRUE);
    SendMessageW(scroll->hwnd, WM_VSCROLL, SB_BOTTOM, 0);
    gui::flush_layout(window);
    assert(SUCCEEDED(provider->GetPropertyValue(UIA_IsOffscreenPropertyId, &offscreen)));
    assert(offscreen.boolVal == VARIANT_FALSE);
    VariantClear(&offscreen);
    gui::close(*tabs);
}

void accessibility_and_canvas(gfx::window& window, gui::node& root)
{
    auto split = gui::create_split(root, false, 0.5, 60, 80);
    split->height = {gui::length_mode::stretch, 1};
    auto canvas = gui::create(*split->children[1], tx::graphics_kind::gui_canvas);
    canvas->height = {gui::length_mode::stretch, 1};
    auto label = gui::create(*split->children[0], tx::graphics_kind::label, "说明名称");
    auto input = gui::create(*split->children[0], tx::graphics_kind::text_box);
    gui::set_label(*input, *label);
    gui::set_accessibility(*split, "分隔条", "方向键调整");
    gui::set_accessibility(*canvas, "绘图区", "空格刷新");
    gui::flush_layout(window);

    IAccessible* accessible = nullptr;
    assert(SUCCEEDED(AccessibleObjectFromWindow(input->hwnd, OBJID_CLIENT, IID_IAccessible,
        reinterpret_cast<void**>(&accessible))));
    gfx::com_ptr<IAccessible> native(accessible);
    VARIANT child{};
    child.vt = VT_I4;
    child.lVal = CHILDID_SELF;
    BSTR name = nullptr;
    assert(SUCCEEDED(native->get_accName(child, &name)));
    assert(std::wstring(name) == L"说明名称");
    SysFreeString(name);
    gui::set_text(*label, "新名称");
    gui::flush_layout(window);
    assert(SUCCEEDED(native->get_accName(child, &name)));
    assert(std::wstring(name) == L"新名称");
    SysFreeString(name);

    IRangeValueProvider* range = nullptr;
    assert(SUCCEEDED(split->accessibility->QueryInterface(__uuidof(IRangeValueProvider), reinterpret_cast<void**>(&range))));
    gfx::com_ptr<IRangeValueProvider> range_owner(range);
    std::thread client([&]
    {
        double value = 0;
        assert(SUCCEEDED(range->get_Value(&value)) && value == 0.5);
        assert(SUCCEEDED(range->SetValue(0.65)));
    });
    client.join();
    gfx::next_event(*window.owner.lock(), 0);
    assert(std::abs(split->split_position - 0.65) < 0.0001);
    gui::complex_key(*split, VK_LEFT);
    assert(std::abs(split->split_position - 0.63) < 0.0001);
    gui::flush_layout(window);
    const auto divider_x = static_cast<int>((split->divider.x + 2) * window.dpi / 96.0);
    SendMessageW(split->hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(divider_x, 10));
    assert(split->dragging);
    SendMessageW(split->hwnd, WM_KILLFOCUS, 0, 0);
    assert(!split->dragging && GetCapture() != split->hwnd);
    gui::set_enabled(root, false);
    BOOL read_only = FALSE;
    assert(SUCCEEDED(range->get_IsReadOnly(&read_only)) && read_only);
    assert(range->SetValue(0.2) == UIA_E_ELEMENTNOTENABLED);
    gui::set_enabled(root, true);
    const auto previous_position = split->split_position;
    assert(SUCCEEDED(range->SetValue(0.9)));
    EnableWindow(window.hwnd, FALSE);
    assert(range->SetValue(0.9) == UIA_E_ELEMENTNOTENABLED);
    gfx::next_event(*window.owner.lock(), 0);
    assert(split->split_position == previous_position);
    EnableWindow(window.hwnd, TRUE);

    IInvokeProvider* invoke = nullptr;
    assert(SUCCEEDED(canvas->accessibility->QueryInterface(__uuidof(IInvokeProvider), reinterpret_cast<void**>(&invoke))));
    gfx::com_ptr<IInvokeProvider> invoke_owner(invoke);
    window.owner.lock()->events.clear();
    assert(SUCCEEDED(invoke->Invoke()));
    gfx::next_event(*window.owner.lock(), 0);
    gui::sync_canvas(*canvas);
    SendMessageW(canvas->hwnd, WM_KEYDOWN, 'A', 0);
    assert(canvas->canvas_window->keys['A']);
    SendMessageW(window.hwnd, WM_ACTIVATEAPP, FALSE, 0);
    assert(!canvas->canvas_window->keys['A']);
    window.owner.lock()->events.clear();
    SendMessageW(canvas->hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(20, 30));
    const auto& event = window.owner.lock()->events.back();
    assert(event.pointer && event.control->source_id == canvas->id && event.window_id == window.id);
    window.owner.lock()->events.clear();
    SendMessageW(canvas->hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(10, 20));
    SendMessageW(window.hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(30, 40));
    assert(window.owner.lock()->events.size() == 2);

    for (const auto dpi : {120u, 192u})
    {
        window.dpi = dpi;
        gui::window_changed(window, true);
        gui::flush_layout(window);
        assert(root.font_dpi == dpi && canvas->canvas_window->dpi == dpi);
        auto frame = gui::begin_canvas(*canvas);
        assert(frame);
        expect("invalid_frame", [&]
        {
            gfx::begin_frame(window);
        });
        gfx::clear_canvas(*frame, {0.2, 0.3, 0.4, 1});
        gfx::end_frame(*frame);
        // 模拟目标失效后的重建，控件身份与页面树必须保留。
        canvas->canvas_window->target.reset();
        frame = gui::begin_canvas(*canvas);
        gfx::end_frame(*frame);
        assert(canvas->canvas_window->target && !canvas->closed);
    }
    SendMessageW(window.hwnd, WM_THEMECHANGED, 0, 0);
    assert(root.font_dpi == 0 && root.dirty);
    gui::flush_layout(window);
    gui::set_visible(*canvas, false);
    assert(!gui::begin_canvas(*canvas));
    gui::set_visible(*canvas, true);
    auto frame = gui::begin_canvas(*canvas);
    const auto hwnd = canvas->hwnd;
    gui::close(*split);
    assert(!window.owner.lock()->frame && !frame->active && !IsWindow(hwnd));
    assert(invoke->Invoke() == UIA_E_ELEMENTNOTAVAILABLE);
    double value = 0;
    assert(range->get_Value(&value) == UIA_E_ELEMENTNOTAVAILABLE);
}

void exception_cleanup()
{
    std::shared_ptr<gfx::app> app;
    std::shared_ptr<gui::node> canvas;
    std::shared_ptr<gfx::canvas> frame;
    HWND hwnd = nullptr;
    try
    {
        app = gfx::open_app();
        const auto app_handle = gfx::make_handle(app);
        auto window = gfx::create_window(*app, "异常清理验证", 300, 200, true);
        const auto window_handle = gfx::make_handle(window);
        auto root = gui::root(*window);
        canvas = gui::create(*root, tx::graphics_kind::gui_canvas);
        gfx::show_window(*window);
        gui::flush_layout(*window);
        hwnd = window->hwnd;
        frame = gui::begin_canvas(*canvas);
        assert(frame);
        throw std::runtime_error("模拟 TX 栈异常展开");
    }
    catch (const std::runtime_error&)
    {
        gfx::drain(*app);
    }
    assert(!app->open && !app->frame && !frame->active && canvas->closed && !IsWindow(hwnd));
}
}

int main()
{
    detail::current_runtime_context().main_thread = true;
    const auto app = gfx::open_app();
    const auto app_handle = gfx::make_handle(app);
    const auto window = gfx::create_window(*app, "U4-U5 原生验证", 600, 420, true);
    const auto window_handle = gfx::make_handle(window);
    const auto root = gui::root(*window);
    gfx::show_window(*window);
    tabs_and_scroll(*window, *root);
    accessibility_and_canvas(*window, *root);
    const auto hwnd = window->hwnd;
    gfx::close_app(*app);
    assert(!IsWindow(hwnd) && root->closed);
    exception_cleanup();
    std::cout << "native tabs/scroll/split/UIA/DPI/canvas/cleanup: PASS\n";
}
