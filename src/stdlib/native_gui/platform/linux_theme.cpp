#include "stdlib/native_gui/theme.hpp"
#include "stdlib/native_gui/platform/linux_bus.hpp"

namespace tx_generated::native_gui
{
system_theme read_system_theme()
{
    system_theme result;
    try
    {
        tx::ui::system_bus bus;
        auto read = [&](const char* key)
        {
            tx::ui::bus_message reply{bus.api};
            tx::ui::bus_check(bus.api.call_method(bus.value, "org.freedesktop.portal.Desktop",
                "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Settings", "ReadOne",
                nullptr, &reply.value, "ss", "org.freedesktop.appearance", key));
            tx::ui::bus_check(bus.api.enter_container(reply.value, 'v', "u"));
            unsigned value = 0;
            tx::ui::bus_check(bus.api.read(reply.value, "u", &value));
            return value;
        };
        result.dark = read("color-scheme") == 1;
        try
        {
            result.high_contrast = read("contrast") == 1;
        }
        catch (const std::runtime_error&)
        {
        }
    }
    catch (const std::runtime_error&)
    {
        // 无桌面设置服务（例如无桌面会话）时使用可预测的浅色默认值。
    }
    return result;
}
}
