#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/db.hpp"

namespace tx_generated::db_abi
{

template<class value_type>
const value_type& input(const void* handle)
{
    return std::any_cast<const value_type&>(*static_cast<const std::any*>(handle));
}

template<class value_type>
const value_type& field(const dynamic_struct& value, std::size_t index)
{
    return std::any_cast<const value_type&>(value->fields[index].value);
}

template<class value_type>
void* option(const char* type_name, const std::optional<value_type>& value)
{
    struct_fields fields(2);
    fields[0] = {"present", value.has_value()};
    fields[1] = {"value", value ? std::any(*value) : std::any{}};
    return detail::make_handle<std::any>(dynamic_struct(dynamic_struct_data{
        type_name, "option", std::move(fields)}));
}

inline void* make_value(db_value value)
{
    db_validate_value(value, 67108864);
    return detail::make_handle<std::any>(std::move(value));
}

template<class getter>
int read_value(const char* type_name, void** result, getter get) noexcept
{
    return detail::invoke_checked([&]
    {
        *result = option(type_name, get());
    }, tx::error_kind::database);
}

} // namespace tx_generated::db_abi
