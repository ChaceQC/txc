#include "stdlib/native_gui/platform/linux_accessibility.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
namespace
{
unsigned role_number(const std::string& role)
{
    const std::map<std::string, unsigned> roles{{"application", 75}, {"window", 23}, {"dialog", 16},
        {"button", 43}, {"label", 29}, {"text", 61}, {"check_box", 7}, {"radio_button", 44},
        {"slider", 51}, {"progress", 42}, {"combo_box", 11}, {"list", 31}, {"list_item", 32},
        {"table", 55}, {"tree", 65}, {"tree_item", 91}, {"tabs", 38}, {"tab", 37},
        {"toolbar", 63}, {"status", 54}, {"menu", 33}, {"menu_item", 35}};
    const auto found = roles.find(role);
    return found == roles.end() ? 39 : found->second;
}
}

bool atspi_bridge::accessible_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
    const accessible_tree& values, const std::string& interface_name, const std::string& method)
{
    auto& api = bus->api;
    if (interface_name == "org.a11y.atspi.Accessible")
    {
        if (method == "GetRole")
        {
            tx::ui::bus_check(api.append(reply, "u", role_number(value.role)));
        }
        else if (method == "GetRoleName" || method == "GetLocalizedRoleName")
        {
            tx::ui::bus_check(api.append(reply, "s", value.role.c_str()));
        }
        else if (method == "GetState")
        {
            std::uint64_t bits = 0;
            auto set = [&](unsigned bit, bool enabled)
            {
                if (enabled)
                {
                    bits |= std::uint64_t{1} << bit;
                }
            };
            set(4, value.checked); set(7, value.editable); set(8, value.enabled);
            set(11, value.focusable); set(12, value.focused); set(18, value.multiple);
            set(22, value.selectable); set(23, value.selected); set(24, value.enabled);
            set(25, value.visible); set(30, value.visible); set(38, value.text_capable); set(41, value.checkable);
            tx::ui::bus_check(api.append(reply, "au", 2, static_cast<unsigned>(bits), static_cast<unsigned>(bits >> 32)));
        }
        else if (method == "GetChildren")
        {
            tx::ui::bus_check(api.open_container(reply, 'a', "(so)"));
            for (const auto& id : value.children)
            {
                reference(reply, id);
            }
            tx::ui::bus_check(api.close_container(reply));
        }
        else if (method == "GetChildAtIndex")
        {
            int index = -1;
            tx::ui::bus_check(api.read(request, "i", &index));
            if (index < 0 || static_cast<std::size_t>(index) >= value.children.size())
            {
                throw std::out_of_range("辅助子节点下标越界");
            }
            reference(reply, value.children[index]);
        }
        else if (method == "GetIndexInParent")
        {
            int index = -1;
            if (!value.parent.empty())
            {
                const auto& siblings = values.at(value.parent).children;
                index = std::find(siblings.begin(), siblings.end(), value.id) - siblings.begin();
            }
            tx::ui::bus_check(api.append(reply, "i", index));
        }
        else if (method == "GetApplication")
        {
            reference(reply, "root");
        }
        else if (method == "GetInterfaces")
        {
            tx::ui::bus_check(api.open_container(reply, 'a', "s"));
            for (const auto& name : interfaces(value))
            {
                tx::ui::bus_check(api.append(reply, "s", name.c_str()));
            }
            tx::ui::bus_check(api.close_container(reply));
        }
        else if (method == "GetRelationSet" || method == "GetAttributes")
        {
            tx::ui::bus_check(api.open_container(reply, 'a', method == "GetRelationSet" ? "(ua(so))" : "{ss}"));
            tx::ui::bus_check(api.close_container(reply));
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
