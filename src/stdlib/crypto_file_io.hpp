#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef _WIN32
#include <windows.h>
#endif

namespace tx_generated::crypto
{

#ifdef _WIN32
using file_handle = HANDLE;
inline const file_handle invalid_file_handle = INVALID_HANDLE_VALUE;
#else
using file_handle = int;
inline constexpr file_handle invalid_file_handle = -1;
#endif

class file_input
{
public:
    explicit file_input(const std::filesystem::path& path);
    ~file_input() noexcept;
    file_input(const file_input&) = delete;
    file_input& operator=(const file_input&) = delete;

    std::size_t read(std::span<std::uint8_t> output);
    void read_exact(std::span<std::uint8_t> output);

private:
    file_handle handle_ = invalid_file_handle;
};

class file_output
{
public:
    explicit file_output(const std::filesystem::path& destination);
    ~file_output() noexcept;
    file_output(const file_output&) = delete;
    file_output& operator=(const file_output&) = delete;

    void write(std::span<const std::uint8_t> data);
    void commit(const std::filesystem::path& destination);

private:
    std::filesystem::path path_;
    file_handle handle_ = invalid_file_handle;
    bool remove_on_exit_ = true;
};

void require_distinct_paths(const std::filesystem::path& source,
                            const std::filesystem::path& destination);

} // namespace tx_generated::crypto
