#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace tx_generated
{

struct file_info_value
{
    std::string kind;
    std::int64_t size = 0;
    std::int64_t permissions = 0;
    std::int64_t created_millis = 0;
    std::int64_t accessed_millis = 0;
    std::int64_t modified_millis = 0;
};

file_info_value fs_stat(const std::string& path, bool follow);
void fs_set_permissions(const std::string& path, std::int64_t permissions);
void fs_create_symlink(const std::string& target, const std::string& link_path,
                       bool directory);
std::string fs_read_symlink(const std::string& path);
std::vector<std::string> fs_list_directory_filtered(
    const std::string& path, const std::string& kind,
    const std::string& extension, bool recursive);

namespace detail
{

std::filesystem::path checked_path(const std::string& path);
[[noreturn]] void fail_filesystem(const std::string& operation,
                                  const std::string& path,
                                  const std::error_code& error);
[[noreturn]] void fail_windows(const std::string& operation,
                               const std::string& path, unsigned long error);

} // namespace detail
} // namespace tx_generated
