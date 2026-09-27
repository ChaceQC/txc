#include "stdlib/crypto_file_io.hpp"

#include "stdlib/crypto_internal.hpp"
#include "stdlib/filesystem_extended.hpp"

#include <algorithm>
#include <array>
#include <system_error>

namespace tx_generated::crypto
{
namespace
{

std::wstring temporary_suffix()
{
    std::array<std::uint8_t, 16> random{};
    fill_random(random);
    constexpr wchar_t digits[] = L"0123456789abcdef";
    std::wstring text;
    text.reserve(random.size() * 2);
    for (const auto byte : random)
    {
        text.push_back(digits[byte >> 4]);
        text.push_back(digits[byte & 15]);
    }
    return text;
}

} // namespace

file_input::file_input(const std::filesystem::path& path)
{
    handle_ = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                          nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        detail::fail_windows("打开加密输入文件", "", GetLastError());
    }
}

file_input::~file_input() noexcept
{
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle_);
    }
}

std::size_t file_input::read(std::span<std::uint8_t> output)
{
    const auto limit = static_cast<DWORD>(std::min<std::size_t>(
        output.size(), 8 * 1024 * 1024));
    DWORD count = 0;
    if (!ReadFile(handle_, output.data(), limit, &count, nullptr))
    {
        detail::fail_windows("读取加密输入文件", "", GetLastError());
    }
    return count;
}

void file_input::read_exact(std::span<std::uint8_t> output)
{
    while (!output.empty())
    {
        const auto count = read(output);
        if (count == 0)
        {
            fail("invalid_format", "加密文件被截断");
        }
        output = output.subspan(count);
    }
}

file_output::file_output(const std::filesystem::path& destination)
{
    if (destination.filename().empty())
    {
        fail("invalid_argument", "加密目标路径缺少文件名");
    }
    const auto parent = destination.parent_path();
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        path_ = parent / (destination.filename().wstring() +
            L".tx-crypt-" + temporary_suffix());
        handle_ = CreateFileW(path_.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle_ != INVALID_HANDLE_VALUE)
        {
            return;
        }
        const auto error = GetLastError();
        if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
        {
            detail::fail_windows("创建加密临时文件", "", error);
        }
    }
    fail("operation_failed", "无法分配唯一的加密临时文件名");
}

file_output::~file_output() noexcept
{
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle_);
    }
    if (remove_on_exit_)
    {
        DeleteFileW(path_.c_str());
    }
}

void file_output::write(std::span<const std::uint8_t> data)
{
    while (!data.empty())
    {
        const auto limit = static_cast<DWORD>(std::min<std::size_t>(
            data.size(), 8 * 1024 * 1024));
        DWORD count = 0;
        if (!WriteFile(handle_, data.data(), limit, &count, nullptr))
        {
            detail::fail_windows("写入加密临时文件", "", GetLastError());
        }
        if (count == 0)
        {
            fail("operation_failed", "写入加密临时文件未取得进展");
        }
        data = data.subspan(count);
    }
}

void file_output::commit(const std::filesystem::path& destination)
{
    if (!FlushFileBuffers(handle_))
    {
        detail::fail_windows("持久化加密临时文件", "", GetLastError());
    }
    const auto closing = handle_;
    handle_ = INVALID_HANDLE_VALUE;
    if (!CloseHandle(closing))
    {
        detail::fail_windows("关闭加密临时文件", "", GetLastError());
    }
    if (!MoveFileExW(path_.c_str(), destination.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        detail::fail_windows("替换加密目标文件", "", GetLastError());
    }
    remove_on_exit_ = false;
}

void require_distinct_paths(const std::filesystem::path& source,
                            const std::filesystem::path& destination)
{
    if (source.lexically_normal() == destination.lexically_normal())
    {
        fail("invalid_argument", "加密文件的源路径和目标路径必须不同");
    }
    std::error_code error;
    if (std::filesystem::equivalent(source, destination, error))
    {
        fail("invalid_argument", "加密文件的源路径和目标路径必须不同");
    }
}

} // namespace tx_generated::crypto
