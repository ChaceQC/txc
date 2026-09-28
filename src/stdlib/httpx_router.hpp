#pragma once

#include "stdlib/network_common.hpp"

#include <string_view>

namespace tx_generated::httpx_router
{

struct match_result
{
    bool matched = false;
    network::header_map params;
};

match_result match(std::string_view route_method, std::string_view pattern,
                   std::string_view request_method, std::string_view target);

} // namespace tx_generated::httpx_router
