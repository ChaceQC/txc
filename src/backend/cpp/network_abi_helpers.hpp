#pragma once

#include "backend/cpp/value_format.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/httpx.hpp"
#include "stdlib/network_common.hpp"
#include "stdlib/typed_map.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <string_view>

namespace tx_generated::network_abi
{

const std::string& text_at(const void* value);
std::string_view bytes_at(const void* value);
byte_value bytes_from(std::string_view value);
network::header_map read_headers(const std::any& value);
container_handle write_headers(const network::header_map& values);
string_vector write_cookies(const std::vector<std::string>& values);
void* client_response(std::string_view method, std::string_view url,
                      const network::header_map& headers,
                      std::string_view body, std::int64_t timeout,
                      const char* type_name, bool binary = false,
                      bool http2 = false);
dynamic_struct make_response(const char* type_name,
                             http_response_data value, bool binary);
std::int64_t resource_id(const void* value);
dynamic_struct make_resource(const char* type_name,
                             const char* display_name, std::int64_t id);

} // namespace tx_generated::network_abi
