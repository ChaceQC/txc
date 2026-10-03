#include "stdlib/native_gui/platform/linux_accessibility.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
bool atspi_bridge::property(sd_bus_message* reply, const accessible_node& value, const accessible_tree& values,
    const std::string& interface_name, const std::string& key, bool dictionary)
{
    auto& api = bus->api;
    std::string type;
    std::string text;
    std::int32_t integer = 0;
    double number = 0;
    if (interface_name == "org.a11y.atspi.Accessible")
    {
        if (key == "Name" || key == "Description" || key == "Locale" || key == "AccessibleId" || key == "HelpText")
        {
            type = "s";
            text = key == "Name" ? value.name : key == "Locale" ? "zh_CN.UTF-8" : key == "AccessibleId" ? value.id : value.help;
        }
        else if (key == "ChildCount")
        {
            type = "i";
            integer = static_cast<std::int32_t>(value.children.size());
        }
        else if (key == "Parent")
        {
            type = "(so)";
        }
    }
    else if (interface_name == "org.a11y.atspi.Application")
    {
        if (key == "ToolkitName" || key == "Version" || key == "ToolkitVersion" || key == "AtspiVersion")
        {
            type = "s";
            text = key == "ToolkitName" ? "TX Native GUI" : key == "AtspiVersion" ? "2.1" : "1.0";
        }
        else if (key == "Id")
        {
            type = "i";
            integer = application_id;
        }
    }
    else if (interface_name == "org.a11y.atspi.Action" && key == "NActions")
    {
        type = "i";
        integer = 1;
    }
    else if (interface_name == "org.a11y.atspi.Value")
    {
        if (key == "Text")
        {
            type = "s";
            text = std::to_string(value.value);
        }
        else if (key == "MinimumValue" || key == "MaximumValue" || key == "CurrentValue" || key == "MinimumIncrement")
        {
            type = "d";
            number = key == "MinimumValue" ? value.minimum : key == "MaximumValue" ? value.maximum : key == "MinimumIncrement" ? value.step : value.value;
        }
    }
    else if (interface_name == "org.a11y.atspi.Text" && (key == "CharacterCount" || key == "CaretOffset"))
    {
        type = "i";
        integer = key == "CharacterCount" ? static_cast<int>(value.text.size()) : static_cast<int>(value.caret);
    }
    else if (interface_name == "org.a11y.atspi.Selection" && key == "NSelectedChildren")
    {
        type = "i";
        for (const auto& id : value.children)
        {
            integer += values.at(id).selected;
        }
    }
    if (key == "version" || key == "InterfaceVersion")
    {
        type = "u";
        integer = 1;
    }
    if (type.empty())
    {
        return false;
    }
    if (dictionary)
    {
        tx::ui::bus_check(api.open_container(reply, 'e', "sv"));
        tx::ui::bus_check(api.append(reply, "s", key.c_str()));
    }
    tx::ui::bus_check(api.open_container(reply, 'v', type.c_str()));
    if (type == "s")
    {
        tx::ui::bus_check(api.append(reply, "s", text.c_str()));
    }
    else if (type == "d")
    {
        tx::ui::bus_check(api.append(reply, "d", number));
    }
    else if (type == "(so)")
    {
        if (value.id == "root" && !desktop_name.empty())
        {
            tx::ui::bus_check(api.append(reply, "(so)", desktop_name.c_str(), desktop_path.c_str()));
        }
        else
        {
            reference(reply, value.parent);
        }
    }
    else
    {
        tx::ui::bus_check(api.append(reply, type.c_str(), integer));
    }
    tx::ui::bus_check(api.close_container(reply));
    if (dictionary)
    {
        tx::ui::bus_check(api.close_container(reply));
    }
    return true;
}

bool atspi_bridge::properties(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
    const accessible_tree& values, const std::string& method)
{
    auto& api = bus->api;
    const char* interface_name = nullptr;
    const char* key = nullptr;
    tx::ui::bus_check(api.read(request, "s", &interface_name));
    const auto supported = interfaces(value);
    if (std::find(supported.begin(), supported.end(), interface_name) == supported.end())
    {
        return false;
    }
    if (method == "GetAll")
    {
        tx::ui::bus_check(api.open_container(reply, 'a', "{sv}"));
        for (const auto* name : {"Name", "Description", "Parent", "ChildCount", "Locale", "AccessibleId", "HelpText", "version",
            "ToolkitName", "Version", "ToolkitVersion", "AtspiVersion", "Id", "InterfaceVersion", "NActions",
            "MinimumValue", "MaximumValue", "MinimumIncrement", "CurrentValue", "Text", "CharacterCount", "CaretOffset", "NSelectedChildren"})
        {
            property(reply, value, values, interface_name, name, true);
        }
        tx::ui::bus_check(api.close_container(reply));
        return true;
    }
    tx::ui::bus_check(api.read(request, "s", &key));
    if (method == "Get")
    {
        return property(reply, value, values, interface_name, key, false);
    }
    if (method == "Set" && std::string_view(interface_name) == "org.a11y.atspi.Application" && std::string_view(key) == "Id")
    {
        tx::ui::bus_check(api.enter_container(request, 'v', "i"));
        tx::ui::bus_check(api.read(request, "i", &application_id));
        return true;
    }
    if (method == "Set" && value.range && std::string_view(interface_name) == "org.a11y.atspi.Value" && std::string_view(key) == "CurrentValue")
    {
        accessible_action action{value.id, "range"};
        tx::ui::bus_check(api.enter_container(request, 'v', "d"));
        tx::ui::bus_check(api.read(request, "d", &action.value));
        if (!endpoint->action(action))
        {
            throw std::invalid_argument("范围值不可写或超出范围");
        }
        return true;
    }
    return false;
}
}
