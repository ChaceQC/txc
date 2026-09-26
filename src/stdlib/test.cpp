#include "stdlib/test.hpp"

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace tx_generated
{
namespace
{

namespace fs = std::filesystem;

fs::path executable_directory()
{
    std::wstring buffer(MAX_PATH, L'\0');
    while (true)
    {
        const auto length = GetModuleFileNameW(nullptr, buffer.data(),
            static_cast<DWORD>(buffer.size()));
        if (length == 0)
        {
            throw std::runtime_error("无法定位测试程序路径");
        }
        if (length < buffer.size())
        {
            buffer.resize(length);
            return fs::path(buffer).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
}

struct temporary_directories
{
    std::vector<fs::path> paths;

    ~temporary_directories()
    {
        for (const auto& path : paths)
        {
            std::error_code ignored;
            fs::remove_all(path, ignored);
        }
    }
};

temporary_directories& created_directories()
{
    static temporary_directories directories;
    return directories;
}

std::string path_text(const fs::path& path)
{
    const auto bytes = path.u8string();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

} // namespace

std::string create_test_directory()
{
    // txc test 的可执行文件位于运行器工作区；单独运行的只读安装目录回退到系统临时目录。
    const auto executable_base = executable_directory();
    const auto temporary_base = fs::temp_directory_path();
    for (const auto& base : {executable_base, temporary_base})
    {
        for (std::uint64_t suffix = 0; suffix < 10000; ++suffix)
        {
            const auto candidate = base /
                ("tx_test_" + std::to_string(GetCurrentProcessId()) + "_" +
                 std::to_string(suffix));
            std::error_code error;
            if (fs::create_directory(candidate, error))
            {
                try
                {
                    created_directories().paths.push_back(candidate);
                }
                catch (...)
                {
                    fs::remove(candidate, error);
                    throw;
                }
                return path_text(candidate);
            }
            if (error && error != std::errc::file_exists)
            {
                break;
            }
        }
    }
    throw std::runtime_error("无法创建测试临时目录");
}

} // namespace tx_generated
