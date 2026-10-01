#pragma once

#include "stdlib/gui/windows/state.hpp"
#include "stdlib/graphics/windows/assets.hpp"

namespace tx_generated::gui
{

struct chord
{
    UINT key = 0;
    bool ctrl = false, alt = false, shift = false, win = false;
    bool operator==(const chord&) const = default;
};

struct command : graphics::owned_resource
{
    command() : owned_resource(tx::graphics_kind::command)
    {
    }
    std::int64_t id = 0, revision = 0;
    UINT native_id = 0;
    std::string text;
    bool enabled = true, checked = false;
    std::optional<chord> shortcut;
    void release_native() noexcept override;
};

struct menu : graphics::owned_resource, std::enable_shared_from_this<menu>
{
    menu() : owned_resource(tx::graphics_kind::menu)
    {
    }
    HMENU handle = nullptr;
    std::weak_ptr<menu> parent;
    std::vector<std::shared_ptr<menu>> children;
    std::vector<std::shared_ptr<command>> commands;
    bool attached = false;
    void release_native() noexcept override;
};

command& require_command(graphics::resource* value);
menu& require_menu(graphics::resource* value);
std::shared_ptr<command> create_command(graphics::window& window, const std::string& text);
std::shared_ptr<menu> create_menu(graphics::window& window, menu* parent = nullptr, const std::string& text = {});
void bind_button(node& button, command& value);
void add_command(menu& target, command& value);
void add_tool(node& toolbar, command& value);
void set_command(command& value, const std::string& text, bool enabled, bool checked);
void set_shortcut(command& value, chord shortcut);
bool activate_command(graphics::window& window, UINT native_id, std::int64_t source_id = 0);
bool translate_shortcut(graphics::app& app, const MSG& message);
void show_modal(graphics::window& child, graphics::window& owner);
void close_interactions(graphics::window& state) noexcept;
void require_system_idle(graphics::window& window);

} // namespace tx_generated::gui
