#pragma once

#include "common/format_spec.hpp"

#include <any>
#include <string>
#include <string_view>

namespace tx_generated
{

void append_format_integer(std::string& output, std::int64_t value);
void append_format_bool(std::string& output, bool value);
void append_format_text(std::string& output, std::string_view value);
void append_format_value(std::string& output, const std::any& value,
    const tx::format_spec& spec, char conversion);
void append_format_value(std::string& output, std::int64_t value,
    const tx::format_spec& spec, char conversion);
void append_format_value(std::string& output, double value,
    const tx::format_spec& spec, char conversion);
void append_format_value(std::string& output, bool value,
    const tx::format_spec& spec, char conversion);
void append_format_value(std::string& output, const std::string& value,
    const tx::format_spec& spec, char conversion);

[[nodiscard]] std::string format_field_value(const std::any& value,
                                              std::string_view spec,
                                              char conversion);
[[nodiscard]] std::string format_field_value(const std::any& value,
    const tx::format_spec& spec, char conversion);
[[nodiscard]] std::string format_field_value(std::int64_t value,
    const tx::format_spec& spec, char conversion);
[[nodiscard]] std::string format_field_value(double value,
    const tx::format_spec& spec, char conversion);
[[nodiscard]] std::string format_field_value(bool value,
    const tx::format_spec& spec, char conversion);
[[nodiscard]] std::string format_field_value(const std::string& value,
    const tx::format_spec& spec, char conversion);

} // namespace tx_generated
