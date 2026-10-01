#pragma once

#include "stdlib/graphics/resource.hpp"
#include "stdlib/error.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d2d1.h>
#include <chrono>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace tx_generated::gui
{
struct node;
}

namespace tx_generated::graphics
{

template<class value_type>
struct com_release
{
    void operator()(value_type* value) const noexcept
    {
        if (value)
        {
            value->Release();
        }
    }
};
template<class value_type>
using com_ptr = std::unique_ptr<value_type, com_release<value_type>>;

constexpr std::size_t window_limit = 64;
constexpr std::size_t event_limit = 4096;
constexpr UINT pixel_limit = 16384;

struct size_event
{
    double width = 0;
    double height = 0;
    std::int64_t pixel_width = 0;
    std::int64_t pixel_height = 0;
    std::int64_t dpi = 96;
};
struct event
{
    std::string kind;
    std::int64_t window_id = 0;
    std::int64_t timestamp_ms = 0;
    std::optional<size_event> resize;
    struct control_data
    {
        std::int64_t source_id = 0;
        std::string action;
        std::optional<std::string> text;
        std::optional<bool> state;
        std::int64_t revision = 0;
    };
    std::optional<control_data> control;
};

struct window;
struct canvas;
struct app : resource, std::enable_shared_from_this<app>
{
    app() : resource(tx::graphics_kind::app)
    {
    }
    DWORD thread = GetCurrentThreadId();
    bool open = true;
    bool com_initialized = false;
    bool queue_failed = false;
    bool draining = false;
    std::int64_t next_id = 1;
    std::int64_t native_error = 0;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    com_ptr<ID2D1Factory> factory;
    std::vector<std::shared_ptr<window>> windows;
    std::shared_ptr<canvas> frame;
    std::deque<event> events;
    std::optional<error_info> pending_error;
};

struct window : resource, std::enable_shared_from_this<window>
{
    window() : resource(tx::graphics_kind::window)
    {
    }
    std::weak_ptr<app> owner;
    DWORD thread = GetCurrentThreadId();
    HWND hwnd = nullptr;
    std::int64_t id = 0;
    UINT dpi = 96;
    UINT width = 0;
    UINT height = 0;
    bool visible = false;
    bool minimized = false;
    bool closed = false;
    bool software = false;
    bool changing_dpi = false;
    com_ptr<ID2D1HwndRenderTarget> target;
    std::shared_ptr<gui::node> gui_root;
};

struct canvas : resource
{
    canvas() : resource(tx::graphics_kind::canvas)
    {
    }
    std::weak_ptr<app> owner;
    std::shared_ptr<window> target_window;
    DWORD thread = GetCurrentThreadId();
    bool active = false;
    com_ptr<ID2D1BitmapRenderTarget> target;
    com_ptr<ID2D1SolidColorBrush> brush;
};

[[noreturn]] void fail(const char* code, const std::string& message);
void record_native_error(app* state, std::int64_t code) noexcept;
[[noreturn]] void platform_fail(app* state, const char* operation, std::int64_t code);
void require_thread(DWORD thread);
app& require_app(resource* value);
window& require_window(resource* value, bool allow_closed = false);
canvas& require_canvas(resource* value);
void drain(app& state);
void close_app(app& state) noexcept;
void close_window(window& state) noexcept;
void cancel_canvas(canvas& state) noexcept;
void enqueue(window& state, const char* kind, bool with_size = false) noexcept;
void enqueue_control(window& state, event::control_data data);
void update_size(window& state);
void ensure_target(window& state);
std::wstring title_text(const std::string& value);
std::shared_ptr<app> open_app();
std::shared_ptr<window> create_window(app& state, const std::string& title,
    double width, double height, bool resizable);
void show_window(window& state);
void set_title(window& state, const std::string& title);
void invalidate(window& state);
std::optional<event> next_event(app& state, std::int64_t timeout);
std::shared_ptr<canvas> begin_frame(window& state);
void end_frame(canvas& state);
LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept;

struct rgba
{
    double red, green, blue, alpha;
};
struct rectangle
{
    double x, y, width, height;
};
D2D1_COLOR_F checked_color(rgba value);
D2D1_RECT_F checked_rect(rectangle value);
float checked_number(double value);
float checked_width(double value);
ID2D1SolidColorBrush* color_brush(canvas& state, rgba value);

} // namespace tx_generated::graphics
