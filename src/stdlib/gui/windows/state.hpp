#pragma once

#include "stdlib/graphics/windows/state.hpp"
#include "stdlib/gui/layout.hpp"

#include <commctrl.h>

namespace tx_generated::gui
{

constexpr std::size_t node_limit = 4096;
constexpr unsigned depth_limit = 64;
constexpr std::size_t track_limit = 128;
using graphics::fail;
using graphics::platform_fail;

struct node : graphics::resource, std::enable_shared_from_this<node>
{
    explicit node(tx::graphics_kind kind) : resource(kind)
    {
    }
    std::weak_ptr<graphics::window> window;
    std::weak_ptr<node> parent;
    std::vector<std::shared_ptr<node>> children;
    DWORD thread = GetCurrentThreadId();
    HWND hwnd = nullptr;
    std::int64_t id = 0;
    std::int64_t revision = 0;
    unsigned depth = 0;
    bool closed = false;
    bool visible = true;
    bool enabled = true;
    bool reserved = false;
    bool multiline = false;
    bool three_state = false;
    bool password = false;
    bool composing = false;
    unsigned suppress = 0;
    int check = 0;
    std::int64_t text_limit = 16384;
    std::string text;
    layout_mode layout = layout_mode::column;
    length width{length_mode::stretch, 1};
    length height;
    bool explicit_width = false;
    extent minimum;
    extent maximum{16384, 16384};
    double margin = 0;
    double padding = 0;
    double gap = 0;
    alignment horizontal = alignment::stretch;
    alignment vertical = alignment::stretch;
    std::vector<length> rows{length{}};
    std::vector<length> columns{length{length_mode::stretch, 1}};
    std::size_t row = 0;
    std::size_t column = 0;
    std::size_t row_span = 1;
    std::size_t column_span = 1;
    extent natural;
    bounds arranged;
    // 根独占字体，显式 UI 线程关闭时释放；别名析构不调用 GDI。
    HFONT font = nullptr;
    UINT font_dpi = 0;
    bool dirty = true;
    bool arranging = false;
    unsigned notifying = 0;
    std::weak_ptr<node> default_button;
    std::weak_ptr<node> cancel_button;
};

node& require_node(graphics::resource* value, bool allow_closed = false);
graphics::window& owner_window(node& state);
node& root_node(node& state);
void dirty(node& state);
void require_idle(node& state);
std::shared_ptr<node> root(graphics::window& window);
std::shared_ptr<node> create(node& parent, tx::graphics_kind kind,
    const std::string& text = {}, bool option = false);
void close(node& state) noexcept;
void close_root(graphics::window& window) noexcept;
void window_changed(graphics::window& window, bool font_changed = false) noexcept;
void flush_layout(graphics::window& window);
void flush_all(graphics::app& app);
void layout_tree(node& root, double width, double height);
void update_font(node& root);
void notify(node& state, const char* action, bool with_text = false, bool with_state = false);
void process_command(HWND child, unsigned code);
bool translate_message(graphics::app& app, const MSG& message);
void focus(node& state);
void advance_focus(node& root, node* excluded, bool reverse = false);
void set_visible(node& state, bool value);
void set_enabled(node& state, bool value);
void set_text(node& state, const std::string& text);
std::wstring wide_text(const std::string& text);
std::string read_text(node& state);
std::vector<DWORD> scalar_offsets(const std::wstring& text);
void set_selection(node& state, std::int64_t start, std::int64_t end);
std::int64_t selection(node& state, bool end);
void set_check(node& state, int value);
void set_password(node& state, bool value);
length checked_length(const std::string& mode, double value);
double checked_dimension(double value);
alignment checked_alignment(const std::string& value);
void set_layout(node& state, layout_mode mode, double padding, double gap,
    std::int64_t rows = 1, std::int64_t columns = 1);
void set_cell(node& state, std::int64_t row, std::int64_t column,
    std::int64_t row_span, std::int64_t column_span);
LRESULT CALLBACK control_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
    UINT_PTR subclass_id, DWORD_PTR reference) noexcept;

struct notification_guard
{
    explicit notification_guard(node& value) : root(root_node(value))
    {
        ++root.notifying;
    }
    ~notification_guard()
    {
        --root.notifying;
    }
    node& root;
};

} // namespace tx_generated::gui
