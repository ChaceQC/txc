#pragma once

#include <any>
#include <string>
#include <vector>

namespace tx_generated
{

struct dynamic_field
{
    std::string name;
    std::any value;
};

struct dynamic_struct
{
    std::string type_name;
    std::string display_name;
    std::vector<dynamic_field> fields;
};

[[nodiscard]] std::string format_print_value(const std::any& value);

} // namespace tx_generated
