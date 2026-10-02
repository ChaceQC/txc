#pragma once

#include "stdlib/graphics/resource.hpp"
#include "stdlib/error.hpp"
#include "stdlib/native_gui/core/text_layout.hpp"
#include "stdlib/native_gui/core/text_buffer.hpp"
#include "stdlib/native_gui/platform/window.hpp"
#include "stdlib/gui/layout.hpp"

#include <deque>
#include <thread>

namespace tx_generated::native_gui
{
using rectangle = tx::ui::rect;
enum class layout_mode
{
    column, row, grid, overlay
};
struct node;
struct app;
struct window;

struct event
{
    event() = default;
    event(std::string value) : kind(std::move(value))
    {
    }
    std::string kind;
    std::int64_t window_id = 0, source_id = 0;
    std::string text;
    double number = 0;
    bool state = false;
    std::int64_t revision = 0;
};

struct app : graphics::resource, std::enable_shared_from_this<app>
{
    app() : resource(tx::graphics_kind::native_app)
    {
    }
    std::thread::id thread = std::this_thread::get_id();
    bool open = true;
    std::int64_t next_id = 1;
    std::shared_ptr<tx::ui::font_face> font;
    std::vector<std::shared_ptr<window>> windows;
    std::deque<event> events;
};

struct window : graphics::resource, std::enable_shared_from_this<window>
{
    window() : resource(tx::graphics_kind::native_window)
    {
    }
    std::weak_ptr<app> owner;
    std::unique_ptr<tx::ui::platform_window> host;
    std::shared_ptr<node> native_gui_root;
    std::int64_t id = 0;
    unsigned width = 0, height = 0;
    double dpi = 96;
    bool closed = false, visible = false, minimized = false, repaint = true;
};

struct node : graphics::resource, std::enable_shared_from_this<node>
{
    explicit node(tx::graphics_kind kind) : resource(kind)
    {
    }
    std::weak_ptr<native_gui::window> window;
    std::weak_ptr<node> parent;
    std::vector<std::shared_ptr<node>> children;
    std::thread::id thread = std::this_thread::get_id();
    std::int64_t id = 0, revision = 0;
    unsigned depth = 0;
    bool closed = false, visible = true, enabled = true, checked = false;
    bool dirty = true, dark = false;
    layout_mode layout = layout_mode::column;
    gui::length width, height;
    double min_width = 0, min_height = 0, max_width = 16384, max_height = 16384;
    double margin = 0, padding = 12, gap = 8;
    gui::alignment horizontal = gui::alignment::stretch, vertical = gui::alignment::start;
    std::vector<gui::length> rows, columns;
    std::size_t row = 0, column = 0, row_span = 1, column_span = 1;
    rectangle bounds{}, clip{};
    std::u32string text;
    std::unique_ptr<tx::ui::text_buffer> editor;
    std::u32string composition;
    std::vector<std::uint32_t> composition_feedback;
    std::size_t composition_caret = 0, composition_selection = 0, composition_length = 0;
    bool composing = false, password = false;
    double text_scroll_x = 0, text_scroll_y = 0;
    std::optional<double> preferred_x;
    double minimum = 0, maximum = 100, value = 0, step = 1;
    bool indeterminate = false;
    std::int64_t radio_group = 0;
    std::unique_ptr<tx::ui::text_layout> text_layout;
    double text_width = -1;
    std::shared_ptr<tx::ui::font_face> font;
    std::weak_ptr<node> hovered, pressed, focused;
    bool keyboard_pressed = false, window_focused = false;
    bool ime_warning_sent = false;
    double layout_width = -1, layout_height = -1;
};

[[noreturn]] void fail(const char* code, const std::string& message);
void require_thread(std::thread::id thread);
app& require_app(graphics::resource* value, bool allow_closed = false);
window& require_window(graphics::resource* value, bool allow_closed = false);
node& require_node(graphics::resource* value, bool allow_closed = false);
window& owner_window(node& state);
node& root_node(node& state);
void require_idle(node& state);
double checked_size(double value);
gui::length checked_length(const std::string& mode, double value);
bool interactive(const node& state);
bool available(const node& state);
void dirty(node& state);
std::shared_ptr<app> open_app(const std::string& font_path);
std::shared_ptr<window> create_window(app& app, const std::string& title, double width, double height);
void close_app(app& app) noexcept;
void close_window(window& window) noexcept;
void enqueue(window& window, event event);
std::u32string clipboard_text(window& window);
std::optional<event> next_event(app& app, std::int64_t timeout_ms);
std::shared_ptr<node> root(window& window);
std::shared_ptr<node> create(node& parent, tx::graphics_kind kind, const std::string& text);
void close(node& state);
void close_root(window& window) noexcept;
void set_text(node& state, const std::string& text);
void set_visible(node& state, bool value);
void set_enabled(node& state, bool value);
void reset_interaction(node& root) noexcept;
void layout(node& root);
void ensure_text(node& state, double width);
bool contains(rectangle bounds, double x, double y);
void process_event(window& window, const tx::ui::window_event& event);
void activate(node& state);
void slider_pointer(node& state, double x);
void slider_commit(node& state);
bool slider_key(node& state, const tx::ui::window_event& event);
bool radio_key(node& state, const tx::ui::window_event& event);
void focus_node(node& root, const std::shared_ptr<node>& target);
bool text_key(node& state, const tx::ui::window_event& event);
void text_input(node& state, const tx::ui::window_event& event);
void text_pointer(node& state, double x, double y, bool extend);
void refresh_editor(node& state, bool notify);
void selection_changed(node& state);
void update_editor_viewport(node& state);
std::u32string display_text(const node& state);
void draw_editor(node& state, tx::ui::rasterizer& painter, tx::ui::color foreground, tx::ui::color accent);
tx::ui::pixel_buffer render(node& root);
void paint(node& root);
}
