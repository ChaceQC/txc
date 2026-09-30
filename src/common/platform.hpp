#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace tx
{

#ifdef _WIN32
inline constexpr std::string_view target_triple = "x86_64-w64-windows-gnu";
inline constexpr std::string_view executable_suffix = ".exe";
#else
inline constexpr std::string_view target_triple = "x86_64-unknown-linux-gnu";
inline constexpr std::string_view executable_suffix = "";
#endif

inline std::string executable_name(std::string_view name)
{
    return std::string(name) + std::string(executable_suffix);
}

inline std::int64_t process_id() noexcept
{
#ifdef _WIN32
    return _getpid();
#else
    return getpid();
#endif
}

} // namespace tx
