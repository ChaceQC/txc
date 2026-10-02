#pragma once

#include "stdlib/native_gui/platform/xim.hpp"

#include <functional>

namespace xim_test
{
using namespace tx::ui;

void require(bool condition, const char* message);
x11_packet context(unsigned ic = 23);
x11_packet key(unsigned type, unsigned detail = 38);
x11_packet commit(const std::string& text, unsigned flags = 3);
x11_packet preedit(const std::string& text, unsigned first, unsigned removed, unsigned caret);

struct wire_message
{
    unsigned opcode;
    x11_packet payload;
};

// 独立对端只使用 X11 传输工具；XIM 字段、属性编号和响应顺序在测试侧单独构造。
class fixture
{
public:
    explicit fixture(unsigned major = 0, unsigned minor = 0);
    ~fixture();
    void step();
    void until(const std::function<bool()>& condition);
    wire_message take(unsigned opcode);
    void send(unsigned opcode, x11_packet payload);
    void send_pair(unsigned first_opcode, x11_packet first, unsigned second_opcode, x11_packet second);
    void consume();
    void mask(unsigned value, unsigned ic = 23);
    std::deque<window_event> output;
    std::deque<x11_packet> unfiltered;
    std::vector<bool> repeats;
    std::unique_ptr<x11_ime> ime;
    unsigned forwarded = 0, sync_replies = 0, positions = 0, resets = 0;
    x11_packet last_position;
private:
    x11_connection server_, client_;
    std::uint32_t owner_, client_window_, communication_ = 0;
    std::uint32_t connect_atom_, protocol_atom_, more_atom_, property_atom_;
    unsigned major_, minor_;
    x11_packet incoming_;
    std::deque<wire_message> requests_;
    void receive(const x11_packet& packet);
    void respond(unsigned opcode, const x11_packet& payload);
    void transmit(const x11_packet& frame, bool force_property = false);
    void open();
};
}
