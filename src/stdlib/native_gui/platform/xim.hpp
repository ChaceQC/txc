#pragma once

#include "stdlib/native_gui/platform/window.hpp"
#include "stdlib/native_gui/platform/x11_connection.hpp"

#include <chrono>
#include <functional>
#include <map>

namespace tx::ui
{
class x11_ime
{
public:
    x11_ime(x11_connection& connection, std::uint32_t window, std::deque<window_event>& output,
        std::function<void(const x11_packet&, bool)> unfiltered, std::uint32_t server_override = 0,
        std::uint32_t selected_mask = 3);
    ~x11_ime();
    bool handle(const x11_packet& packet);
    bool forward(const x11_packet& packet, std::uint32_t symbol, bool repeat);
    void enable(bool value);
    void active(bool value);
    void position(rect bounds);
    void cancel();
    void tick();
    int deadline() const noexcept;
    bool ready() const noexcept;
    bool composing() const noexcept;
private:
    enum class phase
    {
        idle, transport, connect, open, styles, encoding, create, ready, failed
    };
    struct attribute
    {
        std::uint16_t id, type;
    };
    struct trigger
    {
        std::uint32_t symbol, modifier, mask;
    };
    struct pending_key
    {
        x11_packet packet;
        std::uint32_t symbol;
        bool repeat = false;
    };
    x11_connection& connection_;
    std::deque<window_event>& output_;
    std::function<void(const x11_packet&, bool)> unfiltered_;
    std::uint32_t window_, communication_ = 0, owner_ = 0, server_ = 0, override_ = 0;
    std::uint32_t connect_atom_ = 0, protocol_atom_ = 0, more_atom_ = 0, property_atom_ = 0;
    std::uint16_t im_ = 0, ic_ = 0;
    phase phase_ = phase::idle;
    bool enabled_ = false, active_ = true, waiting_sync_ = false, cancelling_ = false, reset_sent_ = false;
    bool waiting_trigger_ = false, updating_position_ = false, position_dirty_ = true, notified_ = false;
    bool preediting_ = false;
    bool sent_repeat_ = false;
    bool delivery_boundary_ = false;
    bool individual_mask_ = false;
    unsigned transport_major_ = 0, transport_minor_ = 0, transport_divide_ = 20;
    std::uint32_t style_ = 0x402;
    std::uint32_t forward_mask_ = 0;
    std::uint32_t default_mask_ = 0;
    std::uint32_t selected_mask_ = 3;
    std::map<std::string, attribute> im_attributes_, ic_attributes_;
    std::vector<trigger> triggers_, off_triggers_;
    std::deque<pending_key> keys_;
    std::vector<std::uint8_t> incoming_;
    std::u32string preedit_;
    std::vector<std::uint32_t> feedback_;
    std::size_t caret_ = 0;
    rect position_;
    std::chrono::steady_clock::time_point requested_, attempted_;
    std::string failure_;
    void start();
    void send(unsigned opcode, x11_packet payload = {});
    void send_frame(const x11_packet& frame);
    void receive_property(std::uint32_t atom);
    void transport_reply(const x11_packet& packet);
    void parse();
    void message(unsigned opcode, const x11_packet& payload);
    bool session_message(unsigned opcode, const x11_packet& payload);
    void open_reply(const x11_packet& payload);
    void connected_reply(const x11_packet& payload);
    void style_reply(const x11_packet& payload);
    void encoding_reply(const x11_packet& payload);
    void created_reply(const x11_packet& payload);
    void negotiate_encoding();
    void create_context();
    void update_position();
    void focus();
    void flush_keys();
    bool process_key(const pending_key& key);
    void sync_reply();
    void composition(bool active);
    void report(const std::string& message);
    void preedit_draw(const x11_packet& payload);
    void preedit_caret(const x11_packet& payload);
};
}
