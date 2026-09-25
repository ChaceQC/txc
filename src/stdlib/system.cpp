#include "stdlib/system.hpp"
#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_internal.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

namespace tx_generated
{
namespace
{

void check_system_error(unsigned long error, std::string_view operation)
{
    if (error != 0)
    {
        throw std::runtime_error(std::string(operation) +
            "失败（系统错误码 " + std::to_string(error) + "）");
    }
}

std::vector<std::string> read_arguments()
{
    int count = 0;
    std::unique_ptr<wchar_t*, decltype(&LocalFree)> arguments(
        CommandLineToArgvW(GetCommandLineW(), &count), &LocalFree);
    if (!arguments)
    {
        check_system_error(GetLastError(), "读取程序参数");
        throw std::runtime_error("读取程序参数失败");
    }
    std::vector<std::string> result;
    // 第零项只是启动时的程序名，真实可执行文件路径另向系统查询。
    for (int index = 1; index < count; ++index)
    {
        result.push_back(detail::wide_to_utf8(arguments.get()[index]));
    }
    return result;
}

std::string absolute_path_text(const std::filesystem::path& path)
{
    std::error_code error;
    const auto absolute = std::filesystem::absolute(path, error);
    check_system_error(error.value(), "读取系统路径");
    return detail::path_text(absolute.lexically_normal());
}

} // namespace

const std::vector<std::string>& tx_fn_system_args()
{
    // 入口初始化后只读；每次交给 TX 的向量由 ABI 层独立构造。
    static const auto arguments = read_arguments();
    return arguments;
}

void tx_prepare_system()
{
    (void)tx_fn_system_args();
}

std::string tx_fn_system_current_directory()
{
    std::error_code error;
    const auto path = std::filesystem::current_path(error);
    check_system_error(error.value(), "读取当前工作目录");
    return absolute_path_text(path);
}

void tx_fn_system_set_current_directory(const std::string& path)
{
    const auto directory = detail::path_from_utf8(path);
    std::error_code error;
    std::filesystem::current_path(directory, error);
    check_system_error(error.value(), "切换工作目录");
}

std::string tx_fn_system_executable_path()
{
    std::wstring buffer(MAX_PATH, L'\0');
    while (true)
    {
        const auto length = GetModuleFileNameW(
            nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0)
        {
            check_system_error(GetLastError(), "读取可执行文件路径");
            throw std::runtime_error("读取可执行文件路径失败");
        }
        if (length < buffer.size())
        {
            buffer.resize(length);
            return absolute_path_text(std::filesystem::path(buffer));
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::string tx_fn_system_temp_directory()
{
    std::error_code error;
    const auto path = std::filesystem::temp_directory_path(error);
    check_system_error(error.value(), "读取临时目录");
    return absolute_path_text(path);
}

std::string tx_fn_system_home_directory()
{
    std::array<wchar_t, MAX_PATH> buffer{};
    const auto status = SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr,
                                        SHGFP_TYPE_CURRENT, buffer.data());
    if (FAILED(status))
    {
        check_system_error(static_cast<unsigned long>(status), "读取用户主目录");
    }
    return absolute_path_text(std::filesystem::path(buffer.data()));
}

} // namespace tx_generated
