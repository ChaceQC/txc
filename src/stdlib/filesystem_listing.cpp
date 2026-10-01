#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/filesystem_extended.hpp"
#include "stdlib/error.hpp"
#include "stdlib/stdlib.hpp"

#include <algorithm>
#ifndef _WIN32
#include <cerrno>
#include <dirent.h>
#include <memory>
#endif

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
    std::vector<std::string> names;
#ifndef _WIN32
    if (!recursive)
    {
        // 名称枚举不需要 stat。某些挂载盘未提供 d_type，标准迭代器会
        // 为每项补查元数据；直接 readdir 保持悬空符号链接等名称可见。
        const auto close_directory = [](DIR* directory)
        {
            closedir(directory);
        };
        std::unique_ptr<DIR, decltype(close_directory)> directory(
            opendir(root.c_str()), close_directory);
        if (!directory)
        {
            check_filesystem_error({errno, std::generic_category()}, "列出目录", path);
        }
        for (;;)
        {
            errno = 0;
            const auto* entry = readdir(directory.get());
            if (!entry)
            {
                check_filesystem_error({errno, std::generic_category()}, "列出目录", path);
                break;
            }
            const std::string_view name(entry->d_name);
            if (name != "." && name != "..")
            {
                names.push_back(path_text(std::filesystem::path(entry->d_name)));
            }
        }
    }
    else
#endif
    {
        directory_iterator item(root, error);
        check_filesystem_error(error, "列出目录", path);
        const directory_iterator end;
        while (item != end)
        {
            const auto entry = recursive ? item->path().lexically_relative(root)
                                         : item->path().filename();
            names.push_back(path_text(entry));
            item.increment(error);
            check_filesystem_error(error, "列出目录", path);
        }
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

std::vector<std::string> fs_list_directory_filtered(
    const std::string& path, const std::string& kind,
    const std::string& extension, bool recursive)
{
    if (kind != "any" && kind != "file" && kind != "directory" &&
        kind != "symlink")
    {
        throw runtime_failure({tx::error_kind::io, "invalid_argument",
            "目录筛选类型只能是 any/file/directory/symlink"});
    }
    if (!extension.empty() && (extension.front() != '.' ||
        extension.find_first_of("/\\") != std::string::npos))
    {
        throw runtime_failure({tx::error_kind::io, "invalid_argument",
            "扩展名筛选应为空或带点后缀"});
    }
    const auto root = detail::checked_path(path);
    std::error_code error;
    std::vector<std::string> names;
    const auto visit = [&](const std::filesystem::directory_entry& entry)
    {
        const auto status = entry.symlink_status(error);
        if (error)
        {
            detail::fail_filesystem("筛选目录", path, error);
        }
        const auto type = status.type();
        const bool matches = kind == "any" ||
            (kind == "file" && type == std::filesystem::file_type::regular) ||
            (kind == "directory" && type == std::filesystem::file_type::directory) ||
            (kind == "symlink" && type == std::filesystem::file_type::symlink);
        if (matches && (extension.empty() ||
            detail::path_text(entry.path().extension()) == extension))
        {
            names.push_back(detail::path_text(entry.path().lexically_relative(root)));
        }
    };
    if (recursive)
    {
        std::filesystem::recursive_directory_iterator item(root, error);
        if (error)
        {
            detail::fail_filesystem("筛选目录", path, error);
        }
        const std::filesystem::recursive_directory_iterator end;
        while (item != end)
        {
            visit(*item);
            item.increment(error);
            if (error)
            {
                detail::fail_filesystem("筛选目录", path, error);
            }
        }
    }
    else
    {
        std::filesystem::directory_iterator item(root, error);
        if (error)
        {
            detail::fail_filesystem("筛选目录", path, error);
        }
        const std::filesystem::directory_iterator end;
        while (item != end)
        {
            visit(*item);
            item.increment(error);
            if (error)
            {
                detail::fail_filesystem("筛选目录", path, error);
            }
        }
    }
    std::sort(names.begin(), names.end(),
        [](const std::string& left, const std::string& right)
        {
            return std::lexicographical_compare(left.begin(), left.end(),
                right.begin(), right.end(),
                [](unsigned char first, unsigned char second)
                {
                    return first < second;
                });
        });
    return names;
}

} // namespace tx_generated
