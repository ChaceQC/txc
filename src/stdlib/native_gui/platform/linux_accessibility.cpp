#include "stdlib/native_gui/platform/linux_accessibility.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
constexpr auto accessible_prefix = "/org/a11y/atspi/accessible/";
constexpr auto registry = "org.a11y.atspi.Registry";
constexpr auto desktop = "/org/a11y/atspi/accessible/root";
}

atspi_bridge::atspi_bridge(window& owner) : endpoint(owner.accessibility)
{
    tx::ui::system_bus session;
    tx::ui::bus_message reply{session.api};
    tx::ui::bus_check(session.api.call_method(session.value, "org.a11y.Bus", "/org/a11y/bus",
        "org.a11y.Bus", "GetAddress", nullptr, &reply.value, ""));
    const char* bus_address = nullptr;
    tx::ui::bus_check(session.api.read(reply.value, "s", &bus_address));
    address = bus_address;
    bus = std::make_unique<tx::ui::system_bus>(address);
    name = bus->unique_name();
    tx::ui::bus_check(bus->api.add_filter(bus->value, &filter_, dispatch, this));
    tx::ui::bus_message embedded{bus->api};
    tx::ui::bus_check(bus->api.call_method(bus->value, registry, desktop, "org.a11y.atspi.Socket", "Embed",
        nullptr, &embedded.value, "(so)", name.c_str(), desktop));
    const char* parent_name = nullptr;
    const char* parent_path = nullptr;
    tx::ui::bus_check(bus->api.read(embedded.value, "(so)", &parent_name, &parent_path));
    desktop_name = parent_name;
    desktop_path = parent_path;
}

atspi_bridge::~atspi_bridge()
{
    if (bus)
    {
        tx::ui::bus_message request{bus->api};
        if (bus->api.new_call(bus->value, &request.value, registry, desktop, "org.a11y.atspi.Socket", "Unembed") >= 0)
        {
            bus->api.append(request.value, "(so)", name.c_str(), desktop);
            bus->api.send(bus->value, request.value, nullptr);
        }
        bus->api.slot_unref(filter_);
    }
}

std::string atspi_bridge::path(const std::string& id) const
{
    return id.empty() ? "/org/a11y/atspi/null" : std::string(accessible_prefix) + id;
}

void atspi_bridge::reference(sd_bus_message* reply, const std::string& id)
{
    tx::ui::bus_check(bus->api.append(reply, "(so)", id.empty() ? "" : name.c_str(), path(id).c_str()));
}

accessible_tree atspi_bridge::tree()
{
    auto result = endpoint->tree();
    accessible_node root;
    root.id = "root";
    root.name = result.at(endpoint->root_id).name;
    root.role = "application";
    root.enabled = root.visible = true;
    root.children = {endpoint->root_id};
    result[endpoint->root_id].parent = root.id;
    result.emplace(root.id, root);
    return result;
}

std::vector<std::string> atspi_bridge::interfaces(const accessible_node& value) const
{
    std::vector<std::string> result{"org.a11y.atspi.Accessible", "org.a11y.atspi.Component"};
    if (value.role == "application")
    {
        result.push_back("org.a11y.atspi.Application");
    }
    if (value.invoke || value.selectable)
    {
        result.push_back("org.a11y.atspi.Action");
    }
    if (value.range)
    {
        result.push_back("org.a11y.atspi.Value");
    }
    if (value.text_capable)
    {
        result.push_back("org.a11y.atspi.Text");
        if (value.editable)
        {
            result.push_back("org.a11y.atspi.EditableText");
        }
    }
    if (value.role == "list" || value.role == "table" || value.role == "tree" || value.role == "tabs" || value.role == "combo_box")
    {
        result.push_back("org.a11y.atspi.Selection");
    }
    return result;
}

int atspi_bridge::dispatch(sd_bus_message* message, void* userdata, sd_bus_error*)
{
    auto& state = *static_cast<atspi_bridge*>(userdata);
    auto& api = state.bus->api;
    std::uint8_t type = 0;
    api.get_type(message, &type);
    const auto raw_path = api.get_path(message);
    if (type != 1 || !raw_path || !std::string_view(raw_path).starts_with(accessible_prefix))
    {
        return 0;
    }
    try
    {
        const auto values = state.tree();
        const auto id = std::string(raw_path).substr(std::char_traits<char>::length(accessible_prefix));
        state.observed_ = state.observed_ || id.starts_with("n") || id.starts_with("m");
        const auto found = values.find(id);
        if (found == values.end())
        {
            return api.reply_error(message, "org.freedesktop.DBus.Error.UnknownObject", "%s", "辅助对象已经关闭");
        }
        tx::ui::bus_message reply{api};
        tx::ui::bus_check(api.new_reply(message, &reply.value));
        const std::string interface_name = api.get_interface(message) ? api.get_interface(message) : "";
        const std::string method = api.get_member(message) ? api.get_member(message) : "";
        bool handled = false;
        if (interface_name == "org.freedesktop.DBus.Introspectable" && method == "Introspect")
        {
            const auto xml = atspi_introspection(state.interfaces(found->second));
            tx::ui::bus_check(api.append(reply.value, "s", xml.c_str()));
            handled = true;
        }
        else if (interface_name == "org.freedesktop.DBus.Properties")
        {
            handled = state.properties(message, reply.value, found->second, values, method);
        }
        else
        {
            const auto supported = state.interfaces(found->second);
            if (std::find(supported.begin(), supported.end(), interface_name) != supported.end())
            {
                handled = state.methods(message, reply.value, found->second, values, interface_name, method) ||
                    state.text_methods(message, reply.value, found->second, interface_name, method);
            }
        }
        if (!handled)
        {
            return api.reply_error(message, "org.freedesktop.DBus.Error.UnknownMethod", "%s", "辅助方法或属性不存在");
        }
        tx::ui::bus_check(api.send(state.bus->value, reply.value, nullptr));
        return 1;
    }
    catch (const std::exception& error)
    {
        return api.reply_error(message, "org.freedesktop.DBus.Error.Failed", "%s", error.what());
    }
}

