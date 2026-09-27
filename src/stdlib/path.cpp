#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/filesystem_extended.hpp"
#include "stdlib/error.hpp"
#include "stdlib/stdlib.hpp"

#include <windows.h>

#include <limits>
#include <stdexcept>

namespace tx_generated
{

std::string tx_fn_path_normalize(std::string path)
{
    return detail::path_text(detail::path_from_utf8(path).lexically_normal());
}

bool tx_fn_path_is_absolute(std::string path)
{
    return detail::path_from_utf8(path).is_absolute();
}

std::string tx_fn_path_absolute(std::string path)
{
    const auto file = detail::path_from_utf8(path);
    std::error_code error;
    const auto absolute = std::filesystem::absolute(file, error);
    detail::check_filesystem_error(error, "生成绝对路径", path);
    return detail::path_text(absolute.lexically_normal());
}

std::string tx_fn_path_relative(std::string path, std::string base)
{
    const auto file = detail::path_from_utf8(path).lexically_normal();
    const auto root = detail::path_from_utf8(base).lexically_normal();
    const auto relative = file.lexically_relative(root);
    if (relative.empty())
    {
        throw std::runtime_error("无法计算相对路径：" + path + "；基准：" + base);
    }
    return detail::path_text(relative);
}

std::string tx_fn_path_replace_extension(std::string path, std::string extension)
{
    auto file = detail::path_from_utf8(path);
    if (extension.find_first_of("/\\") != std::string::npos)
    {
        throw std::runtime_error("扩展名不能包含路径分隔符");
    }
    const auto replacement = extension.empty() ? std::filesystem::path{}
        : detail::path_from_utf8(extension);
    return detail::path_text(file.replace_extension(replacement));
}

std::string tx_fn_path_canonical(std::string path)
{
    const auto file = detail::checked_path(path);
    std::error_code error;
    const auto resolved = std::filesystem::canonical(file, error);
    if (error)
    {
        detail::fail_filesystem("解析真实路径", path, error);
    }
    return detail::path_text(resolved);
}

std::string tx_fn_path_weakly_canonical(std::string path)
{
    const auto file = detail::checked_path(path);
    std::error_code error;
    const auto resolved = std::filesystem::weakly_canonical(file, error);
    if (error)
    {
        detail::fail_filesystem("解析已有路径前缀", path, error);
    }
    return detail::path_text(resolved);
}

std::string tx_fn_path_root_name(std::string path)
{
    return detail::path_text(detail::checked_path(path).root_name());
}

std::string tx_fn_path_root_directory(std::string path)
{
    return detail::path_text(detail::checked_path(path).root_directory());
}

std::string tx_fn_path_root_path(std::string path)
{
    return detail::path_text(detail::checked_path(path).root_path());
}

std::string tx_fn_path_stem(std::string path)
{
    return detail::path_text(detail::checked_path(path).stem());
}

tx_int tx_fn_path_compare(std::string left, std::string right)
{
    const auto first = detail::checked_path(left).lexically_normal().generic_wstring();
    const auto second = detail::checked_path(right).lexically_normal().generic_wstring();
    if (first.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        second.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        throw runtime_failure({tx::error_kind::io, "invalid_path",
            "路径过长，无法比较"});
    }
    const int result = CompareStringOrdinal(first.data(),
        static_cast<int>(first.size()), second.data(),
        static_cast<int>(second.size()), TRUE);
    if (result == 0)
    {
        detail::fail_windows("比较路径", left + " / " + right, GetLastError());
    }
    return result == CSTR_LESS_THAN ? -1 : result == CSTR_GREATER_THAN ? 1 : 0;
}

bool tx_fn_path_equivalent(std::string left, std::string right)
{
    const auto first = detail::checked_path(left);
    const auto second = detail::checked_path(right);
    std::error_code error;
    const bool result = std::filesystem::equivalent(first, second, error);
    if (error)
    {
        detail::fail_filesystem("比较文件身份", left + " / " + right, error);
    }
    return result;
}

} // namespace tx_generated
