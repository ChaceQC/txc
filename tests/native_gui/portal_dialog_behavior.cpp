#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/platform/linux_bus.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <atomic>
#include <dlfcn.h>
#include <future>
#include <iostream>

namespace ui = tx_generated::native_gui;
namespace transport = tx::ui;

namespace
{
struct fixture
{
    transport::system_bus bus;
    unsigned count = 0;
    static int receive(sd_bus_message* request, void* data, sd_bus_error*)
    {
        auto& state = *static_cast<fixture*>(data);
        auto& api = state.bus.api;
        const auto member = api.get_member(request);
        if (!member || (std::string_view(member) != "OpenFile" && std::string_view(member) != "SaveFile"))
        {
            return 0;
        }
        try
        {
            const char* parent = nullptr;
            const char* title = nullptr;
            transport::bus_check(api.read(request, "ss", &parent, &title));
            if (!std::string_view(parent).starts_with("x11:"))
            {
                throw std::runtime_error("portal owner missing");
            }
            const std::string path = "/org/freedesktop/portal/desktop/request/fixture/r" + std::to_string(++state.count);
            transport::bus_message reply{api};
            transport::bus_check(api.new_reply(request, &reply.value));
            transport::bus_check(api.append(reply.value, "o", path.c_str()));
            transport::bus_check(api.send(state.bus.value, reply.value, nullptr));
            transport::bus_message signal{api};
            transport::bus_check(api.new_signal(state.bus.value, &signal.value, path.c_str(), "org.freedesktop.portal.Request", "Response"));
            const unsigned code = std::string_view(title) == "cancel" ? 1 : std::string_view(title) == "error" ? 2 : 0;
            transport::bus_check(api.append(signal.value, "u", code));
            transport::bus_check(api.open_container(signal.value, 'a', "{sv}"));
            if (code == 0)
            {
                transport::bus_check(api.append(signal.value, "{sv}", "uris", "as", 1, "file:///tmp/%E4%B8%AD%E6%96%87%20file.txt"));
            }
            transport::bus_check(api.close_container(signal.value));
            transport::bus_check(api.send(state.bus.value, signal.value, nullptr));
            return 1;
        }
        catch (const std::exception& error)
        {
            return api.reply_error(request, "org.freedesktop.DBus.Error.Failed", "%s", error.what());
        }
    }
};
}

int main(int argc, char** argv)
{
    try
    {
        tx_generated::detail::current_runtime_context().main_thread = true;
        auto app = ui::open_app(argc > 1 ? argv[1] : "");
        auto owner = ui::create_window(*app, "Portal fixture", 400, 240);
        ui::root(*owner);
        owner->visible = true;
        owner->host->show();
        std::promise<void> ready;
        auto ready_future = ready.get_future();
        std::atomic<bool> finished = false;
        auto service = std::async(std::launch::async, [&]
        {
            try
            {
                fixture server;
                const auto request_name = reinterpret_cast<int (*)(sd_bus*, const char*, std::uint64_t)>(
                    dlsym(server.bus.api.library, "sd_bus_request_name"));
                transport::bus_check(request_name(server.bus.value, "org.freedesktop.portal.Desktop", 0));
                transport::bus_check(server.bus.api.add_filter(server.bus.value, nullptr, fixture::receive, &server));
                ready.set_value();
                while (!finished)
                {
                    server.bus.poll();
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            }
            catch (...)
            {
                ready.set_exception(std::current_exception());
            }
        });
        ready_future.get();
        std::string failure;
        try
        {
            const auto selected = ui::file_dialog(*owner, "open", "accept", "");
            const auto cancelled = ui::file_dialog(*owner, "save", "cancel", "test.txt");
            bool rejected = false;
            try
            {
                ui::file_dialog(*owner, "folder", "error", "");
            }
            catch (const std::exception&)
            {
                rejected = true;
            }
            if (selected != "/tmp/中文 file.txt" || cancelled || !rejected || owner->system_modal)
            {
                failure = "portal result/cancel/error contract failed";
            }
        }
        catch (const std::exception& error)
        {
            failure = error.what();
        }
        finished = true;
        service.get();
        ui::close_app(*app);
        if (!failure.empty())
        {
            throw std::runtime_error(failure);
        }
        std::cout << "Portal independent peer / accept / cancel / error / URI decoding PASS\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
