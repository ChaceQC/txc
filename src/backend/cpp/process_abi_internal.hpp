#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/process.hpp"

namespace tx_generated::process_abi
{

template<class value_type>
const value_type& value(const void* handle)
{
    return std::any_cast<const value_type&>(*static_cast<const std::any*>(handle));
}

template<class value_type>
const value_type& field(const dynamic_struct& object, std::size_t index)
{
    return std::any_cast<const value_type&>(object->fields[index].value);
}

process_options options(const void* handle);
dynamic_struct status(const char* type_name, process_status value);

} // namespace tx_generated::process_abi
