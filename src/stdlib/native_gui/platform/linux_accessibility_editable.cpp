#include "stdlib/native_gui/platform/linux_accessibility.hpp"

#include <algorithm>

namespace tx_generated::native_gui
{
bool atspi_bridge::editable_text_methods(sd_bus_message* request, sd_bus_message* reply, const accessible_node& value,
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
    if (interface_name == "org.a11y.atspi.EditableText")
    {
        accessible_action action{value.id, "value"};
        int begin = 0, end = 0;
        const char* text = nullptr;
        if (method == "SetTextContents")
        {
            tx::ui::bus_check(api.read(request, "s", &text));
            action.text = tx::ui::decode_utf8(text).scalars;
        }
        else if (method == "InsertText")
        {
            int length = 0;
            tx::ui::bus_check(api.read(request, "isi", &begin, &text, &length));
            checked_range(begin, begin);
            const auto inserted = tx::ui::decode_utf8(text).scalars;
            if (length < 0 || static_cast<std::size_t>(length) > inserted.size())
            {
                throw std::out_of_range("辅助插入文本长度越界");
            }
            action.text = value.text;
            action.text.insert(begin, inserted.substr(0, length));
        }
        else if (method == "DeleteText")
        {
            tx::ui::bus_check(api.read(request, "ii", &begin, &end));
            checked_range(begin, end);
            action.text = value.text;
            action.text.erase(begin, end - begin);
        }
        else
        {
            return false;
        }
        tx::ui::bus_check(api.append(reply, "b", int(endpoint->action(action))));
        return true;
    }

    return false;
}
}
