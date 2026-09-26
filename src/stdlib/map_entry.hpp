#pragma once

#include "backend/cpp/value_format.hpp"
#include "stdlib/container_scalar.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace tx_generated
{

template<class value_type>
[[nodiscard]] std::any entry_field_value(const value_type& value)
{
    if constexpr (std::is_same_v<value_type, text_reference>)
    {
        return scalar_value(value);
    }
    else if constexpr (std::is_same_v<value_type, std::uint8_t>)
    {
        return static_cast<bool>(value);
    }
    else
    {
        return value;
    }
}

template<class key_type, class value_type>
[[nodiscard]] dynamic_struct make_map_entry(
    const std::string& type_name, const key_type& key, const value_type& value)
{
    struct_fields fields(2);
    fields[0] = {"key", entry_field_value(key)};
    fields[1] = {"value", entry_field_value(value)};
    return dynamic_struct(dynamic_struct_data{
        type_name, "entry", std::move(fields)});
}

} // namespace tx_generated
