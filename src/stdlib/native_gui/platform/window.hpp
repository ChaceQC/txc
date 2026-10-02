#pragma once

#include "stdlib/native_gui/core/raster.hpp"

#include <memory>
#include <optional>
#include <span>
#include <string>

namespace tx::ui
{
enum class event_kind
{
    redraw, resized, close_requested, pointer_moved, pointer_down, pointer_up,
    wheel, key_down, key_up, text_input, composition, focus_gained, focus_lost,
    pointer_left, capture_lost, input_method_error
};

struct window_event
{
    window_event() = default;
    window_event(event_kind value) : kind(value)
    {
    }
    event_kind kind = event_kind::redraw;
    double x = 0, y = 0, wheel = 0;
    unsigned button = 0, width = 0, height = 0;
    std::string key;
    std::u32string text;
    std::vector<std::uint32_t> feedback;
    std::size_t caret = 0, selection_start = 0, selection_length = 0;
    bool shift = false, ctrl = false, alt = false, meta = false, repeat = false, composing = false;
};

// 平台层只处理系统窗口、原始输入和已完成的像素，不含控件、字体或绘图算法。
class platform_window
{
public:
    virtual ~platform_window() = default;
    virtual void show() = 0;
    virtual void set_title(const std::string& title) = 0;
    virtual void present(const pixel_buffer& image) = 0;
    virtual std::optional<window_event> next_event(int timeout_ms) = 0;
    virtual void capture_pointer(bool enabled) = 0;
    virtual void focus() = 0;
    virtual void set_ime_rect(rect bounds) = 0;
    virtual void enable_ime(bool enabled) = 0;
    virtual void cancel_composition() = 0;
    virtual std::u32string clipboard_text() = 0;
    virtual bool owns_clipboard() = 0;
    virtual void set_clipboard_text(std::u32string_view text) = 0;
    virtual void close() noexcept = 0;
    virtual bool is_open() const noexcept = 0;
    virtual double scale() const noexcept = 0;
    virtual std::intptr_t wait_handle() const noexcept = 0;
    virtual int next_wakeup_ms() const noexcept
    {
        return -1;
    }
};

std::unique_ptr<platform_window> create_platform_window(const std::string& title, unsigned width, unsigned height);
void wait_platform_events(std::span<platform_window* const> windows, int timeout_ms);
}
