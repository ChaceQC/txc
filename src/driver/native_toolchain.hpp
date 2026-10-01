#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace tx
{

std::filesystem::path executable_path();
std::filesystem::path temporary_path(std::string_view extension);
int compile_llvm_native(const std::string& generated_source,
    const std::filesystem::path& output_path, bool lto, bool windows_subsystem = false);
std::vector<std::string> command_arguments(int argc, char* argv[]);

} // namespace tx
