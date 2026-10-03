#include "stdlib/native_gui/platform/linux_accessibility.hpp"

namespace tx_generated::native_gui
{
bool atspi_bridge::component_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
    const accessible_tree& values, const std::string& interface_name, const std::string& method)
{
    auto& api = bus->api;
    if (interface_name == "org.a11y.atspi.Component")
    {
        auto bounds = value.bounds;
        unsigned coordinates = 0;
        int x = 0, y = 0;
        if (method == "Contains" || method == "GetAccessibleAtPoint")
        {
            tx::ui::bus_check(api.read(request, "iiu", &x, &y, &coordinates));
        }
        else if (method == "GetExtents" || method == "GetPosition")
        {
            tx::ui::bus_check(api.read(request, "u", &coordinates));
        }
        const auto origin = coordinates == 1 ? values.at(endpoint->root_id).bounds :
            coordinates == 2 && !value.parent.empty() ? values.at(value.parent).bounds : rectangle{};
        bounds.x -= origin.x;
        bounds.y -= origin.y;
        if (method == "GetExtents")
        {
            tx::ui::bus_check(api.append(reply, "(iiii)", int(bounds.x), int(bounds.y), int(bounds.width), int(bounds.height)));
        }
        else if (method == "GetPosition" || method == "GetSize")
        {
            tx::ui::bus_check(api.append(reply, "ii", int(method == "GetSize" ? bounds.width : bounds.x), int(method == "GetSize" ? bounds.height : bounds.y)));
        }
        else if (method == "Contains")
        {
            tx::ui::bus_check(api.append(reply, "b", int(value.visible && bounds.contains({double(x), double(y)}))));
        }
        else if (method == "GetAccessibleAtPoint")
        {
            std::string selected;
            for (const auto& id : value.children)
            {
                const auto& child = values.at(id);
                if (child.visible && child.bounds.contains({x + origin.x, y + origin.y}))
                {
                    selected = id;
                }
            }
            reference(reply, selected);
        }
        else if (method == "GrabFocus")
        {
            tx::ui::bus_check(api.append(reply, "b", int(endpoint->action({value.id, "focus"}))));
        }
        else if (method == "GetLayer")
        {
            tx::ui::bus_check(api.append(reply, "u", 3u));
        }
        else if (method == "GetMDIZOrder")
        {
            tx::ui::bus_check(api.append(reply, "n", 0));
        }
        else if (method == "GetAlpha")
        {
            tx::ui::bus_check(api.append(reply, "d", 1.0));
        }
        else if (method == "SetExtents" || method == "SetPosition" || method == "SetSize")
        {
            tx::ui::bus_check(api.append(reply, "b", 0));
        }
        else
        {
            return false;
        }
        return true;
    }
    return false;
}
}
