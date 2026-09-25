#pragma once

#include <string>
#include <vector>

namespace tx_generated
{

void tx_prepare_system();
const std::vector<std::string>& tx_fn_system_args();
std::string tx_fn_system_current_directory();
void tx_fn_system_set_current_directory(const std::string& path);
std::string tx_fn_system_executable_path();
std::string tx_fn_system_temp_directory();
std::string tx_fn_system_home_directory();

} // namespace tx_generated
