#pragma once

#include <filesystem>
#include <functional>
#include <optional>

namespace tx
{

int run_test_suite(const std::filesystem::path& source,
                   const std::optional<std::filesystem::path>& selected,
                   bool json_format,
                   const std::function<int(const std::filesystem::path&,
                                           const std::filesystem::path&)>& compile);

} // namespace tx
