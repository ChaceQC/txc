#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include "stdlib/graphics/windows/state.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <thread>

using namespace tx_generated::graphics;

namespace
{

template<class operation>
void expect_error(const char* code, operation&& run)
{
    try
    {
        run();
        throw std::runtime_error("expected graphics error");
    }
    catch (const tx_generated::runtime_failure& error)
    {
        assert(error.error().kind == tx::error_kind::graphics);
        assert(error.error().code == code);
    }
}

void check_window_protocol(app& owner, window& child)
{
    assert(!begin_frame(child));
    show_window(child);
    ShowWindow(child.hwnd, SW_MINIMIZE);
    assert(child.minimized && !begin_frame(child));
    ShowWindow(child.hwnd, SW_RESTORE);
    assert(!child.minimized);
    SetWindowPos(child.hwnd, nullptr, 0, 0, 320, 240, SWP_NOMOVE | SWP_NOZORDER);
    RECT suggested{};
    GetWindowRect(child.hwnd, &suggested);
    SendMessageW(child.hwnd, WM_DPICHANGED, MAKELONG(192, 192),
        reinterpret_cast<LPARAM>(&suggested));
    assert(child.dpi == 192);
    assert(owner.events.back().kind == "dpi_changed");
    assert(owner.events.back().resize->width == child.width / 2.0);
    const auto frame = begin_frame(child);
    assert(frame && frame->active);
    end_frame(*frame);
    assert(!frame->active);
    expect_error("invalid_frame", [&]
    {
        require_canvas(frame.get());
    });
    owner.events.clear();
    SendMessageW(child.hwnd, WM_CLOSE, 0, 0);
    assert(!child.closed && owner.events.back().kind == "close_requested");
    const auto hwnd = child.hwnd;
    close_window(child);
    close_window(child);
    assert(!IsWindow(hwnd));
    assert(owner.events.size() == 2 && owner.events.back().kind == "closed");
}

void check_queue(app& owner, window& child)
{
    owner.events.clear();
    for (int index = 0; index < 5000; ++index)
    {
        enqueue(child, "paint");
    }
    assert(owner.events.size() == 1);
    owner.events.clear();
    for (std::size_t index = 0; index < event_limit + 1; ++index)
    {
        enqueue(child, "close_requested");
    }
    assert(owner.events.size() == event_limit && owner.queue_failed);
    expect_error("resource_limit", [&]
    {
        next_event(owner, 0);
    });
    owner.events.clear();
}

} // namespace

int main()
{
    auto& context = tx_generated::detail::current_runtime_context();
    context.main_thread = true;
    const auto initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    assert(SUCCEEDED(initialized));
    expect_error("platform_error", []
    {
        open_app();
    });
    assert(context.graphics_native_error == RPC_E_CHANGED_MODE);
    CoUninitialize();
    const auto owner = open_app();
    auto app_lease = make_handle(owner);
    const auto first = create_window(*owner, "G1 lifecycle", 160, 120, true);
    auto first_lease = make_handle(first);
    assert(first->width == static_cast<UINT>(std::round(160.0 * first->dpi / 96)));
    std::thread worker([&]
    {
        expect_error("wrong_thread", [&]
        {
            require_window(first.get());
        });
    });
    worker.join();
    check_window_protocol(*owner, *first);
    drain(*owner);
    const auto second = create_window(*owner, "G1 queue", 80, 60, false);
    auto second_lease = make_handle(second);
    check_queue(*owner, *second);
    while (next_event(*owner, 0))
    {
    }
    const auto started = std::chrono::steady_clock::now();
    assert(!next_event(*owner, 80));
    assert(std::chrono::steady_clock::now() - started >= std::chrono::milliseconds(60));
    second_lease.owner.reset();
    drain(*owner);
    assert(second->closed);
    const auto third = create_window(*owner, "G1 frame cleanup", 80, 60, true);
    auto third_lease = make_handle(third);
    show_window(*third);
    const auto abandoned = begin_frame(*third);
    auto frame_lease = make_handle(abandoned);
    frame_lease.owner.reset();
    drain(*owner);
    assert(!owner->frame && !abandoned->active);
    const auto active = begin_frame(*third);
    const auto hwnd = third->hwnd;
    context.close_graphics();
    assert(!owner->open && third->closed && !active->active && !IsWindow(hwnd));
    expect_error("closed_resource", [&]
    {
        require_canvas(active.get());
    });
    expect_error("invalid_argument", []
    {
        checked_number(std::numeric_limits<double>::infinity());
    });
    expect_error("invalid_argument", []
    {
        checked_color({0, 0, 0, -0.1});
    });
    std::cout << "graphics native lifecycle: PASS\n";
}
