#include "stdlib/encoding.hpp"
#include "stdlib/stdlib.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tx_generated
{
namespace
{

namespace fs = std::filesystem;

std::string path_text(const fs::path& path)
{
    const auto bytes = path.generic_u8string();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

void check_error(const std::error_code& error, const std::string& operation,
                 const std::string& path)
{
    if (error)
    {
        throw std::runtime_error(operation + "失败：" + path + "：" +
                                 error.message());
    }
}

} // namespace

bool tx_fn_exists(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const bool result = fs::exists(file, error);
    check_error(error, "检查路径", path);
    return result;
}

bool tx_fn_is_file(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const bool result = fs::is_regular_file(file, error);
    check_error(error, "检查文件", path);
    return result;
}

bool tx_fn_is_directory(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const bool result = fs::is_directory(file, error);
    check_error(error, "检查目录", path);
    return result;
}

void tx_fn_create_directories(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    (void)fs::create_directories(file, error);
    check_error(error, "创建目录", path);
}

tx_array tx_fn_list_directory(std::string path)
{
    const auto directory = detail::path_from_utf8(path);
    std::error_code error;
    fs::directory_iterator item(directory, error);
    check_error(error, "列出目录", path);
    std::vector<std::string> names;
    const fs::directory_iterator end;
    while (item != end)
    {
        names.push_back(path_text(item->path().filename()));
        item.increment(error);
        check_error(error, "列出目录", path);
    }
    std::sort(names.begin(), names.end(),
              [](const std::string& left, const std::string& right)
              {
                  // 按无符号 UTF-8 字节排序，避免 char 的符号性影响结果。
                  return std::lexicographical_compare(
                      left.begin(), left.end(), right.begin(), right.end(),
                      [](unsigned char first, unsigned char second)
                      {
                          return first < second;
                      });
              });
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
