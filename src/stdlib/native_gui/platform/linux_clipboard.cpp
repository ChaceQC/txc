#include "stdlib/native_gui/platform/linux_window.hpp"
#include "stdlib/native_gui/core/unicode.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace tx::ui
{
namespace
{
constexpr std::size_t clipboard_limit = 16 * 1024 * 1024;

std::span<const std::uint8_t> bytes_of(const std::string& value)
{
    return {reinterpret_cast<const std::uint8_t*>(value.data()), value.size()};
}

std::uint32_t selection_owner(x11_connection& connection, std::uint32_t selection)
{
    x11_packet request;
    request.u8(23);
    request.u8(0);
    request.u16(0);
    request.u32(selection);
    return connection.query(std::move(request)).get32(8);
}

void convert_selection(x11_connection& connection, std::uint32_t window, std::uint32_t selection,
    std::uint32_t target, std::uint32_t property)
{
    x11_packet request;
    request.u8(24);
    request.u8(0);
    request.u16(0);
    request.u32(window);
    request.u32(selection);
    request.u32(target);
    request.u32(property);
    request.u32(0);
    connection.send(std::move(request));
}

std::string property_bytes(const x11_packet& packet)
{
    if (packet.data.at(1) != 8 || packet.get32(12) != 0 || packet.get32(16) > packet.data.size() - 32)
    {
        throw std::runtime_error("剪贴板属性格式非法或未完整读取");
    }
    return {packet.data.begin() + 32, packet.data.begin() + 32 + packet.get32(16)};
}
}

x11_packet linux_window::read_property(std::uint32_t atom)
{
    x11_packet request;
    request.u8(20);
    request.u8(1);
    request.u16(0);
    request.u32(window_);
    request.u32(atom);
    request.u32(0);
    request.u32(0);
    request.u32(clipboard_limit / 4);
    return connection_.query(std::move(request));
}

void linux_window::set_clipboard_text(std::u32string_view text)
{
    auto encoded = encode_utf8(text);
    if (encoded.size() > clipboard_limit || encoded.find('\0') != std::string::npos)
    {
        throw std::invalid_argument("剪贴板文本过长或包含 NUL");
    }
    x11_packet request;
    request.u8(22);
    request.u8(0);
    request.u16(0);
    request.u32(window_);
    request.u32(clipboard_atom_);
    request.u32(0);
    connection_.send(std::move(request));
    if (selection_owner(connection_, clipboard_atom_) != window_)
    {
        throw std::runtime_error("无法取得 Linux 剪贴板所有权");
    }
    clipboard_.swap(encoded);
}

bool linux_window::owns_clipboard()
{
    return selection_owner(connection_, clipboard_atom_) == window_;
}

std::u32string linux_window::clipboard_text()
{
    const auto owner = selection_owner(connection_, clipboard_atom_);
    if (!owner)
    {
        return {};
    }
    if (owner == window_)
    {
        return decode_utf8(clipboard_).scalars;
    }
    convert_selection(connection_, window_, clipboard_atom_, utf8_atom_, transfer_atom_);
    bool latin = false, incremental = false;
    std::string bytes;
    const auto started = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - started < std::chrono::seconds(5))
    {
        const auto event = connection_.next(100);
        if (!event)
        {
            continue;
        }
        const auto type = event->data[0] & 0x7f;
        const bool notification = type == 31 && event->get32(8) == window_ && event->get32(12) == clipboard_atom_;
        const bool chunk = incremental && type == 28 && event->get32(4) == window_ &&
            event->get32(8) == transfer_atom_ && event->data[16] == 0;
        if (notification && event->get32(20) == 0)
        {
            if (latin)
            {
                throw std::runtime_error("剪贴板不支持 UTF-8 或 STRING 文本");
            }
            latin = true;
            convert_selection(connection_, window_, clipboard_atom_, 31, transfer_atom_);
            continue;
        }
        if (notification || chunk)
        {
            const auto property = read_property(transfer_atom_);
            if (property.get32(8) == incr_atom_)
            {
                if (property.data[1] != 32 || property.get32(16) != 1 || property.get32(32) > clipboard_limit)
                {
                    throw std::runtime_error("剪贴板 INCR 长度非法");
                }
                incremental = true;
                continue;
            }
            const auto data = property_bytes(property);
            if (data.size() > clipboard_limit - bytes.size())
            {
                throw std::length_error("剪贴板文本超过 16 MiB");
            }
            bytes += data;
            if (!incremental || data.empty())
            {
                if (latin)
                {
                    std::u32string text;
                    for (const unsigned char byte : bytes)
                    {
                        text.push_back(byte);
                    }
                    return text;
                }
                return decode_utf8(bytes).scalars;
            }
        }
        else if (!clipboard_event(*event))
        {
            const auto count = pending_events_.size();
            if (const auto translated = translate(*event))
            {
                pending_events_.insert(std::next(pending_events_.begin(), count), *translated);
            }
        }
    }
    throw std::runtime_error("读取 Linux 剪贴板超时");
}

