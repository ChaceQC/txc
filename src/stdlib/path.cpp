#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/stdlib.hpp"

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

} // namespace tx_generated
