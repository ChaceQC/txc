#include "stdlib/native_gui/platform/linux_accessibility.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
bool atspi_bridge::text_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
    const std::string& interface_name, const std::string& method)
{
    auto& api = bus->api;
    if (interface_name != "org.a11y.atspi.Text" && interface_name != "org.a11y.atspi.EditableText")
    {
        return false;
    }
    auto checked_range = [&](int begin, int end)
    {
        if (begin < 0 || end < begin || static_cast<std::size_t>(end) > value.text.size())
        {
            throw std::out_of_range("辅助文本范围越界");
        }
    };
    if (editable_text_methods(request, reply, value, interface_name, method) ||
        text_selection_methods(request, reply, value, interface_name, method))
    {
        return true;
    }
    if (method == "GetText" || method == "GetCharacterAtOffset")
    {
        int begin = 0, end = 0;
        tx::ui::bus_check(api.read(request, method == "GetText" ? "ii" : "i", &begin, &end));
        end = method == "GetCharacterAtOffset" ? begin + 1 : end < 0 ? static_cast<int>(value.text.size()) : end;
        checked_range(begin, end);
        if (method == "GetCharacterAtOffset")
        {
            tx::ui::bus_check(api.append(reply, "i", static_cast<int>(value.text[begin])));
        }
        else
        {
            const auto text = tx::ui::encode_utf8(std::u32string_view(value.text).substr(begin, end - begin));
            tx::ui::bus_check(api.append(reply, "s", text.c_str()));
        }
    }
    else if (method == "GetStringAtOffset" || method == "GetTextAtOffset" || method == "GetTextBeforeOffset" || method == "GetTextAfterOffset")
    {
        int offset = 0;
        unsigned unit = 0;
        tx::ui::bus_check(api.read(request, "iu", &offset, &unit));
        checked_range(offset, offset);
        unsigned semantic_unit = unit == 0 ? 0 : unit == 1 ? 1 : unit == 4 ? 3 : 2;
        if (method != "GetStringAtOffset")
        {
            semantic_unit = unit == 0 ? 0 : unit <= 2 ? 1 : unit <= 4 ? 3 : 2;
        }
        const auto stops = endpoint->text_units(value.id, semantic_unit);
        auto index = static_cast<std::ptrdiff_t>(std::upper_bound(stops.begin(), stops.end(), offset) - stops.begin()) - 1;
        index += method == "GetTextBeforeOffset" ? -1 : method == "GetTextAfterOffset" ? 1 : 0;
        index = std::clamp(index, std::ptrdiff_t{0}, static_cast<std::ptrdiff_t>(stops.size() - 1));
        const auto begin = stops[index], end = index + 1 < static_cast<std::ptrdiff_t>(stops.size()) ? stops[index + 1] : begin;
        const auto text = tx::ui::encode_utf8(std::u32string_view(value.text).substr(begin, end - begin));
        tx::ui::bus_check(api.append(reply, "sii", text.c_str(), int(begin), int(end)));
    }
    else if (method == "GetDefaultAttributes" || method == "GetDefaultAttributeSet" || method == "GetAttributes" || method == "GetAttributeRun")
    {
        tx::ui::bus_check(api.append(reply, "a{ss}", 1, "editable", value.editable ? "true" : "false"));
        if (method == "GetAttributes" || method == "GetAttributeRun")
        {
            tx::ui::bus_check(api.append(reply, "ii", 0, int(value.text.size())));
        }
    }
    else if (method == "GetAttributeValue")
    {
        int offset = 0;
        const char* key = nullptr;
        tx::ui::bus_check(api.read(request, "is", &offset, &key));
        checked_range(offset, offset);
        tx::ui::bus_check(api.append(reply, "s", std::string_view(key) == "editable" ? (value.editable ? "true" : "false") : ""));
    }
    else
    {
        return text_geometry_methods(request, reply, value, interface_name, method);
    }
    return true;
}
}
