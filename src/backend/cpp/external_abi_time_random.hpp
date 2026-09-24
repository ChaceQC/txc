#pragma once

#include <any>
#include <string_view>

namespace tx_generated::detail
{

std::any dispatch_time_random(std::string_view name,
                              const void* const* arguments);

} // namespace tx_generated::detail
