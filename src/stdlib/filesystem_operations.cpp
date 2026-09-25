#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <chrono>
#include <limits>
#include <stdexcept>

namespace tx_generated
{
namespace
{

namespace fs = std::filesystem;
using detail::check_filesystem_error;

tx_int checked_count(std::uintmax_t count)
{
    if (count > static_cast<std::uintmax_t>(std::numeric_limits<tx_int>::max()))
    {
        throw std::overflow_error("文件大小或删除数量超出 int 范围");
    }
    return static_cast<tx_int>(count);
}

} // namespace

void tx_fn_copy_file(std::string source, std::string destination, bool overwrite)
{
    const auto from = detail::path_from_utf8(source);
    const auto to = detail::path_from_utf8(destination);
    std::error_code error;
    fs::copy_file(from, to, overwrite ? fs::copy_options::overwrite_existing
                                      : fs::copy_options::none, error);
    check_filesystem_error(error, "复制文件", source + " -> " + destination);
}

void tx_fn_rename(std::string source, std::string destination)
{
    const auto from = detail::path_from_utf8(source);
    const auto to = detail::path_from_utf8(destination);
    std::error_code error;
    const auto target = fs::symlink_status(to, error);
    if (error == std::errc::no_such_file_or_directory)
    {
        error.clear();
    }
    check_filesystem_error(error, "检查移动目标", destination);
    if (fs::exists(target))
    {
        throw std::runtime_error("移动目标已存在：" + destination);
    }
    fs::rename(from, to, error);
    check_filesystem_error(error, "移动路径", source + " -> " + destination);
}

bool tx_fn_remove(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const bool removed = fs::remove(file, error);
    check_filesystem_error(error, "删除路径", path);
    return removed;
}

tx_int tx_fn_remove_all(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const auto count = fs::remove_all(file, error);
    check_filesystem_error(error, "递归删除路径", path);
    return checked_count(count);
}

tx_int tx_fn_file_size(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const auto size = fs::file_size(file, error);
    check_filesystem_error(error, "读取文件大小", path);
    return checked_count(size);
}

tx_int tx_fn_modified_millis(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const auto modified = fs::last_write_time(file, error);
    check_filesystem_error(error, "读取修改时间", path);
    const auto system_time = fs::file_time_type::clock::to_sys(modified);
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        system_time.time_since_epoch()).count();
}

} // namespace tx_generated
