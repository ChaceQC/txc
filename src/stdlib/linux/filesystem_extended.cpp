#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_extended.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/error.hpp"

#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

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


} // namespace detail

file_info_value fs_stat(const std::string& path, bool follow)
{
    const auto native = detail::checked_path(path);
    struct statx info{};
    if (statx(AT_FDCWD, native.c_str(), follow ? 0 : AT_SYMLINK_NOFOLLOW,
        STATX_BASIC_STATS | STATX_BTIME, &info) != 0)
    {
        detail::fail_filesystem("读取文件元信息", path, {errno, std::generic_category()});
    }
    const bool regular = S_ISREG(info.stx_mode);
    const auto millis = [](const statx_timestamp& time)
    {
        return time.tv_sec * 1000 + time.tv_nsec / 1000000;
    };
    if (regular && info.stx_size > static_cast<std::uint64_t>(INT64_MAX))
    {
        throw runtime_failure({tx::error_kind::io, "out_of_range", "文件大小超出整数范围"});
    }
    return {S_ISLNK(info.stx_mode) ? "symlink" :
        S_ISDIR(info.stx_mode) ? "directory" : regular ? "file" : "other",
        regular ? static_cast<std::int64_t>(info.stx_size) : 0,
        info.stx_mode & 07777,
        (info.stx_mask & STATX_BTIME) ? millis(info.stx_btime) : 0,
        millis(info.stx_atime), millis(info.stx_mtime)};
}

void fs_set_permissions(const std::string& path, std::int64_t permissions)
{
    const auto native = detail::checked_path(path);
    if (permissions < 0 || permissions > 07777)
    {
        throw runtime_failure({tx::error_kind::io, "invalid_argument",
            "Linux 文件权限必须在 0000 到 07777 之间"});
    }
    if (chmod(native.c_str(), static_cast<mode_t>(permissions)) != 0)
    {
        detail::fail_filesystem("修改文件权限", path, {errno, std::generic_category()});
    }
}

void fs_create_symlink(const std::string& target, const std::string& link_path, bool)
{
    const auto native_target = detail::checked_path(target);
    const auto native_link = detail::checked_path(link_path);
    if (symlink(native_target.c_str(), native_link.c_str()) != 0)
    {
        detail::fail_filesystem("创建符号链接", link_path, {errno, std::generic_category()});
    }
}

std::string fs_read_symlink(const std::string& path)
{
    const auto native = detail::checked_path(path);
    std::error_code error;
    const auto result = std::filesystem::read_symlink(native, error);
    if (error)
    {
        if (error == std::errc::invalid_argument)
        {
            throw runtime_failure({tx::error_kind::io, "invalid_argument", "路径不是符号链接"});
        }
        detail::fail_filesystem("读取符号链接", path, error);
    }
    return detail::path_text(result);
}

} // namespace tx_generated
