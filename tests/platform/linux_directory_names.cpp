#include "stdlib/filesystem_internal.hpp"
#include "stdlib/error.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main()
{
    char pattern[] = "/tmp/tx-directory-XXXXXX";
    const auto* created = mkdtemp(pattern);
    if (!created)
    {
        throw std::runtime_error("无法创建测试目录");
    }
    struct cleanup
    {
        std::filesystem::path path;
        ~cleanup()
        {
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }
    } directory{created};
    std::filesystem::create_directory(directory.path / "sub");
    std::ofstream(directory.path / "a").put('x');
    std::ofstream(directory.path / "中文").put('x');
    std::ofstream(directory.path / "sub/child").put('x');
    std::filesystem::create_symlink("missing", directory.path / "broken");
    const std::vector<std::string> expected{"a", "broken", "sub", "中文"};
    if (tx_generated::detail::directory_names(directory.path.string(), false) != expected)
    {
        throw std::runtime_error("目录名称、符号链接或 UTF-8 排序错误");
    }
    for (const auto& path : {directory.path / "missing", directory.path / "a"})
    {
        bool failed = false;
        try
        {
            (void)tx_generated::detail::directory_names(path.string(), false);
        }
        catch (const std::runtime_error&)
        {
            failed = true;
        }
        if (!failed)
        {
            throw std::runtime_error("无效目录未报告错误");
        }
    }
    std::cout << "LINUX_DIRECTORY_NAMES_OK\n";
}
