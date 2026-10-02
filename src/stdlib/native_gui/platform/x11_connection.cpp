#include "stdlib/native_gui/platform/x11_connection.hpp"

#include <bit>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

namespace tx::ui
{
void x11_packet::u8(unsigned value)
{
    data.push_back(static_cast<std::uint8_t>(value));
}

void x11_packet::u16(unsigned value)
{
    u8(value);
    u8(value >> 8);
}

void x11_packet::u32(std::uint32_t value)
{
    u16(value);
    u16(value >> 16);
}

void x11_packet::bytes(std::span<const std::uint8_t> value)
{
    data.insert(data.end(), value.begin(), value.end());
}

void x11_packet::text(const std::string& value)
{
    data.insert(data.end(), value.begin(), value.end());
}

void x11_packet::pad()
{
    while (data.size() % 4)
    {
        u8(0);
    }
}

void x11_packet::length()
{
    pad();
    if (data.size() < 4 || data.size() / 4 > 65535)
    {
        throw std::length_error("X11 请求长度非法");
    }
    data[2] = static_cast<std::uint8_t>(data.size() / 4);
    data[3] = static_cast<std::uint8_t>((data.size() / 4) >> 8);
}

unsigned x11_packet::get16(std::size_t offset) const
{
    return unsigned(data.at(offset)) | (unsigned(data.at(offset + 1)) << 8);
}

std::uint32_t x11_packet::get32(std::size_t offset) const
{
    return get16(offset) | (std::uint32_t(get16(offset + 2)) << 16);
}

x11_connection::x11_connection()
{
    const auto* environment = std::getenv("DISPLAY");
    const std::string display = environment ? environment : ":0";
    const auto colon = display.find(':');
    if (colon == std::string::npos || (colon != 0 && display.substr(0, colon) != "unix"))
    {
        throw std::runtime_error("Linux GUI 需要本地 X11/XWayland DISPLAY");
    }
    const auto number = display.substr(colon + 1, display.find('.', colon) - colon - 1);
    if (number.empty() || number.find_first_not_of("0123456789") != std::string::npos || number.size() > 5)
    {
        throw std::runtime_error("DISPLAY 编号非法");
    }
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    const auto path = "/tmp/.X11-unix/X" + number;
    std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
    socket_ = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (socket_ < 0)
    {
        throw std::runtime_error("无法创建 X11 系统连接");
    }
    const timeval timeout{5, 0};
    try
    {
        if (setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ||
            setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)))
        {
            throw std::runtime_error("无法设置 X11 通信超时");
        }
        if (connect(socket_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0)
        {
            throw std::runtime_error("无法连接 X11/XWayland 显示服务");
        }
        setup(number);
    }
    catch (...)
    {
        ::close(socket_);
        socket_ = -1;
        throw;
    }
}

x11_connection::~x11_connection()
{
    if (socket_ >= 0)
    {
        ::close(socket_);
    }
}

int x11_connection::descriptor() const noexcept
{
    return socket_;
}

void x11_connection::write_all(std::span<const std::uint8_t> bytes)
{
    while (!bytes.empty())
    {
        const auto sent = ::send(socket_, bytes.data(), bytes.size(), MSG_NOSIGNAL);
        if (sent < 0 && errno == EINTR)
        {
            continue;
        }
        if (sent <= 0)
        {
            throw std::runtime_error("X11 连接写入失败");
        }
        bytes = bytes.subspan(static_cast<std::size_t>(sent));
    }
}

std::vector<std::uint8_t> x11_connection::read_all(std::size_t count)
{
    if (count > 64 * 1024 * 1024)
    {
        throw std::length_error("X11 响应超过 64 MiB");
    }
    std::vector<std::uint8_t> result(count);
    std::size_t offset = 0;
    while (offset < count)
    {
        const auto received = recv(socket_, result.data() + offset, count - offset, 0);
        if (received < 0 && errno == EINTR)
        {
            continue;
        }
        if (received <= 0)
        {
            throw std::runtime_error("X11 显示连接已关闭");
        }
        offset += static_cast<std::size_t>(received);
    }
    return result;
}

std::uint32_t x11_connection::allocate_id()
{
    if (std::uint64_t(counter_) >= (std::uint64_t(1) << std::popcount(mask_)))
    {
        throw std::length_error("X11 资源编号耗尽");
    }
    auto bits = counter_++;
    std::uint32_t result = base_;
    for (unsigned shift = 0; shift < 32; ++shift)
    {
        if (mask_ & (std::uint32_t(1) << shift))
        {
            result |= (bits & 1) << shift;
            bits >>= 1;
        }
    }
    return result;
}

void x11_connection::send(x11_packet request)
{
    request.length();
    if (request.data.size() / 4 > maximum_request)
    {
        throw std::length_error("X11 请求超过服务器上限");
    }
    write_all(request.data);
    ++sequence_;
}

std::optional<x11_packet> x11_connection::receive(int timeout_ms)
{
    pollfd descriptor{socket_, POLLIN, 0};
    int available;
    do
    {
        available = poll(&descriptor, 1, timeout_ms);
    }
    while (available < 0 && errno == EINTR);
    if (available < 0)
    {
        throw std::runtime_error("等待 X11 事件失败");
    }
    if (!available)
    {
        return {};
    }
    x11_packet result{read_all(32)};
    const auto type = result.data[0] & 0x7f;
    if (type == 0)
    {
        throw std::runtime_error("X11 请求失败，错误码 " + std::to_string(result.data[1]) +
            "，操作码 " + std::to_string(result.data[10]));
    }
    if (type == 1 || type == 35)
    {
        result.bytes(read_all(std::size_t(result.get32(4)) * 4));
    }
    return result;
}

std::optional<x11_packet> x11_connection::next(int timeout_ms)
{
    if (!pending_.empty())
    {
        auto result = std::move(pending_.front());
        pending_.pop_front();
        return result;
    }
    return receive(timeout_ms);
}

x11_packet x11_connection::query(x11_packet request)
{
    send(std::move(request));
    const auto expected = sequence_;
    while (true)
    {
        auto reply = receive(10000);
        if (!reply)
        {
            throw std::runtime_error("X11 系统请求超时");
        }
        if (reply->data[0] == 1 && reply->get16(2) == expected)
        {
            return std::move(*reply);
        }
        if (pending_.size() >= 4096)
        {
            throw std::length_error("X11 事件队列超过 4096 条");
        }
        pending_.push_back(std::move(*reply));
    }
}

std::uint32_t x11_connection::atom(const std::string& name)
{
    x11_packet request;
    request.u8(16);
    request.u8(0);
    request.u16(0);
    request.u16(name.size());
    request.u16(0);
    request.text(name);
    return query(std::move(request)).get32(8);
}
}
