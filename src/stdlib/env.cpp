#include "stdlib/env.hpp"
#include "stdlib/encoding.hpp"

#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace tx_generated
{
namespace
{

[[noreturn]] void environment_error(std::string_view operation, DWORD error)
{
    // 外部数据可能包含凭据，失败消息不拼接变量名和值。
    throw std::runtime_error(std::string(operation) +
        "环境变量失败（系统错误码 " + std::to_string(error) + "）");
}

std::wstring environment_name(const std::string& name)
{
    if (name.empty() || name.find('=') != std::string::npos ||
        name.find('\0') != std::string::npos)
    {
        throw std::runtime_error("环境变量名称不能为空或包含等号、NUL 字符");
    }
    return detail::utf8_to_wide(name);
}

bool contains_environment(const std::wstring& name)
{
    SetLastError(ERROR_SUCCESS);
    if (GetEnvironmentVariableW(name.c_str(), nullptr, 0) != 0)
    {
        return true;
    }
    const auto error = GetLastError();
    if (error == ERROR_ENVVAR_NOT_FOUND)
    {
        return false;
    }
    if (error != ERROR_SUCCESS)
    {
        environment_error("查询", error);
    }
    return true;
}

std::optional<std::string> read_environment(const std::string& name)
{
    const auto wide_name = environment_name(name);
    std::wstring buffer(256, L'\0');
    while (true)
    {
        // 空值与缺失都可能返回零，必须同时检查已清零的系统错误码。
        SetLastError(ERROR_SUCCESS);
        const auto length = GetEnvironmentVariableW(wide_name.c_str(),
            buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0)
        {
            const auto error = GetLastError();
            if (error == ERROR_ENVVAR_NOT_FOUND)
            {
                return std::nullopt;
            }
            if (error != ERROR_SUCCESS)
            {
                environment_error("读取", error);
            }
        }
        if (length < buffer.size())
        {
            buffer.resize(length);
            return detail::wide_to_utf8(buffer);
        }
        // 变量可能在查询期间变长，按 API 返回的所需容量重新读取。
        buffer.resize(length);
    }
}

} // namespace

bool tx_fn_env_contains(const std::string& name)
{
    return contains_environment(environment_name(name));
}

std::string tx_fn_env_get(const std::string& name)
{
    auto value = read_environment(name);
    if (!value)
    {
        throw std::runtime_error("环境变量未设置");
    }
    return std::move(*value);
}

std::string tx_fn_env_get(const std::string& name, const std::string& default_value)
{
    auto value = read_environment(name);
    return value ? std::move(*value) : default_value;
}

void tx_fn_env_set(const std::string& name, const std::string& value)
{
    const auto wide_name = environment_name(name);
    if (value.find('\0') != std::string::npos)
    {
        throw std::runtime_error("环境变量的值不能包含 NUL 字符");
    }
    const auto wide_value = detail::utf8_to_wide(value);
    // 空字符串指针保留空值；只有 remove 传 nullptr 才表示删除。
    if (!SetEnvironmentVariableW(wide_name.c_str(), wide_value.c_str()))
    {
        environment_error("设置", GetLastError());
    }
}

bool tx_fn_env_remove(const std::string& name)
{
    const auto wide_name = environment_name(name);
    if (!contains_environment(wide_name))
    {
        return false;
    }
    if (!SetEnvironmentVariableW(wide_name.c_str(), nullptr))
    {
        const auto error = GetLastError();
        if (error == ERROR_ENVVAR_NOT_FOUND)
        {
            return false;
        }
        environment_error("删除", error);
    }
    return true;
}

} // namespace tx_generated
