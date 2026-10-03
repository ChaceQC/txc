#include "stdlib/native_gui/platform/linux_accessibility.hpp"
#include "stdlib/native_gui/commands.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <future>
#include <iostream>

namespace ui = tx_generated::native_gui;
namespace transport = tx::ui;

namespace
{
struct object
{
    std::string bus, path;
};

std::vector<object> children(transport::system_bus& bus, const object& parent)
{
    transport::bus_message reply{bus.api};
    transport::bus_check(bus.api.call_method(bus.value, parent.bus.c_str(), parent.path.c_str(),
        "org.a11y.atspi.Accessible", "GetChildren", nullptr, &reply.value, ""));
    transport::bus_check(bus.api.enter_container(reply.value, 'a', "(so)"));
    std::vector<object> result;
    const char* name = nullptr;
    const char* path = nullptr;
    while (bus.api.read(reply.value, "(so)", &name, &path) > 0)
    {
        result.push_back({name, path});
    }
    return result;
}

std::string name(transport::system_bus& bus, const object& value)
{
    transport::bus_message reply{bus.api};
    transport::bus_check(bus.api.call_method(bus.value, value.bus.c_str(), value.path.c_str(),
        "org.freedesktop.DBus.Properties", "Get", nullptr, &reply.value, "ss", "org.a11y.atspi.Accessible", "Name"));
    transport::bus_check(bus.api.enter_container(reply.value, 'v', "s"));
    const char* text = nullptr;
    transport::bus_check(bus.api.read(reply.value, "s", &text));
    return text;
}

object find(transport::system_bus& bus, const object& root, const std::string& target)
{
    if (name(bus, root) == target)
    {
        return root;
    }
    for (const auto& child : children(bus, root))
    {
        if (auto result = find(bus, child, target); !result.bus.empty())
        {
            return result;
        }
    }
    return {};
}

void client()
{
    transport::system_bus session;
    transport::bus_message reply{session.api};
    transport::bus_check(session.api.call_method(session.value, "org.a11y.Bus", "/org/a11y/bus", "org.a11y.Bus",
        "GetAddress", nullptr, &reply.value, ""));
    const char* address = nullptr;
    transport::bus_check(session.api.read(reply.value, "s", &address));
    transport::system_bus bus(address);
    const object registry{"org.a11y.atspi.Registry", "/org/a11y/atspi/accessible/root"};
    object application;
    for (const auto& candidate : children(bus, registry))
    {
        if (name(bus, candidate) == "AT-SPI protocol check")
        {
            application = candidate;
            break;
        }
    }
    if (application.bus.empty())
    {
        throw std::runtime_error("AT-SPI registry did not contain application");
    }
    const auto button = find(bus, application, "执行命令");
    const auto editor = find(bus, application, "输入内容");
    const auto slider = find(bus, application, "音量");
    transport::bus_message invoked{bus.api}, edited{bus.api}, ranged{bus.api}, text{bus.api};
    transport::bus_check(bus.api.call_method(bus.value, button.bus.c_str(), button.path.c_str(), "org.a11y.atspi.Action",
        "DoAction", nullptr, &invoked.value, "i", 0));
    int accepted = 0;
    transport::bus_check(bus.api.read(invoked.value, "b", &accepted));
    if (!accepted)
    {
        throw std::runtime_error("AT-SPI action rejected");
    }
    transport::bus_check(bus.api.call_method(bus.value, editor.bus.c_str(), editor.path.c_str(), "org.a11y.atspi.EditableText",
        "SetTextContents", nullptr, &edited.value, "s", "AT-SPI 修改😀"));
    transport::bus_check(bus.api.call_method(bus.value, slider.bus.c_str(), slider.path.c_str(), "org.freedesktop.DBus.Properties",
        "Set", nullptr, &ranged.value, "ssv", "org.a11y.atspi.Value", "CurrentValue", "d", 73.0));
    transport::bus_check(bus.api.call_method(bus.value, editor.bus.c_str(), editor.path.c_str(), "org.a11y.atspi.Text",
        "GetText", nullptr, &text.value, "ii", 0, -1));
    const char* content = nullptr;
    transport::bus_check(bus.api.read(text.value, "s", &content));
    if (std::string_view(content) != "AT-SPI 修改😀")
    {
        throw std::runtime_error("AT-SPI text read mismatch");
    }
}
}

int main(int argc, char** argv)
{
    try
    {
        tx_generated::detail::current_runtime_context().main_thread = true;
        auto app = ui::open_app(argc > 1 ? argv[1] : "");
        auto window = ui::create_window(*app, "AT-SPI protocol check", 600, 360);
        if (!window->accessibility_platform)
        {
            throw std::runtime_error("AT-SPI service unavailable; run in a desktop or dbus-run-session");
        }
        auto root = ui::root(*window);
        auto command = ui::create_command(*window, "执行命令", "", true);
        auto button = ui::create(*root, tx::graphics_kind::native_button, "执行命令");
        ui::bind_command(*button, *command);
        auto editor = ui::create(*root, tx::graphics_kind::native_text_box, "初始文本");
        ui::set_accessibility(*editor, "输入内容", "");
        auto slider = ui::create(*root, tx::graphics_kind::native_slider, "");
        ui::set_accessibility(*slider, "音量", "");
        window->visible = true;
        window->host->show();
        auto result = std::async(std::launch::async, client);
        while (result.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        {
            ui::next_event(*app, 20);
        }
        result.get();
        if (!command->checked || editor->editor->text() != U"AT-SPI 修改😀" || slider->value != 73)
        {
            throw std::runtime_error("AT-SPI did not operate actual controls");
        }
        ui::close_app(*app);
        std::cout << "AT-SPI registry / traversal / Action / EditableText / Text / Value PASS\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
