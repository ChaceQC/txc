#pragma once

#include "backend/cpp/value_format.hpp"
#include "stdlib/socket.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

namespace tx_generated::socket_abi
{

std::int64_t resource_id(const void* value);
std::shared_ptr<socket::state> state_at(const void* value,
                                        socket::resource_kind kind);
std::string_view bytes_at(const void* value);
dynamic_struct resource_value(const char* type_name,
                              const char* display_name,
                              socket::resource value);
dynamic_struct read_value(const char* type_name, socket::read_result value);
dynamic_struct datagram_value(const char* type_name, socket::datagram value);

} // namespace tx_generated::socket_abi
