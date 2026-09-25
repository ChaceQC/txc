#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <algorithm>

namespace tx_generated
{
namespace detail
{
namespace
{

template<class directory_iterator>
std::vector<std::string> collect_names(const std::string& path, bool recursive)
{
    const auto root = path_from_utf8(path);
    std::error_code error;
    directory_iterator item(root, error);
    check_filesystem_error(error, "列出目录", path);
    std::vector<std::string> names;
    const directory_iterator end;
    while (item != end)
    {
        const auto entry = recursive ? item->path().lexically_relative(root)
                                     : item->path().filename();
        names.push_back(path_text(entry));
        item.increment(error);
        check_filesystem_error(error, "列出目录", path);
    }
    // 按无符号 UTF-8 字节排序，保持旧 list_directory 的排序契约。
    std::sort(names.begin(), names.end(),
        [](const std::string& left, const std::string& right)
        {
            return std::lexicographical_compare(
                left.begin(), left.end(), right.begin(), right.end(),
                [](unsigned char first, unsigned char second)
                {
                    return first < second;
                });
        });
    return names;
}

} // namespace

std::vector<std::string> directory_names(const std::string& path, bool recursive)
{
    // 默认的递归迭代器不进入子目录符号链接。
    return recursive
        ? collect_names<std::filesystem::recursive_directory_iterator>(path, true)
        : collect_names<std::filesystem::directory_iterator>(path, false);
}

} // namespace detail

std::vector<std::string> tx_fn_list_directory_vector(std::string path)
{
    return detail::directory_names(path, false);
}

std::vector<std::string> tx_fn_walk_directory(std::string path)
{
    return detail::directory_names(path, true);
}

} // namespace tx_generated
