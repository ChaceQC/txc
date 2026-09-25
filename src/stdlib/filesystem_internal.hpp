#pragma once

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace tx_generated::detail
{

std::string path_text(const std::filesystem::path& path);
void check_filesystem_error(const std::error_code& error,
                            const std::string& operation, const std::string& path);
std::vector<std::string> directory_names(const std::string& path, bool recursive);

} // namespace tx_generated::detail
