#pragma once

#include "stdlib/array.hpp"
#include "stdlib/dictionary.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace tx_generated
{

using tx_int = std::int64_t;

std::string tx_input();
std::string tx_input(const std::string& prompt);
std::any tx_input_or_none();
std::any tx_input_or_none(const std::string& prompt);
void tx_prepare_console();
void tx_print(tx_int value, bool newline);
void tx_print(double value, bool newline);
void tx_print(bool value, bool newline);
void tx_print(const std::string& value, bool newline);
void tx_fn_write(std::string text);
void tx_fn_write_line(std::string text);
void tx_fn_write_error(std::string text);
void tx_fn_flush();
std::string tx_fn_format(const std::string& text, const tx_array& args,
                         const tx_dict& kwargs);
std::string tx_fn_read_text(std::string path, std::string encoding);
void tx_fn_write_text(std::string path, std::string text, std::string encoding);
void tx_fn_append_text(std::string path, std::string text, std::string encoding);
tx_int tx_parse_int(const std::string& text);
double tx_parse_float(const std::string& text);
tx_int tx_float_to_int(double value);
std::string tx_int_to_string(tx_int value);
std::string tx_float_to_string(double value);
std::string tx_bool_to_string(bool value);
tx_int tx_to_int(const std::any& value);
double tx_to_float(const std::any& value);
std::string tx_to_string(const std::any& value);
tx_int tx_len(const std::string& text);

bool tx_fn_contains(std::string text, std::string part);
bool tx_fn_starts_with(std::string text, std::string prefix);
bool tx_fn_ends_with(std::string text, std::string suffix);
tx_int tx_fn_find(std::string text, std::string part);
std::string tx_fn_slice(std::string text, tx_int start, tx_int end);
std::string tx_fn_replace(std::string text, std::string old, std::string replacement);
tx_array tx_fn_split(std::string text, std::string separator);
std::string tx_fn_join(tx_array parts, std::string separator);
std::string tx_fn_trim(std::string text);
std::string tx_fn_lower(std::string text);
std::string tx_fn_upper(std::string text);
tx_int tx_fn_abs(tx_int value);
double tx_fn_abs(double value);
tx_int tx_fn_min(tx_int left, tx_int right);
double tx_fn_min(double left, double right);
tx_int tx_fn_max(tx_int left, tx_int right);
double tx_fn_max(double left, double right);
tx_int tx_fn_clamp(tx_int value, tx_int lower, tx_int upper);
double tx_fn_clamp(double value, double lower, double upper);
tx_int tx_fn_mod(tx_int left, tx_int right);
double tx_fn_sqrt(double value);
double tx_fn_pow(double base, double exponent);
tx_int tx_fn_floor(double value);
tx_int tx_fn_ceil(double value);
tx_array tx_fn_concat(tx_array left, tx_array right);
tx_array tx_fn_array_slice(tx_array values, tx_int start, tx_int end);
tx_array tx_fn_reverse(tx_array values);
bool tx_fn_exists(std::string path);
bool tx_fn_is_file(std::string path);
bool tx_fn_is_directory(std::string path);
void tx_fn_create_directories(std::string path);
tx_array tx_fn_list_directory(std::string path);
std::string tx_fn_path_join(std::string left, std::string right);
std::string tx_fn_parent(std::string path);
std::string tx_fn_file_name(std::string path);
std::string tx_fn_extension(std::string path);
tx_int tx_fn_unix_millis();
tx_int tx_fn_monotonic_millis();
void tx_fn_sleep_millis(tx_int duration);
void tx_fn_seed(tx_int value);
tx_int tx_fn_random_int(tx_int lower, tx_int upper);
double tx_fn_random_float();

} // namespace tx_generated
