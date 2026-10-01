#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace tx
{

struct child_result
{
    std::uint32_t exit_code = 0;
    std::string output;
    std::int64_t duration_ms = 0;
    bool timed_out = false;
    bool output_truncated = false;
};

child_result run_child_process(const std::filesystem::path& executable,
    const std::filesystem::path& working_directory, std::uint32_t timeout_ms,
    const std::vector<std::string>& arguments = {});
bool windows_crash_status(std::uint32_t code);
std::string json_quote(const std::string& value);

} // namespace tx