void atspi_bridge::signal(const accessible_node& value, const char* event, const std::string& detail,
    int first, int second, const std::string& text)
{
    auto& api = bus->api;
    tx::ui::bus_message message{api};
    tx::ui::bus_check(api.new_signal(bus->value, &message.value, path(value.id).c_str(), "org.a11y.atspi.Event.Object", event));
    tx::ui::bus_check(api.append(message.value, "siiv", detail.c_str(), first, second, "s", text.c_str()));
    tx::ui::bus_check(api.open_container(message.value, 'a', "{sv}"));
    tx::ui::bus_check(api.close_container(message.value));
    tx::ui::bus_check(api.send(bus->value, message.value, nullptr));
}

void atspi_bridge::poll(bool changed)
{
    bus->poll();
    if (!changed || !observed_)
    {
        return;
    }
    const auto values = tree();
    for (const auto& [id, value] : values)
    {
        const auto previous = previous_.find(id);
        if (previous == previous_.end())
        {
            continue;
        }
        const auto& old = previous->second;
        if (old.focused != value.focused)
        {
            signal(value, "StateChanged", "focused", value.focused);
        }
        if (old.enabled != value.enabled)
        {
            signal(value, "StateChanged", "enabled", value.enabled);
        }
        if (old.checked != value.checked)
        {
            signal(value, "StateChanged", "checked", value.checked);
        }
        if (old.selected != value.selected)
        {
            signal(value, "StateChanged", "selected", value.selected);
            signal(values.at(value.parent), "SelectionChanged", "");
        }
        if (!old.password && !value.password && old.text != value.text)
        {
            signal(value, "TextChanged", "delete", 0, old.text.size(), tx::ui::encode_utf8(old.text));
            signal(value, "TextChanged", "insert", 0, value.text.size(), tx::ui::encode_utf8(value.text));
        }
        if (old.caret != value.caret)
        {
            signal(value, "TextCaretMoved", "", value.caret);
        }
        if (old.selection_start != value.selection_start || old.selection_end != value.selection_end)
        {
            signal(value, "TextSelectionChanged", "");
        }
        if (old.visible != value.visible)
        {
            signal(value, "StateChanged", "showing", value.visible);
        }
        if (old.name != value.name)
        {
            signal(value, "PropertyChange", "accessible-name", 0, 0, value.name);
        }
        if (old.value != value.value)
        {
            signal(value, "PropertyChange", "accessible-value", 0, 0, std::to_string(value.value));
        }
        if (old.children != value.children)
        {
            auto notify = [&](const std::string& child, const char* operation, int index)
            {
                auto& api = bus->api;
                tx::ui::bus_message message{api};
                tx::ui::bus_check(api.new_signal(bus->value, &message.value, path(value.id).c_str(),
                    "org.a11y.atspi.Event.Object", "ChildrenChanged"));
                tx::ui::bus_check(api.append(message.value, "siiv", operation, index, 0, "(so)", name.c_str(), path(child).c_str()));
                tx::ui::bus_check(api.open_container(message.value, 'a', "{sv}"));
                tx::ui::bus_check(api.close_container(message.value));
                tx::ui::bus_check(api.send(bus->value, message.value, nullptr));
            };
            for (std::size_t index = 0; index < old.children.size(); ++index)
            {
                if (std::find(value.children.begin(), value.children.end(), old.children[index]) == value.children.end())
                {
                    notify(old.children[index], "remove", index);
                }
            }
            for (std::size_t index = 0; index < value.children.size(); ++index)
            {
                if (std::find(old.children.begin(), old.children.end(), value.children[index]) == old.children.end())
                {
                    notify(value.children[index], "add", index);
                }
            }
        }
    }
    previous_ = values;
}

std::shared_ptr<accessibility_bridge> connect_accessibility(window& owner)
{
    try
    {
        return std::make_shared<atspi_bridge>(owner);
    }
    catch (const std::runtime_error& error)
    {
        event notification{"accessibility_error"};
        notification.text = error.what();
        enqueue(owner, std::move(notification));
        return {};
    }
}
}
