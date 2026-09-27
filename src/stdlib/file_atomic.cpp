#include "stdlib/file_extra.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"
#include "stdlib/filesystem_extended.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>

namespace tx_generated
{
namespace
{

[[noreturn]] void atomic_error(const char* code, std::string message)
{
    throw runtime_failure({tx::error_kind::io, code, std::move(message)});
}

std::wstring random_suffix()
{
    std::array<unsigned char, 16> bytes{};
    if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
    {
        atomic_error("operation_failed", "无法生成临时文件名称");
    }
    constexpr wchar_t digits[] = L"0123456789abcdef";
    std::wstring result;
    result.reserve(bytes.size() * 2);
    for (const auto value : bytes)
    {
        result.push_back(digits[value >> 4]);
        result.push_back(digits[value & 15]);
    }
    return result;
}

class temporary_file
{
public:
    explicit temporary_file(const std::filesystem::path& destination)
    {
        const auto parent = destination.parent_path();
        for (int attempt = 0; attempt < 8; ++attempt)
        {
            path_ = parent / (destination.filename().wstring() +
                L".tx-tmp-" + random_suffix());
            handle_ = CreateFileW(path_.c_str(), GENERIC_WRITE, 0, nullptr,
                                  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (handle_ != INVALID_HANDLE_VALUE)
            {
                return;
            }
            const auto error = GetLastError();
            if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
            {
                detail::fail_windows("创建同目录临时文件", "", error);
            }
        }
        atomic_error("already_exists", "无法分配唯一的同目录临时文件名");
    }

    ~temporary_file() noexcept
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

    temporary_file(const temporary_file&) = delete;
    temporary_file& operator=(const temporary_file&) = delete;

    void write(std::string_view data)
    {
        while (!data.empty())
        {
            const auto count = static_cast<DWORD>(std::min<std::size_t>(
                data.size(), 8 * 1024 * 1024));
            DWORD written = 0;
            if (!WriteFile(handle_, data.data(), count, &written, nullptr))
            {
                detail::fail_windows("写入同目录临时文件", "", GetLastError());
            }
            if (written == 0)
            {
                atomic_error("operation_failed", "写入同目录临时文件未取得进展");
            }
            data.remove_prefix(written);
        }
    }

    void commit(const std::filesystem::path& destination)
    {
        if (!FlushFileBuffers(handle_))
        {
            detail::fail_windows("持久化同目录临时文件", "", GetLastError());
        }
        const auto closing = handle_;
        handle_ = INVALID_HANDLE_VALUE;
        if (!CloseHandle(closing))
        {
            detail::fail_windows("关闭同目录临时文件", "", GetLastError());
        }
        if (!MoveFileExW(path_.c_str(), destination.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            detail::fail_windows("原子替换目标文件", "", GetLastError());
        }
        remove_on_exit_ = false;
    }

private:
    std::filesystem::path path_;
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    bool remove_on_exit_ = true;
};

void atomic_write(std::string_view path, std::string_view data)
{
    const auto destination = detail::checked_path(std::string(path));
    if (destination.filename().empty())
    {
        atomic_error("invalid_path", "目标文件路径缺少文件名");
    }
    temporary_file temporary(destination);
    temporary.write(data);
    temporary.commit(destination);
}

} // namespace

void file_atomic_write_bytes(std::string_view path, const byte_value& data)
{
    atomic_write(path, {reinterpret_cast<const char*>(data->data()), data->size()});
}

void file_atomic_write_text(std::string_view path, std::string_view text,
                            std::string_view encoding)
{
    std::string data;
    try
    {
        data = detail::encode_text(text, detail::parse_encoding(encoding));
    }
    catch (const std::runtime_error& error)
    {
        atomic_error("invalid_encoding", error.what());
    }
    atomic_write(path, data);
}

} // namespace tx_generated
