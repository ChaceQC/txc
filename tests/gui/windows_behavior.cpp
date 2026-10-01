#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include "stdlib/gui/windows/layout_internal.hpp"
#include "backend/cpp/runtime_context.hpp"
#include "backend/cpp/gui_abi.hpp"
#include "backend/cpp/runtime_abi.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <thread>

using namespace tx_generated;
using namespace tx_generated::gui;

namespace
{

template<class operation>
void expect_error(const char* code, operation&& run)
{
    try
    {
        run();
        throw std::runtime_error("expected GUI error");
    }
    catch (const runtime_failure& error)
    {
        assert(error.error().kind == tx::error_kind::graphics);
        assert(error.error().code == code);
    }
}

void check_axis()
{
    const auto items = std::vector<axis_item>{
        {{length_mode::stretch, 1}, 0, 0, 20},
        {{length_mode::stretch, 1}, 0, 0, 200},
        {{length_mode::fixed, 10}, 10, 0, 200}};
    const auto sizes = allocate_axis(items, 110);
    assert(sizes[0] == 20 && sizes[1] == 80 && sizes[2] == 10);
    const auto small = allocate_axis({{{}, 80, 20, 200}, {{}, 80, 30, 200}}, 20);
    assert(small[0] == 20 && small[1] == 30);
}

void check_layout(graphics::window& window, node& root)
{
    set_layout(root, layout_mode::row, 0, 0);
    const auto first = create(root, tx::graphics_kind::button, "一");
    const auto second = create(root, tx::graphics_kind::button, "二");
    first->explicit_width = second->explicit_width = true;
    first->width = second->width = {length_mode::stretch, 1};
    for (const auto dpi : {120, 192})
    {
        RECT suggested{};
        GetWindowRect(window.hwnd, &suggested);
        SendMessageW(window.hwnd, WM_DPICHANGED, MAKELONG(dpi, dpi),
            reinterpret_cast<LPARAM>(&suggested));
        flush_layout(window);
        RECT a{};
        RECT b{};
        GetWindowRect(first->hwnd, &a);
        GetWindowRect(second->hwnd, &b);
        assert(a.right == b.left);
        assert(a.right - a.left >= 0 && b.right - b.left >= 0);
        assert(root.font_dpi == static_cast<UINT>(dpi));
    }
    set_visible(*first, false);
    flush_layout(window);
    assert(second->arranged.x == 0);
    first->reserved = true;
    dirty(root);
    flush_layout(window);
    assert(second->arranged.x > 0);
    close(*first);
    close(*second);
    set_layout(root, layout_mode::grid, 0, 4, 2, 2);
    root.columns[0] = {length_mode::fixed, 80};
    const auto span = create(root, tx::graphics_kind::label, "跨越两列的标签");
    set_cell(*span, 0, 0, 1, 2);
    flush_layout(window);
    assert(span->arranged.width > 80);
    expect_error("invalid_layout", [&]
    {
        set_layout(root, layout_mode::grid, 0, 0, 1, 1);
    });
    assert(root.columns.size() == 2 && span->column_span == 2);
    set_cell(*span, 0, 0, 1, 1);
    span->width = {length_mode::fixed, 160};
    dirty(root);
    flush_layout(window);
    assert(span->arranged.width == 80);
    close(*span);
}

void check_events(graphics::app& app, graphics::window& window, node& root)
{
    set_layout(root, layout_mode::column, 8, 8);
    const auto input = create(root, tx::graphics_kind::text_box);
    const auto button = create(root, tx::graphics_kind::button, "确定");
    const auto check = create(root, tx::graphics_kind::check_box, "通知");
    graphics::show_window(window);
    focus(*input);
    app.events.clear();
    set_text(*input, "程序设置");
    assert(app.events.empty());
    SetWindowTextW(input->hwnd, L"甲");
    SetWindowTextW(input->hwnd, L"乙");
    assert(app.events.size() == 1);
    assert(app.events.front().control->text == "乙");
    assert(app.events.front().control->revision == input->revision);
    MSG key{};
    key.hwnd = input->hwnd;
    key.message = WM_KEYDOWN;
    key.wParam = VK_RETURN;
    input->composing = true;
    assert(!translate_message(app, key));
    input->composing = false;
    assert(translate_message(app, key));
    assert(app.events.back().control->action == "text_committed");
    SetWindowTextW(input->hwnd, L"丙");
    assert(app.events.size() == 3);
    set_password(*input, true);
    app.events.clear();
    SetWindowTextW(input->hwnd, L"秘密");
    assert(!app.events.back().control->text);
    set_text(*input, "甲😀é");
    set_selection(*input, 1, 2);
    assert(selection(*input, false) == 1 && selection(*input, true) == 2);
    expect_error("invalid_argument", [&]
    {
        set_selection(*input, 0, 100);
    });
    key.wParam = VK_TAB;
    assert(translate_message(app, key));
    assert(GetFocus() == button->hwnd);
    app.events.clear();
    SendMessageW(button->hwnd, BM_CLICK, 0, 0);
    assert(app.events.back().control->action == "activated");
    SendMessageW(check->hwnd, BM_CLICK, 0, 0);
    assert(check->check == BST_CHECKED && app.events.back().control->state == true);
    app.events.clear();
    set_check(*check, BST_UNCHECKED);
    assert(app.events.empty());
    root.cancel_button = button;
    key.hwnd = button->hwnd;
    key.wParam = VK_ESCAPE;
    assert(translate_message(app, key));
    assert(app.events.back().control->source_id == button->id);
    focus(*input);
    close(*input);
    assert(GetFocus() == button->hwnd);
    set_enabled(*button, false);
    assert(GetFocus() == check->hwnd);
    auto alias = graphics::make_handle(check);
    alias.owner.reset();
    graphics::drain(app);
    assert(!check->closed && IsWindow(check->hwnd));
    const auto id = check->id;
    app.events.clear();
    notify(*check, "activated");
    close(*check);
    assert(app.events.front().control->source_id == id);
    close(*button);
}

void check_errors(graphics::app& app, graphics::window& window, node& root)
{
    const auto button = create(root, tx::graphics_kind::button, "队列");
    app.events.clear();
    for (std::size_t index = 0; index < graphics::event_limit; ++index)
    {
        notify(*button, "activated");
    }
    SendMessageW(button->hwnd, BM_CLICK, 0, 0);
    assert(app.queue_failed || (app.pending_error && app.pending_error->code == "resource_limit"));
    expect_error("resource_limit", [&]
    {
        graphics::next_event(app, 0);
    });
    app.events.clear();
    std::thread worker([&]
    {
        expect_error("wrong_thread", [&]
        {
            require_node(button.get());
        });
    });
    worker.join();
    flush_layout(window);
    const auto frame = graphics::begin_frame(window);
    assert(frame);
    expect_error("invalid_frame", [&]
    {
        flush_layout(window);
    });
    graphics::end_frame(*frame);
    const auto native = button->hwnd;
    graphics::close_window(window);
    assert(root.closed && button->closed && !IsWindow(native));
    close(*button);
    expect_error("closed_resource", [&]
    {
        require_node(button.get());
    });
}

} // namespace

int main()
{
    detail::current_runtime_context().main_thread = true;
    check_axis();
    const auto app = graphics::open_app();
    const auto app_lease = graphics::make_handle(app);
    const auto window = graphics::create_window(*app, "GUI U0-U1 native", 480, 300, true);
    const auto window_lease = graphics::make_handle(window);
    const auto tree = root(*window);
    check_layout(*window, *tree);
    check_events(*app, *window, *tree);
    check_errors(*app, *window, *tree);
    graphics::close_app(*app);
    std::cout << "gui native lifecycle/layout/events/DPI: PASS\n";
}
