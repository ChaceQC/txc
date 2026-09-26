#pragma once

#include <filesystem>

namespace tx
{

void verify_tool_interfaces(const std::filesystem::path& tool_dir);
void verify_tool_package(const std::filesystem::path& tool_dir);

} // namespace tx
