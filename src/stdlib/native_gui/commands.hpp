#pragma once

#include "stdlib/native_gui/state.hpp"

namespace tx_generated::native_gui
{
struct shortcut
{
    std::string key;
    bool ctrl = false, shift = false, alt = false, meta = false;
    bool operator==(const shortcut&) const = default;
};

struct command : graphics::resource, std::enable_shared_from_this<command>
{
    command() : resource(tx::graphics_kind::native_command)
    {
    }
    std::weak_ptr<window> owner;
    std::int64_t id = 0;
    std::string text, accelerator;
    shortcut keys;
    bool enabled = true, checked = false, checkable = false, closed = false;
    std::vector<std::weak_ptr<node>> bindings;
};

struct menu_item
{
    std::string text;
    std::shared_ptr<command> action;
    std::shared_ptr<menu> submenu;
    std::int64_t id = 0;
};

struct menu : graphics::resource, std::enable_shared_from_this<menu>
{
    menu() : resource(tx::graphics_kind::native_menu)
    {
    }
    std::weak_ptr<window> owner;
    std::int64_t id = 0;
    bool closed = false;
    std::vector<menu_item> items;
};

struct menu_level
{
    std::shared_ptr<menu> model;
    rectangle bounds;
    int selected = -1, first = 0;
};

struct menu_session
{
    std::vector<menu_level> levels;
    std::weak_ptr<node> restore_focus;
    int bar_index = -1;
    bool pressed = false;
};

command& require_command(graphics::resource* value, bool allow_closed = false);
menu& require_menu(graphics::resource* value, bool allow_closed = false);
std::shared_ptr<command> create_command(window& owner, const std::string& text,
    const std::string& accelerator, bool checkable);
std::shared_ptr<menu> create_menu(window& owner);
void set_shortcut(command& state, const std::string& accelerator);
void update_command(command& state);
void close_command(command& state);
void invoke_command(command& state);
void bind_command(node& state, command& action);
bool command_key(window& owner, const tx::ui::window_event& input);
void append_menu(menu& state, std::string text, std::shared_ptr<command> action, std::shared_ptr<menu> submenu);
void close_menu(window& owner) noexcept;
void show_menu(menu& state, double x, double y);
bool menu_input(window& owner, const tx::ui::window_event& input);
double menu_bar_height(const window& owner);
void draw_menus(window& owner, tx::ui::rasterizer& painter, const palette& theme);
bool menu_item_enabled(const menu_item& item);
void open_submenu(window& owner, std::size_t level);
}
