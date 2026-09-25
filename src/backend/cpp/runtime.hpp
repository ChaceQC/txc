#pragma once

#include "stdlib/stdlib.hpp"

namespace tx_generated
{

tx_array tx_make_array(tx_int length);
tx_array tx_make_array(tx_int length, tx_array initial);
tx_int tx_len(const tx_array& values);
tx_int tx_len(const std::any& value);
std::any& tx_at(tx_array& values, tx_int index);
const std::any& tx_at(const tx_array& values, tx_int index);
std::any& tx_at(std::any& value, tx_int index);
const std::any& tx_at(const std::any& value, tx_int index);
bool tx_is_none(const std::any& value);
tx_int tx_add(tx_int left, tx_int right);
tx_int tx_sub(tx_int left, tx_int right);
tx_int tx_mul(tx_int left, tx_int right);
tx_int tx_div(tx_int left, tx_int right);
double tx_float_div(double left, double right);
tx_int tx_neg(tx_int value);
double tx_neg(double value);

} // namespace tx_generated
