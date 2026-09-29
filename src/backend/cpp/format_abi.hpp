#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"

#include <any>
#include <optional>

namespace tx_generated::format_abi
{

template<class type>
const type& argument(const void* value)
{
    return std::any_cast<const type&>(*static_cast<const std::any*>(value));
}

inline const std::any& value(const void* handle)
{
    return *static_cast<const std::any*>(handle);
}

inline const std::string& text(const void* handle)
{
    return tx_generated::detail::text_value(handle);
}

template<class type>
const type& member(const void* handle, std::size_t index)
{
    return std::any_cast<const type&>(argument<dynamic_struct>(handle)->fields[index].value);
}

inline void* structure(const char* type, const char* display,
                       std::initializer_list<dynamic_field> values)
{
    struct_fields fields(values.size());
    std::size_t index = 0;
    for (const auto& value : values)
    {
        fields[index++] = value;
    }
    return detail::make_handle<std::any>(dynamic_struct(dynamic_struct_data{
        type, display, std::move(fields)}));
}

inline void* option(const char* type, std::optional<std::any> value)
{
    return structure(type, "option", {{"present", value.has_value()},
        {"value", value ? std::move(*value) : std::any{}}});
}

} // namespace tx_generated::format_abi
