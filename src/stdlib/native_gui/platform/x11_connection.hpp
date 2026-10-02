#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace tx::ui
{
struct x11_packet
{
    std::vector<std::uint8_t> data;
    void u8(unsigned value);
    void u16(unsigned value);
    void u32(std::uint32_t value);
    void bytes(std::span<const std::uint8_t> value);
    void text(const std::string& value);
    void pad();
    void length();
    unsigned get16(std::size_t offset) const;
    std::uint32_t get32(std::size_t offset) const;
};

class x11_connection
{
public:
    x11_connection();
    ~x11_connection();
    x11_connection(const x11_connection&) = delete;
    x11_connection& operator=(const x11_connection&) = delete;
    std::uint32_t allocate_id();
    void send(x11_packet request);
    x11_packet query(x11_packet request);
    std::optional<x11_packet> next(int timeout_ms);
    std::uint32_t atom(const std::string& name);
    std::string atom_name(std::uint32_t atom);
    std::uint32_t selection_owner(std::uint32_t atom);
    x11_packet property(std::uint32_t window, std::uint32_t atom, bool remove, std::size_t limit = 1048576);
    void change_property(std::uint32_t window, std::uint32_t atom, std::uint32_t type, unsigned format,
        std::span<const std::uint8_t> value, bool append = false);
    void client_message(std::uint32_t destination, std::uint32_t atom, unsigned format,
        std::span<const std::uint8_t> value);
    int descriptor() const noexcept;
    std::uint32_t root = 0;
    unsigned depth = 0, maximum_request = 65535, minimum_key = 8, maximum_key = 255;
private:
    int socket_ = -1;
    std::uint32_t base_ = 0, mask_ = 0, counter_ = 1;
    std::uint16_t sequence_ = 0;
    std::deque<x11_packet> pending_;
    void write_all(std::span<const std::uint8_t> bytes);
    std::vector<std::uint8_t> read_all(std::size_t count);
    std::optional<x11_packet> receive(int timeout_ms);
    void setup(const std::string& display);
};
}