bool linux_window::clipboard_event(const x11_packet& event)
{
    const auto type = event.data[0] & 0x7f;
    if (type == 29 && event.get32(12) == clipboard_atom_)
    {
        clipboard_.clear();
        return true;
    }
    if (type == 28 && event.data[16] == 1)
    {
        const auto found = std::find_if(transfers_.begin(), transfers_.end(), [&](const auto& transfer)
        {
            return transfer.window == event.get32(4) && transfer.property == event.get32(8);
        });
        if (found == transfers_.end())
        {
            return false;
        }
        const auto count = std::min<std::size_t>(65536, found->bytes.size() - found->offset);
        write_property(found->window, found->property, found->type, 8,
            bytes_of(found->bytes).subspan(found->offset, count));
        found->offset += count;
        if (!count)
        {
            transfers_.erase(found);
        }
        return true;
    }
    if (type != 30 || event.get32(16) != clipboard_atom_)
    {
        return false;
    }
    const auto requestor = event.get32(12), target = event.get32(20);
    auto property = event.get32(24);
    if (!property)
    {
        property = target;
    }
    if (target == targets_atom_)
    {
        x11_packet targets;
        targets.u32(targets_atom_);
        targets.u32(utf8_atom_);
        targets.u32(31);
        write_property(requestor, property, 4, 32, targets.data);
    }
    else if (target == utf8_atom_ || target == 31)
    {
        std::string text = clipboard_;
        if (target == 31)
        {
            text.clear();
            for (const auto scalar : decode_utf8(clipboard_).scalars)
            {
                text.push_back(scalar <= 255 ? static_cast<char>(scalar) : '?');
            }
        }
        if (text.size() <= 65536)
        {
            write_property(requestor, property, target, 8, bytes_of(text));
        }
        else if (transfers_.size() < 8)
        {
            x11_packet select;
            select.u8(2);
            select.u8(0);
            select.u16(0);
            select.u32(requestor);
            select.u32(2048);
            select.u32(4194304);
            connection_.send(std::move(select));
            x11_packet size;
            size.u32(text.size());
            write_property(requestor, property, incr_atom_, 32, size.data);
            transfers_.push_back({requestor, property, target, std::move(text), 0});
        }
        else
        {
            property = 0;
        }
    }
    else
    {
        property = 0;
    }
    x11_packet notification;
    notification.u8(25);
    notification.u8(0);
    notification.u16(0);
    notification.u32(requestor);
    notification.u32(0);
    notification.u8(31);
    notification.u8(0);
    notification.u16(0);
    notification.u32(event.get32(4));
    notification.u32(requestor);
    notification.u32(clipboard_atom_);
    notification.u32(target);
    notification.u32(property);
    notification.u32(0);
    notification.u32(0);
    connection_.send(std::move(notification));
    return true;
}
}
