#include "stdlib/native_gui/platform/linux_accessibility.hpp"

namespace tx_generated::native_gui
{
bool atspi_bridge::selection_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
    const accessible_tree& values, const std::string& interface_name, const std::string& method)
{
    auto& api = bus->api;
    if (interface_name == "org.a11y.atspi.Action")
    {
        if (method == "GetActions")
        {
            tx::ui::bus_check(api.append(reply, "a(sss)", 1, value.selectable ? "select" : "activate", "", ""));
            return true;
        }
        int index = -1;
        tx::ui::bus_check(api.read(request, "i", &index));
        if (index != 0)
        {
            throw std::out_of_range("辅助操作下标越界");
        }
        if (method == "DoAction")
        {
            tx::ui::bus_check(api.append(reply, "b", int(endpoint->action({value.id, value.selectable ? "select" : "invoke"}))));
        }
        else
        {
            tx::ui::bus_check(api.append(reply, "s", method == "GetKeyBinding" ? "" : value.selectable ? "select" : "activate"));
        }
        return true;
    }
    if (interface_name != "org.a11y.atspi.Selection")
    {
        return false;
    }
    std::vector<std::string> selected;
    for (const auto& id : value.children)
    {
        if (values.at(id).selected)
        {
            selected.push_back(id);
        }
    }
    if (method == "ClearSelection" || method == "SelectAll")
    {
        bool success = method == "ClearSelection" || value.multiple;
        if (success)
        {
            for (const auto& id : method == "ClearSelection" ? selected : value.children)
            {
                success = endpoint->action({id, method == "ClearSelection" ? "remove_selection" : "add_selection"}) && success;
            }
        }
        tx::ui::bus_check(api.append(reply, "b", int(success)));
        return true;
    }
    int index = -1;
    tx::ui::bus_check(api.read(request, "i", &index));
    const auto& candidates = method == "GetSelectedChild" || method == "DeselectSelectedChild" ? selected : value.children;
    if (index < 0 || static_cast<std::size_t>(index) >= candidates.size())
    {
        throw std::out_of_range("辅助选区下标越界");
    }
    const auto& id = candidates[index];
    if (method == "GetSelectedChild")
    {
        reference(reply, id);
    }
    else
    {
        const bool success = method == "IsChildSelected" ? values.at(id).selected :
            endpoint->action({id, method == "SelectChild" ? "add_selection" : "remove_selection"});
        tx::ui::bus_check(api.append(reply, "b", int(success)));
    }
    return true;
}
}
