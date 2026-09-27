#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_extended.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/error.hpp"

#include <windows.h>
#include <winioctl.h>

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

namespace tx_generated
{
namespace detail
{

std::filesystem::path checked_path(const std::string& path)
{
    try
    {
        return path_from_utf8(path);
    }
    catch (const std::exception& error)
    {
        throw runtime_failure({tx::error_kind::io, "invalid_path",
            std::string("无效路径：") + error.what()});
    }
}

[[noreturn]] void fail_filesystem(const std::string& operation,
                                  const std::string& path,
                                  const std::error_code& error)
{
    const char* code = "operation_failed";
    if (error == std::errc::no_such_file_or_directory)
    {
        code = "not_found";
    }
    else if (error == std::errc::permission_denied)
    {
        code = "permission_denied";
    }
    else if (error == std::errc::file_exists)
    {
        code = "already_exists";
    }
    else if (error == std::errc::invalid_argument ||
             error == std::errc::filename_too_long)
    {
        code = "invalid_path";
    }
    throw runtime_failure({tx::error_kind::io, code,
        operation + "失败：" + path + "：" + error.message()});
}

[[noreturn]] void fail_windows(const std::string& operation,
                               const std::string& path, unsigned long error)
{
    const char* code = "operation_failed";
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
    {
        code = "not_found";
    }
    else if (error == ERROR_ACCESS_DENIED || error == ERROR_PRIVILEGE_NOT_HELD)
    {
        code = "permission_denied";
    }
    else if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS)
    {
        code = "already_exists";
    }
    else if (error == ERROR_INVALID_NAME || error == ERROR_BAD_PATHNAME)
    {
        code = "invalid_path";
    }
    throw runtime_failure({tx::error_kind::io, code,
        operation + "失败：" + path + "（系统错误 " +
        std::to_string(error) + "）"});
}

} // namespace detail

namespace
{

class native_handle
{
public:
    explicit native_handle(HANDLE value)
        : value_(value)
    {
    }
    ~native_handle()
    {
        if (value_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(value_);
        }
    }
    native_handle(const native_handle&) = delete;
    native_handle& operator=(const native_handle&) = delete;
    native_handle(native_handle&& other) noexcept
        : value_(other.value_)
    {
        other.value_ = INVALID_HANDLE_VALUE;
    }
    [[nodiscard]] HANDLE get() const noexcept
    {
        return value_;
    }

private:
    HANDLE value_;
};

std::int64_t unix_millis(LARGE_INTEGER time)
{
    constexpr std::int64_t epoch_offset = 11644473600000LL;
    return time.QuadPart / 10000 - epoch_offset;
}

native_handle open_metadata(const std::filesystem::path& path, bool follow,
                            DWORD access, const std::string& text)
{
    const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS |
        (follow ? 0 : FILE_FLAG_OPEN_REPARSE_POINT);
    native_handle handle(CreateFileW(path.c_str(), access,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, flags, nullptr));
    if (handle.get() == INVALID_HANDLE_VALUE)
    {
        detail::fail_windows("读取文件元信息", text, GetLastError());
    }
    return handle;
}

struct reparse_header
{
    DWORD tag;
    WORD data_length;
    WORD reserved;
};

struct reparse_names
{
    WORD substitute_offset;
    WORD substitute_length;
    WORD print_offset;
    WORD print_length;
};

std::wstring reparse_target(const std::array<unsigned char, 16 * 1024>& data,
                            DWORD received, const std::string& path)
{
    reparse_header header{};
    reparse_names names{};
    if (received < sizeof(header) + sizeof(names))
    {
        throw runtime_failure({tx::error_kind::io, "operation_failed",
            "符号链接数据过短：" + path});
    }
    std::memcpy(&header, data.data(), sizeof(header));
    std::memcpy(&names, data.data() + sizeof(header), sizeof(names));
    if (header.tag != IO_REPARSE_TAG_SYMLINK &&
        header.tag != IO_REPARSE_TAG_MOUNT_POINT)
    {
        throw runtime_failure({tx::error_kind::io, "invalid_argument",
            "路径不是符号链接或目录联接：" + path});
    }
    const std::size_t buffer_offset = sizeof(header) + sizeof(names) +
        (header.tag == IO_REPARSE_TAG_SYMLINK ? sizeof(ULONG) : 0);
    const std::size_t name_offset = names.print_length == 0
        ? names.substitute_offset : names.print_offset;
    const std::size_t name_length = names.print_length == 0
        ? names.substitute_length : names.print_length;
    const std::size_t valid_end = sizeof(header) + header.data_length;
    if ((name_offset & 1) != 0 || (name_length & 1) != 0 ||
        valid_end > received || buffer_offset > valid_end ||
        name_offset > valid_end - buffer_offset ||
        name_length > valid_end - buffer_offset - name_offset)
    {
        throw runtime_failure({tx::error_kind::io, "operation_failed",
            "符号链接数据无效：" + path});
    }
    std::wstring target(name_length / sizeof(wchar_t), L'\0');
    std::memcpy(target.data(), data.data() + buffer_offset + name_offset,
                name_length);
    if (target.starts_with(L"\\??\\UNC\\"))
    {
        target = L"\\\\" + target.substr(8);
    }
    else if (target.starts_with(L"\\??\\"))
    {
        target.erase(0, 4);
    }
    return target;
}

} // namespace

