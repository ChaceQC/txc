#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/platform/windows_window.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <future>
#include <iostream>

namespace ui = tx_generated::native_gui;

namespace
{
void respond(const wchar_t* title, bool enter)
{
    for (int attempt = 0; attempt < 1000; ++attempt)
    {
        if (const auto hwnd = FindWindowW(nullptr, title); hwnd && IsWindowVisible(hwnd))
        {
            if (enter)
            {
                PostMessageW(hwnd, WM_KEYDOWN, VK_RETURN, 0);
                PostMessageW(hwnd, WM_KEYUP, VK_RETURN, 0);
            }
            else
            {
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    throw std::runtime_error("dialog did not appear");
}
}

int main(int argc, char** argv)
{
    try
    {
        tx_generated::detail::current_runtime_context().main_thread = true;
        const auto app = ui::open_app(argc > 1 ? argv[1] : "");
        const auto owner = ui::create_window(*app, "TX dialog fixture owner", 600, 360);
        ui::root(*owner);
        owner->visible = true;
        owner->host->show();
        auto confirm = std::async(std::launch::async, []
        {
            respond(L"TX message fixture", true);
        });
        const auto answer = ui::message_box(*owner, "TX message fixture", "确认默认动作和 owner 恢复", "yes_no_cancel");
        confirm.get();
        if (answer != "yes" || !owner->modal_child.expired())
        {
            throw std::runtime_error("message result or modal restoration failed");
        }
        auto cancel = std::async(std::launch::async, []
        {
            respond(L"TX file cancel fixture", false);
        });
        const auto selected = ui::file_dialog(*owner, "open", "TX file cancel fixture", "");
        cancel.get();
        const auto hwnd = static_cast<tx::ui::windows_window&>(*owner->host).hwnd;
        if (selected || owner->system_modal || !IsWindowEnabled(hwnd))
        {
            throw std::runtime_error("file cancellation or owner restoration failed");
        }
        ui::close_app(*app);
        std::cout << "self-rendered message / Windows Shell cancellation / owner restoration PASS\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
