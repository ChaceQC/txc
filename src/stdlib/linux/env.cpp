#include "stdlib/env.hpp"
#include "stdlib/encoding.hpp"

#include <cerrno>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <system_error>

extern char** environ;

namespace tx_generated
{
namespace
{

std::mutex environment_mutex;

void check_name(const std::string& name)
{
    if (name.empty() || name.find_first_of("=\0", 0, 2) != std::string::npos)
    {
        throw std::runtime_error("环境变量名称不能为空或包含等号、NUL 字符");
    }
    detail::validate_utf8(name);
}

std::optional<std::string> read_environment(const std::string& name)
{
    check_name(name);
    std::lock_guard lock(environment_mutex);
    const auto* value = std::getenv(name.c_str());
    if (!value)
    {
        return std::nullopt;
    }
    detail::validate_utf8(value);
    return std::string(value);
}

} // namespace

bool tx_fn_env_contains(const std::string& name)
{
    check_name(name);
    std::lock_guard lock(environment_mutex);
    return std::getenv(name.c_str()) != nullptr;
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
    check_name(name);
    detail::validate_utf8(value);
    if (value.find('\0') != std::string::npos)
    {
        throw std::runtime_error("环境变量的值不能包含 NUL 字符");
    }
    std::lock_guard lock(environment_mutex);
    if (setenv(name.c_str(), value.c_str(), 1) != 0)
    {
        // 不在错误中输出变量名或值，环境中可能含凭据。
        throw std::system_error(errno, std::generic_category(), "设置环境变量失败");
    }
}

bool tx_fn_env_remove(const std::string& name)
{
    check_name(name);
    std::lock_guard lock(environment_mutex);
    const bool existed = std::getenv(name.c_str()) != nullptr;
    if (unsetenv(name.c_str()) != 0)
    {
        throw std::system_error(errno, std::generic_category(), "删除环境变量失败");
    }
    return existed;
}

tx_dict tx_fn_env_snapshot()
{
    std::lock_guard lock(environment_mutex);
    tx_dict result;
    for (auto** entry = environ; *entry; ++entry)
    {
        const std::string_view item(*entry);
        const auto separator = item.find('=');
        if (separator != std::string_view::npos)
        {
            detail::validate_utf8(item);
            result.emplace_back(std::string(item.substr(0, separator)),
                std::string(item.substr(separator + 1)));
        }
    }
    return result;
}

} // namespace tx_generated
