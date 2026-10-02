#include "xim_fixture.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace xim_test
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

x11_packet context(unsigned ic)
{
    x11_packet value;
    value.u16(7);
    value.u16(ic);
    return value;
}

namespace
{
std::uint32_t hidden_window(x11_connection& connection)
{
    const auto id = connection.allocate_id();
    x11_packet value;
    value.u8(1);
    value.u8(0);
    value.u16(0);
    value.u32(id);
    value.u32(connection.root);
    for (const unsigned field : {0, 0, 1, 1, 0, 2})
    {
        value.u16(field);
    }
    value.u32(0);
    value.u32(2048);
    value.u32(4194304);
    connection.send(value);
    return id;
}

x11_packet frame(unsigned opcode, x11_packet value)
{
    value.pad();
    x11_packet result;
    result.u8(opcode);
    result.u8(0);
    result.u16(value.data.size() / 4);
    result.bytes(value.data);
    return result;
}

void attribute(x11_packet& list, unsigned id, unsigned type, const std::string& name)
{
    list.u16(id);
    list.u16(type);
    list.u16(name.size());
    list.text(name);
    list.pad();
}
}

fixture::fixture(unsigned major, unsigned minor) : major_(major), minor_(minor)
{
    owner_ = hidden_window(server_);
    client_window_ = hidden_window(client_);
    connect_atom_ = server_.atom("_XIM_XCONNECT");
    protocol_atom_ = server_.atom("_XIM_PROTOCOL");
    more_atom_ = server_.atom("_XIM_MOREDATA");
    property_atom_ = server_.atom("_server_tx_test_" + std::to_string(owner_));
    ime = std::make_unique<x11_ime>(client_, client_window_, output, [&](const x11_packet& packet, bool repeat)
    {
        unfiltered.push_back(packet);
        repeats.push_back(repeat);
    }, owner_);
    ime->position({31, 42, 1, 18});
    ime->enable(true);
    until([&]
    {
        return ime->ready() && positions == 1;
    });
    take(54);
    consume();
}

fixture::~fixture()
{
    ime.reset();
}

void fixture::open()
{
    x11_packet im, ic, reply;
    attribute(im, 71, 10, "queryInputStyle");
    attribute(ic, 101, 3, "inputStyle");
    attribute(ic, 103, 5, "clientWindow");
    attribute(ic, 105, 5, "focusWindow");
    attribute(ic, 107, 0x7fff, "preeditAttributes");
    attribute(ic, 109, 12, "spotLocation");
    attribute(ic, 111, 0, "separatorofNestedList");
    reply.u16(7);
    reply.u16(im.data.size());
    reply.bytes(im.data);
    reply.u16(ic.data.size());
    reply.u16(0);
    reply.bytes(ic.data);
    send(31, reply);
}

void fixture::respond(unsigned opcode, const x11_packet& payload)
{
    requests_.push_back({opcode, payload});
    if (opcode == 1)
    {
        require(payload.data.at(0) == 'l' && payload.get16(2) == 1, "connect version/byte order");
        x11_packet reply;
        reply.u16(1);
        reply.u16(0);
        send(2, reply);
    }
    else if (opcode == 30)
    {
        require(payload.data.at(0) > 0, "open locale");
        open();
    }
    else if (opcode == 44)
    {
        require(payload.get16(0) == 7 && payload.get16(4) == 71, "style attribute ID");
        x11_packet reply;
        for (const unsigned value : {7, 12, 71, 8, 1, 0})
        {
            reply.u16(value);
        }
        reply.u32(0x402);
        send(45, reply);
    }
    else if (opcode == 38)
    {
        require(payload.data.at(4) == 11 && std::string(payload.data.begin() + 5,
            payload.data.begin() + 16) == "UTF8_STRING", "UTF-8 negotiation");
        x11_packet reply;
        for (const unsigned value : {7, 0, 0, 0})
        {
            reply.u16(value);
        }
        send(39, reply);
    }
    else if (opcode == 50)
    {
        require(payload.get16(2) == 24 && payload.get16(4) == 101 && payload.get32(8) == 0x402 &&
            payload.get16(12) == 103 && payload.get32(16) == client_window_ &&
            payload.get16(20) == 105 && payload.get32(24) == client_window_, "IC attributes");
        send(51, context());
        mask(3);
    }
    else if (opcode == 54)
    {
        ++positions;
        last_position = payload;
        send(55, context());
    }
    else if (opcode == 60)
    {
        ++forwarded;
        require(payload.get16(4) == 3 && payload.data.size() == 40, "sync key forwarding");
    }
    else if (opcode == 62)
    {
        ++sync_replies;
    }
    else if (opcode == 64)
    {
        ++resets;
    }
}

