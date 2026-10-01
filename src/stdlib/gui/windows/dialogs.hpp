#pragma once

#include "stdlib/gui/windows/commands.hpp"

namespace tx_generated::gui
{

struct file_filter
{
    std::wstring name;
    std::wstring pattern;
};
enum class file_dialog_kind
{
    open, multiple, save, folder
};
std::optional<std::vector<std::string>> file_dialog(graphics::window& owner, file_dialog_kind kind,
    const std::string& title, const std::vector<file_filter>& filters, const std::string& extension = {});
std::string message_box(graphics::window& owner, const std::string& title,
    const std::string& text, const std::string& buttons);
std::optional<std::string> clipboard_text(graphics::window& owner);
void set_clipboard_text(graphics::window& owner, const std::string& text);

} // namespace tx_generated::gui
