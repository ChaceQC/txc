#pragma once

#include "stdlib/network_common.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

namespace tx_generated::httpx_listener
{

class listener_state;

class slot_token
{
public:
    explicit slot_token(std::shared_ptr<listener_state> owner);
    slot_token(slot_token&& other) noexcept;
    slot_token(const slot_token&) = delete;
    slot_token& operator=(const slot_token&) = delete;
    slot_token& operator=(slot_token&&) = delete;
    ~slot_token();

private:
    std::shared_ptr<listener_state> owner_;
};

struct accepted_connection
{
    network::socket_handle socket;
    slot_token slot;
};

std::int64_t listen(std::string_view host, std::int64_t port,
                    std::int64_t max_connections);
accepted_connection accept(std::int64_t listener, std::int64_t timeout_ms);
void close(std::int64_t listener) noexcept;

} // namespace tx_generated::httpx_listener
