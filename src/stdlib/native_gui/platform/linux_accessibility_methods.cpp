#include "stdlib/native_gui/platform/linux_accessibility.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
bool atspi_bridge::methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
    const accessible_tree& values, const std::string& interface_name, const std::string& method)
{
    auto& api = bus->api;

    if (interface_name == "org.a11y.atspi.Application")
    {
        if (method != "GetLocale" && method != "GetApplicationBusAddress")
        {
            return false;
        }
        tx::ui::bus_check(api.append(reply, "s", method == "GetLocale" ? "zh_CN.UTF-8" : address.c_str()));
        return true;
    }


    return accessible_methods(request, reply, value, values, interface_name, method) ||
        component_methods(request, reply, value, values, interface_name, method) ||
        selection_methods(request, reply, value, values, interface_name, method);
}
}
