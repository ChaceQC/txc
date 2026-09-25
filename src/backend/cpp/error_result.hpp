#pragma once

#include "backend/cpp/value_format.hpp"
#include "stdlib/error.hpp"

#include <any>
#include <utility>

namespace tx_generated::detail
{

[[nodiscard]] dynamic_struct make_error_value(const char* type_name,
                                             const error_info& error);

template<class value_type>
dynamic_struct make_result_value(const char* type_name, const char* error_type,
                                  operation_result<value_type> result)
{
    struct_fields fields(3);
    fields[0] = {"ok", result.ok};
    fields[1] = {"value", std::move(result.value)};
    fields[2] = {"error", make_error_value(error_type, result.error)};
    return dynamic_struct(dynamic_struct_data{type_name, "result", std::move(fields)});
}

} // namespace tx_generated::detail
