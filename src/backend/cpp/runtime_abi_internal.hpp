#pragma once

#include <cstdio>
#include <cstddef>
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
struct handle_link
{
    handle_link* newer = nullptr;
    handle_link* older = nullptr;
    handle_kind kind = handle_kind::value;
};

template<class value_type>
struct handle_record : handle_link, value_type
{
    std::size_t references = 1;
    std::size_t internal_references = 0;

    template<class... arguments>
    explicit handle_record(arguments&&... values)
        : value_type(std::forward<arguments>(values)...)
    {
    }
};

std::string* retain_text_handle(const void* value) noexcept;
void retain_text_reference(const void* value) noexcept;
void release_text_reference(const void* value) noexcept;

void register_handle(handle_link* value, handle_kind kind) noexcept;
void unregister_handle(handle_link* value) noexcept;
void cleanup_live_handles() noexcept;
[[nodiscard]] bool cleanup_in_progress() noexcept;

template<class value_type, class... arguments>
value_type* make_handle(arguments&&... values)
{
    static_assert(std::is_same_v<value_type, std::string> ||
                  std::is_same_v<value_type, std::any>);
    auto result = std::make_unique<handle_record<value_type>>(
        std::forward<arguments>(values)...);
    register_handle(result.get(), std::is_same_v<value_type, std::string>
        ? handle_kind::text : handle_kind::value);
    return static_cast<value_type*>(result.release());
}

template<class value_type>
void destroy_handle(value_type* value) noexcept
{
    if (!value)
    {
        return;
    }
    auto* record = static_cast<handle_record<value_type>*>(value);
    if constexpr (std::is_same_v<value_type, std::string>)
    {
        if (--record->references != 0)
        {
            return;
        }
        unregister_handle(record);
        if (record->internal_references == 0)
        {
            delete record;
        }
        return;
    }
    unregister_handle(record);
    delete record;
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
