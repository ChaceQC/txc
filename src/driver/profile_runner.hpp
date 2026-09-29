#pragma once

#include <filesystem>
#include <functional>

namespace tx
{

struct profile_options
{
    std::filesystem::path output;
    int warmup = 1;
    int samples = 3;
    int interval_ms = 10;
    int timeout_ms = 30000;
};

int run_profile(const std::filesystem::path& source, const profile_options& options,
    const std::function<int(const std::filesystem::path&, const std::filesystem::path&, int)>& compile);

} // namespace tx
