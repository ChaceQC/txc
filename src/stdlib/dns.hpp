#pragma once

#include "stdlib/cancellation.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated::dns
{

struct address
{
    std::string ip;
    std::string family;
    std::int64_t ttl_seconds = 0;
};

std::vector<address> resolve(std::string_view host, std::int64_t timeout_ms,
                             const std::shared_ptr<cancellation_state>& token);

} // namespace tx_generated::dns
