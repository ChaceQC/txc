#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <cstdint>

namespace tx
{

struct test_options
{
    bool json_format = false;
    bool source_directory = false;
    std::uint32_t jobs = 1;
    std::uint32_t timeout_ms = 30000;
};

int run_test_suite(const std::filesystem::path& source,
                   const std::optional<std::filesystem::path>& selected,
                   const test_options& options,
                   const std::function<int(const std::filesystem::path&,
                                           const std::filesystem::path&)>& compile);

} // namespace tx
