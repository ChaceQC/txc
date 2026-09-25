#pragma once

#include "stdlib/typed_container.hpp"

#include <any>
#include <string_view>

namespace tx_generated
{

inline bool container_matches(const std::any& value, std::string_view name)
{
    const auto* container = std::any_cast<container_handle>(&value);
    return container && *container && (*container)->type_name() == name;
}

} // namespace tx_generated
