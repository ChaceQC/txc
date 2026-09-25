#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tx_generated
{
namespace detail
{

namespace fs = std::filesystem;

std::string path_text(const fs::path& path)
{
    const auto bytes = path.generic_u8string();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

void check_filesystem_error(const std::error_code& error, const std::string& operation,
                            const std::string& path)
{
    if (error)
    {
        throw std::runtime_error(operation + "失败：" + path + "：" +
                                 error.message());
    }
}

} // namespace detail

namespace fs = std::filesystem;
using detail::check_filesystem_error;
using detail::path_text;

bool tx_fn_exists(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const bool result = fs::exists(file, error);
    check_filesystem_error(error, "检查路径", path);
    return result;
}

bool tx_fn_is_file(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const bool result = fs::is_regular_file(file, error);
    check_filesystem_error(error, "检查文件", path);
    return result;
}

bool tx_fn_is_directory(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const bool result = fs::is_directory(file, error);
    check_filesystem_error(error, "检查目录", path);
    return result;
}

void tx_fn_create_directories(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    (void)fs::create_directories(file, error);
    check_filesystem_error(error, "创建目录", path);
}

tx_array tx_fn_list_directory(std::string path)
{
    auto names = detail::directory_names(path, false);
    tx_array result;
    result.reserve(names.size());
    for (auto& name : names)
    {
        result.emplace_back(std::move(name));
    }
    return result;
}

std::string tx_fn_path_join(std::string left, std::string right)
{
    return path_text(detail::path_from_utf8(left) /
                     detail::path_from_utf8(right));
}

std::string tx_fn_parent(std::string path)
{
    return path_text(detail::path_from_utf8(path).parent_path());
}

std::string tx_fn_file_name(std::string path)
{
    return path_text(detail::path_from_utf8(path).filename());
}

std::string tx_fn_extension(std::string path)
{
    return path_text(detail::path_from_utf8(path).extension());
}

} // namespace tx_generated
