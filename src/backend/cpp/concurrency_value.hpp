#pragma once

#include <any>
#include <string_view>

namespace tx_generated
{

bool concurrency_matches(const std::any& value, std::string_view type) noexcept;
bool is_concurrency_value(const std::any& value) noexcept;
bool is_shareable_concurrency_value(const std::any& value) noexcept;

} // namespace tx_generated