void fixture::transmit(const x11_packet& value, bool force_property)
{
    const bool messages = !force_property && major_ != 1 && (value.data.size() <= 20 ||
        (major_ == 0 && minor_ == 1) || (minor_ > 0 && value.data.size() < 80));
    if (messages)
    {
        for (std::size_t i = 0; i < value.data.size(); i += 20)
        {
            const auto count = std::min<std::size_t>(20, value.data.size() - i);
            server_.client_message(communication_, i + count < value.data.size() ? more_atom_ : protocol_atom_,
                8, std::span(value.data).subspan(i, count));
        }
    }
    else
    {
        server_.change_property(communication_, property_atom_, 31, 8, value.data, true);
        if (major_ == 0)
        {
            x11_packet notice;
            notice.u32(value.data.size());
            notice.u32(property_atom_);
            server_.client_message(communication_, protocol_atom_, 32, notice.data);
        }
    }
}

void fixture::send(unsigned opcode, x11_packet payload)
{
    transmit(frame(opcode, std::move(payload)));
}

void fixture::send_pair(unsigned first_opcode, x11_packet first, unsigned second_opcode, x11_packet second)
{
    auto value = frame(first_opcode, std::move(first));
    value.bytes(frame(second_opcode, std::move(second)).data);
    transmit(value, true);
}

void fixture::receive(const x11_packet& packet)
{
    const auto type = packet.data.at(0) & 0x7f;
    if (type == 33 && packet.get32(8) == connect_atom_)
    {
        communication_ = packet.get32(12);
        x11_packet reply;
        for (const unsigned value : {owner_, major_, minor_, 80u})
        {
            reply.u32(value);
        }
        server_.client_message(communication_, connect_atom_, 32, reply.data);
        return;
    }
    const bool notification = type == 28 && major_ > 0 && packet.data.at(16) == 0;
    if (!notification && (type != 33 || (packet.get32(8) != protocol_atom_ && packet.get32(8) != more_atom_)))
    {
        return;
    }
    if (notification || packet.data.at(1) == 32)
    {
        const auto value = server_.property(owner_, packet.get32(notification ? 8 : 16), true);
        if (!value.get32(8))
        {
            return;
        }
        require(value.data.at(1) == 8 && !value.get32(12), "client property format");
        incoming_.bytes(std::span(value.data).subspan(32, value.get32(16)));
    }
    else
    {
        incoming_.bytes(std::span(packet.data).subspan(12, 20));
        if (packet.get32(8) == more_atom_)
        {
            return;
        }
    }
    while (incoming_.data.size() >= 4 && incoming_.data[0])
    {
        const std::size_t length = 4 + incoming_.get16(2) * 4;
        if (length > incoming_.data.size())
        {
            return;
        }
        const auto opcode = incoming_.data[0];
        x11_packet payload{{incoming_.data.begin() + 4, incoming_.data.begin() + length}};
        incoming_.data.erase(incoming_.data.begin(), incoming_.data.begin() + length);
        respond(opcode, payload);
    }
    require(std::all_of(incoming_.data.begin(), incoming_.data.end(), [](auto byte)
    {
        return byte == 0;
    }), "client frame padding");
    incoming_.data.clear();
}

void fixture::step()
{
    if (const auto packet = server_.next(1))
    {
        receive(*packet);
    }
    if (const auto packet = client_.next(1))
    {
        ime->handle(*packet);
    }
    if (output.empty() && unfiltered.empty())
    {
        ime->tick();
    }
}

void fixture::until(const std::function<bool()>& condition)
{
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!condition())
    {
        require(std::chrono::steady_clock::now() < end, "XIM test peer timeout");
        step();
    }
}

wire_message fixture::take(unsigned opcode)
{
    const auto find = [&]
    {
        return std::find_if(requests_.begin(), requests_.end(), [&](const auto& value)
        {
            return value.opcode == opcode;
        });
    };
    until([&]
    {
        return find() != requests_.end();
    });
    auto result = *find();
    requests_.erase(find());
    return result;
}

void fixture::consume()
{
    output.clear();
    unfiltered.clear();
    ime->tick();
}

void fixture::mask(unsigned value, unsigned ic)
{
    auto payload = context(ic);
    payload.u32(value);
    payload.u32(3);
    send(37, payload);
}
}
