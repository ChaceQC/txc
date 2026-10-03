#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "stdlib/native_gui/platform/window.hpp"

#include <deque>
#include <functional>

namespace tx::ui
{
class windows_window final : public platform_window
{
public:
    windows_window(const std::string& title, unsigned width, unsigned height);
    ~windows_window() override;
    void show() override;
    void set_title(const std::string& title) override;
    void present(const pixel_buffer& image) override;
    std::optional<window_event> next_event(int timeout_ms) override;
    void capture_pointer(bool enabled) override;
    void focus() override;
    void set_ime_rect(rect bounds) override;
    void enable_ime(bool enabled) override;
    void cancel_composition() override;
    std::u32string clipboard_text() override;
    bool owns_clipboard() override;
    void set_clipboard_text(std::u32string_view text) override;
    void close() noexcept override;
    bool is_open() const noexcept override;
    double scale() const noexcept override;
    std::intptr_t wait_handle() const noexcept override;
    int next_wakeup_ms() const noexcept override;
    LRESULT message(UINT message, WPARAM wparam, LPARAM lparam);
    HWND hwnd = nullptr;
    std::function<LRESULT(WPARAM, LPARAM)> accessibility;
private:
    std::deque<window_event> events_;
    double scale_ = 1;
    wchar_t surrogate_ = 0;
    rect ime_;
    bool queue_failed_ = false;
    bool composing_ = false;
    bool releasing_capture_ = false;
    bool ime_enabled_ = false;
    bool input(UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result);
    bool ime(UINT message, LPARAM lparam);
    void enqueue(window_event event);
};

std::wstring utf16_text(std::u32string_view text);
std::u32string utf32_text(std::wstring_view text);
}
