#include "stdlib/process_internal.hpp"
#include "stdlib/encoding.hpp"

#include <filesystem>
#include <map>

namespace tx_generated::process_detail
{
namespace
{

std::wstring checked_text(const std::string& text)
{
    if (text.find('\0') != std::string::npos)
    {
        fail("invalid_argument", "进程参数不能包含 NUL");
    }
    return detail::utf8_to_wide(text);
}

std::wstring quote_argument(const std::wstring& value)
{
    // CRT 只把引号之前及末尾的反斜杠视为转义；始终包引号也保留空参数。
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (const auto character : value)
    {
        if (character == L'\\')
        {
            ++slashes;
            continue;
        }
        result.append(slashes * (character == L'"' ? 2 : 1), L'\\');
        if (character == L'"')
        {
            result += L'\\';
        }
        result += character;
        slashes = 0;
    }
    result.append(slashes * 2, L'\\');
    return result + L'"';
}

struct environment_less
{
    bool operator()(const std::wstring& left, const std::wstring& right) const
    {
        return CompareStringOrdinal(left.c_str(), -1, right.c_str(), -1, TRUE)
            == CSTR_LESS_THAN;
    }
};

using environment_map = std::map<std::wstring, std::wstring, environment_less>;

void inherit_environment(environment_map& values)
{
    struct environment_owner
    {
        wchar_t* data = GetEnvironmentStringsW();
        ~environment_owner()
        {
            if (data)
            {
                FreeEnvironmentStringsW(data);
            }
        }
    } owner;
    if (!owner.data)
    {
        fail("spawn_failed", "读取父进程环境失败", GetLastError());
    }
    for (auto* entry = owner.data; *entry; entry += wcslen(entry) + 1)
    {
        const std::wstring item(entry);
        const auto separator = item.find(L'=', item[0] == L'=' ? 1 : 0);
        if (separator != std::wstring::npos)
        {
            values[item.substr(0, separator)] = item.substr(separator + 1);
        }
    }
}

std::vector<wchar_t> prepare_environment(const process_options& options)
{
    environment_map values;
    if (!options.clear_env)
    {
        inherit_environment(values);
    }
    for (const auto& item : options.env)
    {
        const auto wide = checked_text(item);
        const auto separator = wide.find(L'=');
        if (separator == 0 || separator == std::wstring::npos)
        {
            fail("invalid_argument", "环境覆盖条目必须为非空 NAME=value");
        }
        values[wide.substr(0, separator)] = wide.substr(separator + 1);
    }
    std::vector<wchar_t> result;
    for (const auto& [name, value] : values)
    {
        if (result.size() + name.size() + value.size() + 3 > 32767)
        {
            fail("invalid_argument", "子进程环境超过 32767 个 UTF-16 单元");
        }
        result.insert(result.end(), name.begin(), name.end());
        result.push_back(L'=');
        result.insert(result.end(), value.begin(), value.end());
        result.push_back(L'\0');
    }
    if (result.empty())
    {
        result.push_back(L'\0');
    }
    result.push_back(L'\0');
    return result;
}

} // namespace

launch_data prepare_launch(const process_options& options)
{
    const auto executable = checked_text(options.executable);
    if (executable.empty() || executable.find(L'"') != std::wstring::npos)
    {
        fail("invalid_argument", "可执行文件路径不能为空或含引号");
    }
    std::error_code error;
    const auto absolute = std::filesystem::absolute(executable, error);
    if (error)
    {
        fail("spawn_failed", "解析可执行文件路径失败", error.value());
    }
    const auto extension = absolute.extension().wstring();
    if (CompareStringOrdinal(extension.c_str(), -1, L".bat", -1, TRUE) == CSTR_EQUAL ||
        CompareStringOrdinal(extension.c_str(), -1, L".cmd", -1, TRUE) == CSTR_EQUAL)
    {
        fail("invalid_argument", "process 不隐式执行 shell 脚本");
    }
    launch_data result;
    result.executable = absolute.wstring();
    result.command_line = quote_argument(result.executable);
    for (const auto& argument : options.args)
    {
        result.command_line += L' ' + quote_argument(checked_text(argument));
        if (result.command_line.size() >= 32767)
        {
            fail("invalid_argument", "命令行超过 Windows 长度限制");
        }
    }
    if (result.command_line.size() >= 32767)
    {
        fail("invalid_argument", "可执行文件路径超过 Windows 长度限制");
    }
    result.cwd = checked_text(options.cwd);
    result.environment = prepare_environment(options);
    return result;
}

} // namespace tx_generated::process_detail
