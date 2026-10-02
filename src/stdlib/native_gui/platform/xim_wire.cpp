#include "stdlib/native_gui/platform/xim.hpp"

#include <algorithm>
#include <stdexcept>

namespace tx::ui
{
void x11_ime::transport_reply(const x11_packet& packet)
{
    if (packet.data.at(1) != 32 || !packet.get32(12))
    {
        throw std::runtime_error("输入法传输握手格式非法");
    }
    transport_major_ = packet.get32(16);
    transport_minor_ = packet.get32(20);
    transport_divide_ = packet.get32(24);
    // X.Org XIM 表 D.3：0.x 用 ClientMessage 通知属性，1.x/2.x 用 PropertyNotify。
    if (transport_major_ > 2 || transport_minor_ > (transport_major_ == 0 ? 2u : transport_major_ == 1 ? 0u : 1u))
    {
        throw std::runtime_error("输入法选择了未知 XIM 传输版本");
    }
    server_ = packet.get32(12);
}

void x11_ime::send_frame(const x11_packet& frame)
{
    if (frame.data.size() > 262144)
    {
        throw std::length_error("输入法消息超过 XIM 帧上限");
    }
    const bool multiple = (transport_major_ == 0 && transport_minor_ > 0) ||
        (transport_major_ == 2 && transport_minor_ == 1);
    const bool only_messages = transport_major_ == 0 && transport_minor_ == 1;
    const bool message_transport = transport_major_ != 1 &&
        (frame.data.size() <= 20 || only_messages || (multiple && frame.data.size() < transport_divide_));
    if (message_transport)
    {
        for (std::size_t offset = 0; offset < frame.data.size(); offset += 20)
        {
            const auto count = std::min<std::size_t>(20, frame.data.size() - offset);
            const auto atom = offset + count == frame.data.size() ? protocol_atom_ : more_atom_;
            connection_.client_message(server_, atom, 8, std::span(frame.data).subspan(offset, count));
        }
        return;
    }
    // 大帧可超过 X11 单次请求上限，按字节追加；接收方按 XIM 帧长重组。
    const std::size_t chunk = connection_.maximum_request * 4 - 24;
    for (std::size_t offset = 0; offset < frame.data.size(); offset += chunk)
    {
        connection_.change_property(server_, property_atom_, 31, 8,
            std::span(frame.data).subspan(offset, std::min(chunk, frame.data.size() - offset)), true);
    }
    if (transport_major_ == 0)
    {
        x11_packet notice;
        notice.u32(frame.data.size());
        notice.u32(property_atom_);
        connection_.client_message(server_, protocol_atom_, 32, notice.data);
    }
}

void x11_ime::receive_property(std::uint32_t atom)
{
    const auto data = connection_.property(communication_, atom, true);
    if (!data.get32(8))
    {
        // 同一属性可合并多次追加；后续已排队的通知可能对应已读完的属性。
        return;
    }
    if (data.get32(8) != 31 || data.data.at(1) != 8 || data.get32(12) ||
        data.get32(16) > data.data.size() - 32 || incoming_.size() + data.get32(16) > 1048576)
    {
        throw std::runtime_error("输入法传输属性类型或长度非法");
    }
    incoming_.insert(incoming_.end(), data.data.begin() + 32, data.data.begin() + 32 + data.get32(16));
}
}
