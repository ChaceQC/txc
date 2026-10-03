#pragma once

#include "stdlib/native_gui/state.hpp"

namespace tx_generated::native_gui
{
void show_modal(window& dialog, window& owner);
void restore_modal(window& dialog) noexcept;
void end_dialog(window& dialog, std::int64_t result);
std::string message_box(window& owner, const std::string& title, const std::string& text, const std::string& buttons);
std::optional<std::string> file_dialog(window& owner, const std::string& kind,
    const std::string& title, const std::string& initial);
// 平台窗口的所有权和输入屏蔽不包含控件逻辑。
void platform_modal(window& dialog, window& owner, bool enabled);
tx::ui::point screen_origin(window& owner);
}
