#pragma once

#include "stdlib/json_output.hpp"

namespace tx_generated::json_detail
{
void append_string(output_buffer& output, std::string_view text);
void append_float(output_buffer& output, double value);
}
