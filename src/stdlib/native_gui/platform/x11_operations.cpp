#include "stdlib/native_gui/platform/x11_connection.hpp"

#include <stdexcept>

namespace tx::ui
{
std::string x11_connection::atom_name(std::uint32_t atom)
{
    x11_packet request;
    request.u8(17);
    request.u8(0);
    request.u16(0);
    request.u32(atom);
    const auto reply = query(std::move(request));
    const auto length = reply.get16(8);
    if (length > reply.data.size() - 32)
    {
        throw std::runtime_error("X11 atom 名称长度非法");
    }
    return {reply.data.begin() + 32, reply.data.begin() + 32 + length};
}

std::uint32_t x11_connection::selection_owner(std::uint32_t atom)
{
    x11_packet request;
    request.u8(23);
    request.u8(0);
    request.u16(0);
    request.u32(atom);
    return query(std::move(request)).get32(8);
}

x11_packet x11_connection::property(std::uint32_t window, std::uint32_t atom, bool remove, std::size_t limit)
{
    x11_packet request;
    request.u8(20);
    request.u8(remove);
    request.u16(0);
    request.u32(window);
    request.u32(atom);
    request.u32(0);
    request.u32(0);
    request.u32(static_cast<std::uint32_t>((limit + 3) / 4));
    return query(std::move(request));
}

void x11_connection::change_property(std::uint32_t window, std::uint32_t atom, std::uint32_t type,
    unsigned format, std::span<const std::uint8_t> value, bool append)
{
    if ((format != 8 && format != 16 && format != 32) || value.size() % (format / 8))
    {
        throw std::invalid_argument("X11 属性格式或长度非法");
    }
    x11_packet request;
    request.u8(18);
    request.u8(append ? 2 : 0);
    request.u16(0);
    request.u32(window);
    request.u32(atom);
    request.u32(type);
    request.u8(format);
    request.u8(0);
    request.u16(0);
    request.u32(value.size() / (format / 8));
    request.bytes(value);
    send(std::move(request));
}

void x11_connection::client_message(std::uint32_t destination, std::uint32_t atom,
    unsigned format, std::span<const std::uint8_t> value)
{
    if (value.size() > 20 || (format != 8 && format != 32))
    {
        throw std::invalid_argument("X11 ClientMessage 长度或格式非法");
    }
    x11_packet request;
    request.u8(25);
    request.u8(0);
    request.u16(0);
    request.u32(destination);
    request.u32(0);
    request.u8(33);
    request.u8(format);
    request.u16(0);
    request.u32(destination);
    request.u32(atom);
    request.bytes(value);
    request.data.resize(44, 0);
    send(std::move(request));
}
}
