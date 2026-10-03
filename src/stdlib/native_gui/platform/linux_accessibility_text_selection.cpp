#include "stdlib/native_gui/platform/linux_accessibility.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
bool atspi_bridge::text_selection_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
    const std::string& interface_name, const std::string& method)
{
    auto& api = bus->api;
    auto checked_range = [&](int begin, int end)
    {
        if (begin < 0 || end < begin || static_cast<std::size_t>(end) > value.text.size())
        {
            throw std::out_of_range("辅助文本范围越界");
        }
    };
    if (method == "GetNSelections")
    {
        tx::ui::bus_check(api.append(reply, "i", int(value.selection_start != value.selection_end)));
    }
    else if (method == "GetSelection")
    {
        int index = 0;
        tx::ui::bus_check(api.read(request, "i", &index));
        if (index != 0)
        {
            throw std::out_of_range("文本选区下标越界");
        }
        tx::ui::bus_check(api.append(reply, "ii", int(value.selection_start), int(value.selection_end)));
    }
    else if (method == "SetCaretOffset" || method == "SetSelection" || method == "AddSelection" || method == "RemoveSelection")
    {
        int index = 0, begin = 0, end = 0;
        if (method == "SetSelection" || method == "RemoveSelection")
        {
            tx::ui::bus_check(api.read(request, "i", &index));
        }
        if (method == "RemoveSelection")
        {
            begin = end = value.caret;
        }
        else if (method == "SetCaretOffset")
        {
            tx::ui::bus_check(api.read(request, "i", &begin));
            end = begin;
        }
        else
        {
            tx::ui::bus_check(api.read(request, "ii", &begin, &end));
        }
        checked_range(begin, end);
        accessible_action action{value.id, "text_selection"};
        action.start = begin;
        action.end = end;
        tx::ui::bus_check(api.append(reply, "b", int(index == 0 && endpoint->action(action))));
    }
    else
    {
        return false;
    }
    return true;
}
}
