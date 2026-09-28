#pragma once

#include "backend/cpp/error_abi.hpp"
#include "stdlib/error.hpp"

#include <cstdio>
#include <cstddef>
#include <any>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

namespace tx_generated::detail
{

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
    std::mutex reference_mutex;

    template<class... arguments>
    explicit handle_record(arguments&&... values)
        : value_type(std::forward<arguments>(values)...)
    {
    }
};

std::string* copy_text_handle(const void* value);
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
        bool destroy = false;
        {
            std::lock_guard lock(record->reference_mutex);
            if (record->references == 0 || --record->references != 0)
            {
                return;
            }
            unregister_handle(record);
            destroy = record->internal_references == 0;
        }
        if (destroy)
        {
            delete record;
        }
        return;
    }
    unregister_handle(record);
    error_cleanup_guard error_guard;
    delete record;
}

enum class error_effect
{
    local_only,
    may_run_user_code
};

template<error_effect effect = error_effect::may_run_user_code, class operation>
int invoke_checked(operation&& run,
                   tx::error_kind fallback = tx::error_kind::runtime) noexcept
{
    try
    {
        std::forward<operation>(run)();
        // 回调中的 TX 函数可能通过状态返回；成功不清空错误，避免丢失析构错误。
        if constexpr (effect == error_effect::may_run_user_code)
        {
            return static_cast<int>(current_runtime_context().last_error_kind);
        }
        return 0;
    }
    catch (const runtime_failure& error)
    {
        set_error(error.error().kind, error.error().code.c_str(), error.what());
    }
    catch (const std::bad_alloc& error)
    {
        set_error(tx::error_kind::runtime, "allocation_failed", error.what());
    }
    catch (const std::exception& error)
    {
        set_error(fallback, "operation_failed", error.what());
    }
    catch (...)
    {
        set_error(tx::error_kind::runtime, "unknown_error", "未知运行时错误");
    }
    return 1;
}

// 仅用于已经确认不会执行 TX 回调或释放用户对象的操作。
template<class operation>
int invoke_leaf(operation&& run,
                tx::error_kind fallback = tx::error_kind::runtime) noexcept
{
    return invoke_checked<error_effect::local_only>(
        std::forward<operation>(run), fallback);
}

} // namespace tx_generated::detail
