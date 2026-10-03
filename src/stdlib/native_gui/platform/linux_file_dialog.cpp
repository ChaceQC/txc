#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/platform/linux_bus.hpp"
#include "stdlib/native_gui/platform/linux_window.hpp"

#include <sstream>

namespace tx_generated::native_gui
{
namespace
{
constexpr auto portal = "org.freedesktop.portal.Desktop";
constexpr auto portal_path = "/org/freedesktop/portal/desktop";
struct response
{
    explicit response(tx::ui::bus_api& value) : api(value)
    {
    }
    tx::ui::bus_api& api;
    std::string path, uri;
    unsigned code = 2;
    bool finished = false;
    std::string error;
};

int receive_response(sd_bus_message* message, void* userdata, sd_bus_error*)
{
    auto& state = *static_cast<response*>(userdata);
    auto& api = state.api;
    if (state.path != api.get_path(message))
    {
        return 0;
    }
    try
    {
        tx::ui::bus_check(api.read(message, "u", &state.code));
        tx::ui::bus_check(api.enter_container(message, 'a', "{sv}"));
        while (api.enter_container(message, 'e', "sv") > 0)
        {
            const char* key = nullptr;
            tx::ui::bus_check(api.read(message, "s", &key));
            if (std::string_view(key) == "uris")
            {
                tx::ui::bus_check(api.enter_container(message, 'v', "as"));
                tx::ui::bus_check(api.enter_container(message, 'a', "s"));
                const char* uri = nullptr;
                if (api.read(message, "s", &uri) > 0)
                {
                    state.uri = uri;
                }
                while (api.skip(message, "s") > 0)
                {
                }
                tx::ui::bus_check(api.exit_container(message));
                tx::ui::bus_check(api.exit_container(message));
            }
            else
            {
                tx::ui::bus_check(api.skip(message, "v"));
            }
            tx::ui::bus_check(api.exit_container(message));
        }
    }
    catch (const std::exception& error)
    {
        state.error = error.what();
    }
    state.finished = true;
    return 1;
}

std::string local_path(std::string uri)
{
    if (uri.starts_with("file://localhost/"))
    {
        uri.erase(7, 9);
    }
    if (!uri.starts_with("file:///"))
    {
        fail("platform_error", "文件选择器未返回本地文件 URI");
    }
    std::string result;
    for (std::size_t index = 7; index < uri.size(); ++index)
    {
        if (uri[index] == '%')
        {
            auto hex = [](char value) -> int
            {
                return value >= '0' && value <= '9' ? value - '0' :
                    value >= 'A' && value <= 'F' ? value - 'A' + 10 :
                    value >= 'a' && value <= 'f' ? value - 'a' + 10 : -1;
            };
            if (index + 2 >= uri.size() || hex(uri[index + 1]) < 0 || hex(uri[index + 2]) < 0)
            {
                fail("platform_error", "文件 URI 转义无效");
            }
            result += static_cast<char>(hex(uri[index + 1]) * 16 + hex(uri[index + 2]));
            index += 2;
        }
        else
        {
            result += uri[index];
        }
    }
    if (result.find('\0') != std::string::npos)
    {
        fail("platform_error", "文件 URI 包含空字符");
    }
    tx::ui::decode_utf8(result);
    return result;
}
}

std::optional<std::string> file_dialog(window& owner, const std::string& kind,
    const std::string& title, const std::string& initial)
{
    if (kind != "open" && kind != "save" && kind != "folder")
    {
        fail("invalid_argument", "文件对话框类型必须为 open/save/folder");
    }
    if (!owner.modal_child.expired() || owner.system_modal)
    {
        fail("modal_blocked", "owner 已被模态窗口阻塞");
    }
    tx::ui::system_bus bus;
    auto& api = bus.api;
    response selected{api};
    sd_bus_slot* slot = nullptr;
    tx::ui::bus_check(api.add_match(bus.value, &slot,
        "type='signal',sender='org.freedesktop.portal.Desktop',interface='org.freedesktop.portal.Request',member='Response'", receive_response, &selected));
    struct cleanup
    {
        tx::ui::bus_api& api;
        sd_bus_slot* slot;
        window& owner;
        tx::ui::system_bus& bus;
        response& selected;
        ~cleanup()
        {
            if (!selected.finished && !selected.path.empty())
            {
                tx::ui::bus_message close{api};
                if (api.new_call(bus.value, &close.value, portal, selected.path.c_str(), "org.freedesktop.portal.Request", "Close") >= 0)
                {
                    api.send(bus.value, close.value, nullptr);
                }
            }
            api.slot_unref(slot);
            owner.system_modal = false;
        }
    } guard{api, slot, owner, bus, selected};
    tx::ui::bus_message request{api}, reply{api};
    tx::ui::bus_check(api.new_call(bus.value, &request.value, portal, portal_path,
        "org.freedesktop.portal.FileChooser", kind == "save" ? "SaveFile" : "OpenFile"));
    std::ostringstream parent;
    parent << "x11:" << std::hex << static_cast<tx::ui::linux_window&>(*owner.host).native_id();
    tx::ui::bus_check(api.append(request.value, "ss", parent.str().c_str(), title.c_str()));
    tx::ui::bus_check(api.open_container(request.value, 'a', "{sv}"));
    if (kind == "folder")
    {
        tx::ui::bus_check(api.append(request.value, "{sv}", "directory", "b", 1));
    }
    if (!initial.empty())
    {
        tx::ui::bus_check(api.append(request.value, "{sv}", "current_name", "s", initial.c_str()));
    }
    tx::ui::bus_check(api.close_container(request.value));
    tx::ui::bus_check(api.call(bus.value, request.value, 10000000, nullptr, &reply.value));
    const char* path = nullptr;
    tx::ui::bus_check(api.read(reply.value, "o", &path));
    selected.path = path;
    owner.system_modal = true;
    if (owner.native_gui_root)
    {
        reset_interaction(*owner.native_gui_root);
    }
    auto app = owner.owner.lock();
    std::deque<event> deferred;
    struct restore_events
    {
        native_gui::app& application;
        std::deque<event>& events;
        ~restore_events()
        {
            application.events.insert(application.events.begin(),
                std::make_move_iterator(events.begin()), std::make_move_iterator(events.end()));
        }
    } restore{*app, deferred};
    while (!selected.finished && !owner.closed)
    {
        bus.poll();
        if (const auto input = next_event(*app, 20))
        {
            deferred.push_back(*input);
        }
    }
    if (!selected.error.empty() || selected.code > 1)
    {
        fail("platform_error", selected.error.empty() ? "桌面文件选择请求失败" : selected.error);
    }
    return selected.code == 1 ? std::nullopt : std::optional<std::string>(local_path(selected.uri));
}
}
