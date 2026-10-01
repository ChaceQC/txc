#pragma once

#include <string_view>

namespace tx_generated::detail
{
void report_unhandled_error(std::string_view message) noexcept;
}
