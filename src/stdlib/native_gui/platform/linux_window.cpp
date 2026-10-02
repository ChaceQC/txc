#include "stdlib/native_gui/platform/linux_window.hpp"
#include "stdlib/native_gui/core/unicode.hpp"
#include "stdlib/native_gui/platform/xim.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <poll.h>
#include <cerrno>

namespace tx::ui
{
namespace
{
constexpr std::uint32_t window_events = 1 | 2 | 4 | 8 | 32 | 64 | 32768 | 131072 | 2097152 | 4194304;
}

linux_window::linux_window(const std::string& title, unsigned width, unsigned height)
{
    if (!width || !height || width > 16384 || height > 16384)
    {
        throw std::invalid_argument("窗口尺寸非法");
    }
    window_ = connection_.allocate_id();
    gc_ = connection_.allocate_id();
    x11_packet create;
    create.u8(1);
    create.u8(0);
    create.u16(0);
    create.u32(window_);
    create.u32(connection_.root);
    create.u16(0);
    create.u16(0);
    create.u16(width);
    create.u16(height);
    create.u16(0);
    create.u16(1);
    create.u32(0);
    create.u32(2 | 2048);
    create.u32(0xffffff);
    create.u32(window_events);
    connection_.send(std::move(create));
    x11_packet gc;
    gc.u8(55);
    gc.u8(0);
    gc.u16(0);
    gc.u32(gc_);
    gc.u32(window_);
    gc.u32(0);
    connection_.send(std::move(gc));
    protocols_atom_ = connection_.atom("WM_PROTOCOLS");
    delete_atom_ = connection_.atom("WM_DELETE_WINDOW");
    x11_packet protocols;
    protocols.u32(delete_atom_);
    property(protocols_atom_, 4, 32, protocols.data);
    set_title(title);
    clipboard_atom_ = connection_.atom("CLIPBOARD");
    utf8_atom_ = connection_.atom("UTF8_STRING");
    targets_atom_ = connection_.atom("TARGETS");
    transfer_atom_ = connection_.atom("_TX_CLIPBOARD_TRANSFER");
    incr_atom_ = connection_.atom("INCR");
    initialize_keyboard();
    ime_ = std::make_unique<x11_ime>(connection_, window_, pending_events_, [&](const x11_packet& packet, bool repeat)
    {
        const auto count = pending_events_.size();
        if (const auto event = translate(packet, true, repeat))
        {
            pending_events_.insert(std::next(pending_events_.begin(), count), *event);
        }
    }, 0, window_events);
}

linux_window::~linux_window()
{
    close();
}

void linux_window::property(std::uint32_t name, std::uint32_t type, unsigned format,
    std::span<const std::uint8_t> value)
{
    write_property(window_, name, type, format, value);
}

void linux_window::write_property(std::uint32_t window, std::uint32_t name, std::uint32_t type,
    unsigned format, std::span<const std::uint8_t> value)
{
    x11_packet request;
    request.u8(18);
    request.u8(0);
    request.u16(0);
    request.u32(window);
    request.u32(name);
    request.u32(type);
    request.u8(format);
    request.u8(0);
    request.u16(0);
    request.u32(value.size() / (format / 8));
    request.bytes(value);
    connection_.send(std::move(request));
}

void linux_window::set_title(const std::string& title)
{
    decode_utf8(title);
    if (title.size() > 65536)
    {
        throw std::length_error("窗口标题超过 65536 字节");
    }
    const auto bytes = std::span(reinterpret_cast<const std::uint8_t*>(title.data()), title.size());
    property(connection_.atom("_NET_WM_NAME"), connection_.atom("UTF8_STRING"), 8, bytes);
    property(39, 31, 8, bytes);
}

void linux_window::show()
{
    x11_packet request;
    request.u8(8);
    request.u8(0);
    request.u16(0);
    request.u32(window_);
    connection_.send(std::move(request));
}

void linux_window::present(const pixel_buffer& image)
{
    const std::size_t stride = image.width() * 4;
    const auto rows = std::max<std::size_t>(1, (connection_.maximum_request * 4 - 24) / stride);
    for (unsigned y = 0; y < image.height();)
    {
        const auto count = static_cast<unsigned>(std::min<std::size_t>(rows, image.height() - y));
        x11_packet request;
        request.u8(72);
        request.u8(2);
        request.u16(0);
        request.u32(window_);
        request.u32(gc_);
        request.u16(image.width());
        request.u16(count);
        request.u16(0);
        request.u16(y);
        request.u8(0);
        request.u8(connection_.depth);
        request.u16(0);
        for (std::size_t index = std::size_t(y) * image.width();
            index < std::size_t(y + count) * image.width(); ++index)
        {
            request.u32(image.pixels()[index]);
        }
        connection_.send(std::move(request));
        y += count;
    }
}

void linux_window::focus()
{
    x11_packet request;
    request.u8(42);
    request.u8(1);
    request.u16(0);
    request.u32(window_);
    request.u32(0);
    connection_.send(std::move(request));
}

void linux_window::capture_pointer(bool enabled)
{
    x11_packet request;
    request.u8(enabled ? 26 : 27);
    request.u8(0);
    request.u16(0);
    if (enabled)
    {
        request.u32(window_);
        request.u16(4 | 8 | 64);
        request.u8(1);
        request.u8(1);
        request.u32(0);
        request.u32(0);
    }
    request.u32(0);
    if (enabled)
    {
        const auto reply = connection_.query(std::move(request));
        if (reply.data[1] != 0)
        {
            throw std::runtime_error("无法捕获 Linux GUI 鼠标");
        }
    }
    else
    {
        connection_.send(std::move(request));
    }
}

void linux_window::set_ime_rect(rect bounds)
{
    if (ime_)
    {
        ime_->position(bounds);
    }
}

void linux_window::enable_ime(bool enabled)
{
    if (ime_)
    {
        ime_->enable(enabled);
    }
}

void linux_window::cancel_composition()
{
    if (ime_)
    {
        ime_->cancel();
    }
}

void linux_window::close() noexcept
{
    if (!window_)
    {
        return;
    }
    try
    {
        ime_.reset();
        x11_packet request;
        request.u8(4);
        request.u8(0);
        request.u16(0);
        request.u32(window_);
        connection_.send(std::move(request));
    }
    catch (...)
    {
    }
    window_ = 0;
    pending_events_.clear();
    transfers_.clear();
    clipboard_.clear();
}

bool linux_window::is_open() const noexcept
{
    return window_ != 0;
}

double linux_window::scale() const noexcept
{
    return 1;
}

std::intptr_t linux_window::wait_handle() const noexcept
{
    return connection_.descriptor();
}

int linux_window::next_wakeup_ms() const noexcept
{
    return !pending_events_.empty() ? 0 : ime_ ? ime_->deadline() : -1;
}

void wait_platform_events(std::span<platform_window* const> windows, int timeout_ms)
{
    std::vector<pollfd> descriptors;
    for (const auto* window : windows)
    {
        descriptors.push_back({static_cast<int>(window->wait_handle()), POLLIN, 0});
        const auto wakeup = window->next_wakeup_ms();
        if (wakeup >= 0 && (timeout_ms < 0 || wakeup < timeout_ms))
        {
            timeout_ms = wakeup;
        }
    }
    int result;
    do
    {
        result = poll(descriptors.data(), descriptors.size(), timeout_ms);
    }
    while (result < 0 && errno == EINTR);
    if (result < 0)
    {
        throw std::runtime_error("等待 GUI 系统事件失败");
    }
}

std::optional<window_event> linux_window::next_event(int timeout_ms)
{
    if (timeout_ms < -1)
    {
        throw std::invalid_argument("事件等待时间非法");
    }
    if (!pending_events_.empty())
    {
        auto result = std::move(pending_events_.front());
        pending_events_.pop_front();
        return result;
    }
    const auto started = std::chrono::steady_clock::now();
    while (window_)
    {
        if (!pending_events_.empty())
        {
            auto event = std::move(pending_events_.front());
            pending_events_.pop_front();
            return event;
        }
        if (ime_)
        {
            ime_->tick();
            if (!pending_events_.empty())
            {
                continue;
            }
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        auto remaining = timeout_ms < 0 ? -1 : static_cast<int>(std::max<std::int64_t>(0, timeout_ms - elapsed));
        if (ime_)
        {
            const auto wakeup = ime_->deadline();
            if (wakeup >= 0 && (remaining < 0 || wakeup < remaining))
            {
                remaining = wakeup;
            }
        }
        const auto message = connection_.next(remaining);
        if (!message)
        {
            if (ime_)
            {
                ime_->tick();
                if (!pending_events_.empty())
                {
                    continue;
                }
            }
            return {};
        }
        if (ime_ && ime_->handle(*message))
        {
            continue;
        }
        if (clipboard_event(*message))
        {
            continue;
        }
        if (auto event = translate(*message))
        {
            return event;
        }
    }
    return {};
}

std::unique_ptr<platform_window> create_platform_window(const std::string& title, unsigned width, unsigned height)
{
    return std::make_unique<linux_window>(title, width, height);
}
}
