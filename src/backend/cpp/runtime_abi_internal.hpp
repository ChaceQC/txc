#pragma once

#include <cstdio>
#include <any>
#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace tx_generated::detail
{

extern thread_local char last_error[256];

enum class handle_kind
{
    text,
    value
};
void register_handle(void* value, handle_kind kind);
void unregister_handle(void* value) noexcept;
void cleanup_live_handles() noexcept;
[[nodiscard]] bool cleanup_in_progress() noexcept;

template<class value_type, class... arguments>
value_type* make_handle(arguments&&... values)
{
    static_assert(std::is_same_v<value_type, std::string> ||
                  std::is_same_v<value_type, std::any>);
    auto result = std::make_unique<value_type>(
        std::forward<arguments>(values)...);
    register_handle(result.get(), std::is_same_v<value_type, std::string>
        ? handle_kind::text : handle_kind::value);
    return result.release();
}

template<class value_type>
void destroy_handle(value_type* value) noexcept
{
    unregister_handle(value);
    delete value;
}

template<class operation>
int invoke_checked(operation&& run) noexcept
{
    try
    {
        std::forward<operation>(run)();
        // 调用方只在失败状态读取错误文本，成功路径无需访问线程局部缓冲区。
        return 0;
    }
    catch (const std::exception& error)
    {
        std::snprintf(last_error, 256, "%s", error.what());
    }
    catch (...)
    {
        std::snprintf(last_error, 256, "%s", "未知运行时错误");
    }
    return 1;
}

} // namespace tx_generated::detail
