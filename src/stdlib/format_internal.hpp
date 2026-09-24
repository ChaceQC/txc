#pragma once

#include <any>
#include <string>
#include <string_view>

namespace tx_generated
{

[[nodiscard]] std::string format_field_value(const std::any& value,
                                              std::string_view spec,
                                              char conversion);

} // namespace tx_generated