file_info_value fs_stat(const std::string& path, bool follow)
{
    const auto native_path = detail::checked_path(path);
    const auto handle = open_metadata(native_path, follow,
        FILE_READ_ATTRIBUTES, path);
    FILE_BASIC_INFO basic{};
    FILE_STANDARD_INFO standard{};
    FILE_ATTRIBUTE_TAG_INFO tag{};
    if (!GetFileInformationByHandleEx(handle.get(), FileBasicInfo,
            &basic, sizeof(basic)) ||
        !GetFileInformationByHandleEx(handle.get(), FileStandardInfo,
            &standard, sizeof(standard)) ||
        !GetFileInformationByHandleEx(handle.get(), FileAttributeTagInfo,
            &tag, sizeof(tag)))
    {
        detail::fail_windows("读取文件元信息", path, GetLastError());
    }
    const bool link = !follow &&
        (tag.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 &&
        (tag.ReparseTag == IO_REPARSE_TAG_SYMLINK ||
         tag.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT);
    const bool directory = (basic.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    const bool regular = !link && !directory &&
        (basic.FileAttributes & FILE_ATTRIBUTE_DEVICE) == 0;
    if (regular && standard.EndOfFile.QuadPart < 0)
    {
        throw runtime_failure({tx::error_kind::io, "operation_failed",
            "文件大小无效：" + path});
    }
    const bool readonly = (basic.FileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
    return {link ? "symlink" : directory ? "directory" : regular ? "file" : "other",
        regular ? standard.EndOfFile.QuadPart : 0,
        directory ? (readonly ? 0555 : 0777) : (readonly ? 0444 : 0666),
        unix_millis(basic.CreationTime), unix_millis(basic.LastAccessTime),
        unix_millis(basic.LastWriteTime)};
}

void fs_set_permissions(const std::string& path, std::int64_t permissions)
{
    const auto native_path = detail::checked_path(path);
    const auto handle = open_metadata(native_path, true,
        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, path);
    FILE_BASIC_INFO basic{};
    if (!GetFileInformationByHandleEx(handle.get(), FileBasicInfo,
                                      &basic, sizeof(basic)))
    {
        detail::fail_windows("读取文件权限", path, GetLastError());
    }
    const bool directory = (basic.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (directory ? permissions != 0555 && permissions != 0777
                  : permissions != 0444 && permissions != 0666)
    {
        throw runtime_failure({tx::error_kind::io, "invalid_argument",
            "Windows 文件权限只接受目标类型对应的只读/可写位"});
    }
    if ((permissions & 0222) == 0)
    {
        basic.FileAttributes |= FILE_ATTRIBUTE_READONLY;
    }
    else
    {
        basic.FileAttributes &= ~FILE_ATTRIBUTE_READONLY;
    }
    if (!SetFileInformationByHandle(handle.get(), FileBasicInfo,
                                    &basic, sizeof(basic)))
    {
        detail::fail_windows("修改文件权限", path, GetLastError());
    }
}

void fs_create_symlink(const std::string& target, const std::string& link_path,
                       bool directory)
{
    const auto native_target = detail::checked_path(target);
    const auto native_link = detail::checked_path(link_path);
    constexpr DWORD allow_unprivileged = 0x2;
    const DWORD flags = (directory ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0) |
        allow_unprivileged;
    if (!CreateSymbolicLinkW(native_link.c_str(), native_target.c_str(), flags))
    {
        detail::fail_windows("创建符号链接", link_path, GetLastError());
    }
}

std::string fs_read_symlink(const std::string& path)
{
    if (fs_stat(path, false).kind != "symlink")
    {
        throw runtime_failure({tx::error_kind::io, "invalid_argument",
            "路径不是符号链接或目录联接：" + path});
    }
    const auto native_path = detail::checked_path(path);
    const auto handle = open_metadata(native_path, false, 0, path);
    std::array<unsigned char, 16 * 1024> data{};
    DWORD received = 0;
    if (!DeviceIoControl(handle.get(), FSCTL_GET_REPARSE_POINT,
            nullptr, 0, data.data(), static_cast<DWORD>(data.size()),
            &received, nullptr))
    {
        detail::fail_windows("读取符号链接", path, GetLastError());
    }
    return detail::path_text(std::filesystem::path(
        reparse_target(data, received, path)));
}

} // namespace tx_generated
