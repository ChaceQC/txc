#include "stdlib/process_internal.hpp"
#include "stdlib/encoding.hpp"

#include <filesystem>
#include <map>

extern char** environ;

namespace tx_generated::process_detail
{
namespace
{

void check_text(const std::string& text)
{
    if (text.find('\0') != std::string::npos)
    {
        fail("invalid_argument", "进程参数不能包含 NUL");
    }
    (void)detail::utf8_to_wide(text);
}

} // namespace

launch_data prepare_launch(const process_options& options)
{
    check_text(options.executable);
    check_text(options.cwd);
    if (options.executable.empty())
    {
        fail("invalid_argument", "可执行文件路径不能为空");
    }
    launch_data result;
    std::error_code error;
    result.executable = std::filesystem::absolute(options.executable, error).string();
    if (error)
    {
        fail("spawn_failed", "解析可执行文件路径失败", error.value());
    }
    result.cwd = options.cwd;
    result.arguments.push_back(result.executable);
    for (const auto& argument : options.args)
    {
        check_text(argument);
        result.arguments.push_back(argument);
    }
    std::map<std::string, std::string> values;
    if (!options.clear_env)
    {
        for (auto** entry = environ; entry && *entry; ++entry)
        {
            const std::string item(*entry);
            const auto separator = item.find('=');
            if (separator != std::string::npos)
            {
                values[item.substr(0, separator)] = item.substr(separator + 1);
            }
        }
    }
    for (const auto& item : options.env)
    {
        check_text(item);
        const auto separator = item.find('=');
        if (separator == 0 || separator == std::string::npos)
        {
            fail("invalid_argument", "环境覆盖条目必须为非空 NAME=value");
        }
        values[item.substr(0, separator)] = item.substr(separator + 1);
    }
    for (const auto& [name, value] : values)
    {
        result.environment.push_back(name + '=' + value);
    }
    return result;
}

} // namespace tx_generated::process_detail
