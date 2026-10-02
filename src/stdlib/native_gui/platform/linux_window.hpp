#pragma once

#include "stdlib/native_gui/platform/window.hpp"
#include "stdlib/native_gui/platform/x11_connection.hpp"
#include <array>

namespace tx::ui
{
class x11_ime;
class linux_window final : public platform_window
{
public:
    linux_window(const std::string& title, unsigned width, unsigned height);
    ~linux_window() override;
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
private:
    x11_connection connection_;
    std::uint32_t window_ = 0, gc_ = 0, delete_atom_ = 0, protocols_atom_ = 0;
    std::vector<std::uint32_t> keys_;
    unsigned keys_per_code_ = 0;
    std::array<bool, 256> keys_down_{};
    std::deque<window_event> pending_events_;
    std::string clipboard_;
    std::uint32_t clipboard_atom_ = 0, utf8_atom_ = 0, targets_atom_ = 0, transfer_atom_ = 0, incr_atom_ = 0;
    struct clipboard_transfer
    {
        std::uint32_t window, property, type;
        std::string bytes;
        std::size_t offset = 0;
    };
    std::vector<clipboard_transfer> transfers_;
    std::unique_ptr<x11_ime> ime_;
    void initialize_keyboard();
    void write_property(std::uint32_t window, std::uint32_t name, std::uint32_t type,
        unsigned format, std::span<const std::uint8_t> value);
    x11_packet read_property(std::uint32_t atom);
    bool clipboard_event(const x11_packet& event);
    void property(std::uint32_t name, std::uint32_t type, unsigned format, std::span<const std::uint8_t> value);
    std::optional<window_event> translate(const x11_packet& event, bool from_ime = false, bool repeat = false);
};
}
