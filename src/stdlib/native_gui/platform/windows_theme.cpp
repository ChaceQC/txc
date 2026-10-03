#include "stdlib/native_gui/platform/windows_window.hpp"
#include "stdlib/native_gui/theme.hpp"

namespace tx_generated::native_gui
{
system_theme read_system_theme()
{
    system_theme result;
    HIGHCONTRASTW contrast{};
    contrast.cbSize = sizeof(contrast);
    result.high_contrast = SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) &&
        (contrast.dwFlags & HCF_HIGHCONTRASTON);
    DWORD light = 1, size = sizeof(light);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size);
    result.dark = !light;
    if (result.high_contrast)
    {
        auto color = [](int index) -> tx::ui::color
        {
            const auto value = GetSysColor(index);
            return {GetRValue(value), GetGValue(value), GetBValue(value), 255};
        };
        result.background = color(COLOR_WINDOW);
        result.foreground = color(COLOR_WINDOWTEXT);
        result.accent = color(COLOR_HIGHLIGHT);
        result.muted = color(COLOR_GRAYTEXT);
    }
    return result;
}
}
