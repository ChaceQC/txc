#include "stdlib/native_gui/platform/xim.hpp"
#include "stdlib/native_gui/core/unicode.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace tx::ui
{
namespace
{
void destroy(x11_connection& connection, std::uint32_t window)
{
    if (window)
    {
        x11_packet request;
        request.u8(4);
        request.u8(0);
        request.u16(0);
        request.u32(window);
        connection.send(std::move(request));
    }
}

void watch(x11_connection& connection, std::uint32_t window)
{
    x11_packet request;
    request.u8(2);
    request.u8(0);
    request.u16(0);
    request.u32(window);
    request.u32(2048);
    request.u32(131072);
    connection.send(std::move(request));
}
}

x11_ime::x11_ime(x11_connection& connection, std::uint32_t window, std::deque<window_event>& output,
    std::function<void(const x11_packet&, bool)> unfiltered, std::uint32_t server_override, std::uint32_t selected_mask)
    : connection_(connection), output_(output), unfiltered_(std::move(unfiltered)), window_(window),
      override_(server_override), selected_mask_(selected_mask)
{
    connect_atom_ = connection_.atom("_XIM_XCONNECT");
    protocol_atom_ = connection_.atom("_XIM_PROTOCOL");
    more_atom_ = connection_.atom("_XIM_MOREDATA");
    property_atom_ = connection_.atom("_client_tx_" + std::to_string(window_));
    start();
}

x11_ime::~x11_ime()
{
    try
    {
        if (ic_)
        {
            x11_packet value;
            value.u16(im_);
            value.u16(ic_);
            send(52, value);
        }
        if (im_)
        {
            x11_packet value;
            value.u16(im_);
            value.u16(0);
            send(32, value);
            send(3);
        }
        destroy(connection_, communication_);
    }
    catch (...)
    {
    }
}

void x11_ime::start()
{
    attempted_ = std::chrono::steady_clock::now();
    forward_mask_ = 0;
    default_mask_ = 0;
    individual_mask_ = false;
    transport_major_ = transport_minor_ = 0;
    transport_divide_ = 20;
    triggers_.clear();
    off_triggers_.clear();
    im_attributes_.clear();
    ic_attributes_.clear();
    incoming_.clear();
    position_dirty_ = true;
    delivery_boundary_ = false;
    phase_ = phase::idle;
    owner_ = override_;
    if (!owner_)
    {
        const auto servers = connection_.property(connection_.root, connection_.atom("XIM_SERVERS"), false, 4096);
        std::string desired;
        if (const auto* modifier = std::getenv("XMODIFIERS"))
        {
            const std::string value(modifier);
            const auto offset = value.find("@im=");
            if (offset != std::string::npos)
            {
                desired = value.substr(offset + 4, value.find('@', offset + 4) - offset - 4);
            }
        }
        if (desired == "none" || desired == "local")
        {
            return;
        }
        if (servers.data[1] != 32 || servers.get32(16) > 256 || servers.get32(12))
        {
            return;
        }
        for (unsigned index = 0; index < servers.get32(16); ++index)
        {
            const auto atom = servers.get32(32 + index * 4);
            if (desired.empty() || connection_.atom_name(atom).ends_with("=" + desired))
            {
                owner_ = connection_.selection_owner(atom);
                if (owner_)
                {
                    break;
                }
            }
        }
    }
    if (!owner_)
    {
        return;
    }
    communication_ = connection_.allocate_id();
    x11_packet create;
    create.u8(1);
    create.u8(0);
    create.u16(0);
    create.u32(communication_);
    create.u32(connection_.root);
    create.u16(0);
    create.u16(0);
    create.u16(1);
    create.u16(1);
    create.u16(0);
    create.u16(2);
    create.u32(0);
    create.u32(2048);
    create.u32(4194304);
    connection_.send(std::move(create));
    watch(connection_, owner_);
    x11_packet hello;
    hello.u32(communication_);
    hello.u32(0);
    hello.u32(0);
    connection_.client_message(owner_, connect_atom_, 32, hello.data);
    requested_ = attempted_;
    phase_ = phase::transport;
    notified_ = false;
    failure_.clear();
}

void x11_ime::send(unsigned opcode, x11_packet payload)
{
    if (!server_)
    {
        return;
    }
    payload.pad();
    x11_packet frame;
    frame.u8(opcode);
    frame.u8(0);
    frame.u16(payload.data.size() / 4);
    frame.bytes(payload.data);
    send_frame(frame);
    if (opcode == 1 || opcode == 30 || opcode == 35 || opcode == 38 || opcode == 44 ||
        opcode == 50 || opcode == 54 || opcode == 60 || opcode == 64)
    {
        requested_ = std::chrono::steady_clock::now();
    }
}

void x11_ime::parse()
{
    while (incoming_.size() >= 4)
    {
        if (incoming_[0] == 0 && std::all_of(incoming_.begin(), incoming_.end(), [](auto byte)
            {
                return byte == 0;
            }))
        {
            incoming_.clear();
            return;
        }
        const auto length = 4 + (unsigned(incoming_[2]) | (unsigned(incoming_[3]) << 8)) * 4;
        if (incoming_.size() < length)
        {
            return;
        }
        const unsigned opcode = incoming_[0];
        x11_packet payload{{incoming_.begin() + 4, incoming_.begin() + length}};
        incoming_.erase(incoming_.begin(), incoming_.begin() + length);
        message(opcode, payload);
    }
}

bool x11_ime::handle(const x11_packet& packet)
{
    const auto type = packet.data[0] & 0x7f;
    if (type == 17 && (packet.get32(8) == owner_ || packet.get32(8) == server_))
    {
        report("输入法服务连接已断开");
        return true;
    }
    if ((type != 33 && type != 28) || !communication_ || packet.get32(4) != communication_)
    {
        return false;
    }
    try
    {
        if (type == 28)
        {
            if (transport_major_ > 0 && packet.data.at(16) == 0)
            {
                receive_property(packet.get32(8));
                parse();
            }
            return true;
        }
        const auto atom = packet.get32(8);
        if (atom == connect_atom_ && phase_ == phase::transport)
        {
            transport_reply(packet);
            watch(connection_, server_);
            phase_ = phase::connect;
            x11_packet connect;
            connect.u8('l');
            connect.u8(0);
            connect.u16(1);
            connect.u16(0);
            connect.u16(0);
            send(1, connect);
            return true;
        }
        if (atom != protocol_atom_ && atom != more_atom_)
        {
            return false;
        }
        if (packet.data[1] == 8)
        {
            incoming_.insert(incoming_.end(), packet.data.begin() + 12, packet.data.begin() + 32);
        }
        else if (packet.data[1] == 32)
        {
            receive_property(packet.get32(16));
        }
        else
        {
            throw std::runtime_error("输入法 ClientMessage 格式非法");
        }
        if (incoming_.size() > 1048576)
        {
            throw std::length_error("输入法消息超过 1 MiB");
        }
        if (atom == protocol_atom_)
        {
            parse();
        }
    }
    catch (const std::out_of_range&)
    {
        report("输入法响应字段越界");
    }
    catch (const std::exception& error)
    {
        report(error.what());
    }
    return true;
}

bool x11_ime::ready() const noexcept
{
    return phase_ == phase::ready;
}

int x11_ime::deadline() const noexcept
{
    if (ready() && !waiting_sync_ && !waiting_trigger_ && !updating_position_ && !reset_sent_ &&
        (!keys_.empty() || delivery_boundary_))
    {
        return 0;
    }
    if (phase_ == phase::idle || phase_ == phase::failed ||
        (phase_ == phase::ready && !waiting_sync_ && !waiting_trigger_ && !updating_position_ && !reset_sent_))
    {
        return -1;
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - requested_).count();
    return static_cast<int>(std::max<std::int64_t>(0, 5000 - elapsed));
}

void x11_ime::tick()
{
    if (ready())
    {
        delivery_boundary_ = false;
        flush_keys();
    }
    const bool waiting = (phase_ != phase::idle && phase_ != phase::failed && phase_ != phase::ready) ||
        waiting_sync_ || waiting_trigger_ || updating_position_ || reset_sent_;
    if (waiting && std::chrono::steady_clock::now() - requested_ >= std::chrono::seconds(5))
    {
        report("输入法服务响应超时");
    }
}

void x11_ime::report(const std::string& message)
{
    phase_ = phase::failed;
    failure_ = message;
    keys_.clear();
    incoming_.clear();
    preedit_.clear();
    waiting_sync_ = waiting_trigger_ = updating_position_ = reset_sent_ = cancelling_ = false;
    composition(false);
    if (enabled_ && !notified_)
    {
        window_event error{event_kind::input_method_error};
        error.text = decode_utf8(message).scalars;
        output_.push_back(std::move(error));
        notified_ = true;
    }
    destroy(connection_, communication_);
    communication_ = 0;
    im_ = ic_ = 0;
    server_ = 0;
}
}
