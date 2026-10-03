#include "stdlib/native_gui/platform/linux_accessibility.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
bool atspi_bridge::text_geometry_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
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
    if (method == "GetCharacterExtents" || method == "GetRangeExtents")
    {
        int begin = 0, end = 0;
        unsigned coordinates = 0;
        tx::ui::bus_check(api.read(request, "i", &begin));
        if (method == "GetRangeExtents")
        {
            tx::ui::bus_check(api.read(request, "i", &end));
        }
        else
        {
            end = begin + 1;
        }
        tx::ui::bus_check(api.read(request, "u", &coordinates));
        checked_range(begin, end);
        const auto boxes = endpoint->text_bounds(value.id, begin, end);
        rectangle bounds;
        for (const auto& box : boxes)
        {
            if (bounds.width == 0)
            {
                bounds = box;
            }
            else
            {
                const auto x = std::min(bounds.x, box.x), y = std::min(bounds.y, box.y);
                bounds = {x, y, std::max(bounds.x + bounds.width, box.x + box.width) - x,
                    std::max(bounds.y + bounds.height, box.y + box.height) - y};
            }
        }
        if (coordinates != 0)
        {
            const auto values = tree();
            const auto origin = values.at(coordinates == 2 ? value.parent : endpoint->root_id).bounds;
            bounds.x -= origin.x;
            bounds.y -= origin.y;
        }
        tx::ui::bus_check(api.append(reply, "iiii", int(bounds.x), int(bounds.y), int(bounds.width), int(bounds.height)));
    }
    else if (method == "GetOffsetAtPoint")
    {
        int x = 0, y = 0;
        unsigned coordinates = 0;
        tx::ui::bus_check(api.read(request, "iiu", &x, &y, &coordinates));
        auto point = tx::ui::point{double(x), double(y)};
        if (coordinates != 0)
        {
            const auto values = tree();
            const auto origin = values.at(coordinates == 2 ? value.parent : endpoint->root_id).bounds;
            point.x += origin.x;
            point.y += origin.y;
        }
        tx::ui::bus_check(api.append(reply, "i", int(endpoint->text_at_point(value.id, point))));
    }
    else
    {
        return false;
    }
    return true;
}
}
