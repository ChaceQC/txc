#include "stdlib/system.hpp"
#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_internal.hpp"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <pwd.h>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

namespace tx_generated
{

const std::vector<std::string>& tx_fn_system_args()
{
    static const auto arguments = []
    {
        std::ifstream input("/proc/self/cmdline", std::ios::binary);
        if (!input)
        {
            throw std::runtime_error("无法读取程序参数");
        }
        std::vector<std::string> result;
        std::string value;
        std::getline(input, value, '\0');
        while (std::getline(input, value, '\0'))
        {
            detail::validate_utf8(value);
            result.push_back(value);
        }
        return result;
    }();
    return arguments;
}

void tx_prepare_system()
{
    (void)tx_fn_system_args();
}

std::string tx_fn_system_current_directory()
{
    return detail::path_text(std::filesystem::current_path());
}

void tx_fn_system_set_current_directory(const std::string& path)
{
    std::filesystem::current_path(detail::path_from_utf8(path));
}

std::string tx_fn_system_executable_path()
{
    return detail::path_text(std::filesystem::read_symlink("/proc/self/exe"));
}

std::string tx_fn_system_temp_directory()
{
    return detail::path_text(std::filesystem::absolute(std::filesystem::temp_directory_path()));
}

std::string tx_fn_system_home_directory()
{
    // 用户目录按 passwd 记录查询，不把可覆盖的 HOME 当作系统身份信息。
    std::vector<char> buffer(4096);
    passwd entry{};
    passwd* found = nullptr;
    int status = 0;
    while ((status = getpwuid_r(getuid(), &entry, buffer.data(), buffer.size(), &found)) == ERANGE)
    {
        buffer.resize(buffer.size() * 2);
    }
    if (status || !found || !found->pw_dir)
    {
        throw std::runtime_error("无法读取用户主目录");
    }
    return detail::path_text(std::filesystem::absolute(detail::path_from_utf8(found->pw_dir)));
}

} // namespace tx_generated
