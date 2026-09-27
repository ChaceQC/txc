#pragma once

#include "stdlib/dictionary.hpp"

#include <string>

namespace tx_generated
{

bool tx_fn_env_contains(const std::string& name);
std::string tx_fn_env_get(const std::string& name);
std::string tx_fn_env_get(const std::string& name, const std::string& default_value);
void tx_fn_env_set(const std::string& name, const std::string& value);
bool tx_fn_env_remove(const std::string& name);
tx_dict tx_fn_env_snapshot();

} // namespace tx_generated
