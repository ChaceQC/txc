#pragma once

#include "backend/cpp/error_abi.hpp"
#include "stdlib/error.hpp"

#include <cstdio>
#include <cstddef>
#include <atomic>
#include <any>
#include <exception>
#include <memory>
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

// 普通值根只由创建它的线程登记和释放，不参与文本内部引用计数。
struct value_handle_record : handle_link, std::any
{
    template<class... arguments>
    explicit value_handle_record(arguments&&... values)
        : std::any(std::forward<arguments>(values)...)
    {
    }
};

struct text_handle_record;

// 内容拥有独立引用计数；首次创建时与所属根同次分配，根注销后仍可存活。
struct text_payload
{
    std::atomic<std::size_t> references{1};
    text_handle_record* owner = nullptr;
    std::string value;

    template<class... arguments>
    explicit text_payload(arguments&&... values)
        : value(std::forward<arguments>(values)...)
    {
    }
};

void retain_text_payload(text_payload* value) noexcept;
void release_text_payload(text_payload* value) noexcept;

struct text_handle_record : handle_link
{
    text_payload* content = nullptr;
    bool building = false;
    bool owns_content = false;

    text_handle_record() noexcept = default;
    explicit text_handle_record(text_payload* value) noexcept
        : content(value)
    {
        retain_text_payload(content);
    }
    ~text_handle_record()
    {
        if (!owns_content)
        {
            release_text_payload(content);
        }
    }
};

struct owned_text_handle_record final : text_handle_record
{
    text_payload storage;

    template<class... arguments>
    explicit owned_text_handle_record(arguments&&... values)
        : storage(std::forward<arguments>(values)...)
    {
        content = &storage;
        owns_content = true;
        storage.owner = this;
    }
};

void release_text_root(text_handle_record* value) noexcept;
text_handle_record* copy_text_handle(const void* value);
text_handle_record* make_text_builder();
std::string& text_builder(void* value);
void publish_text_builder(void* value) noexcept;
const std::string& text_value(const void* value) noexcept;
text_payload* retain_text_reference(const void* value) noexcept;

void register_handle(handle_link* value, handle_kind kind) noexcept;
void unregister_handle(handle_link* value) noexcept;
void cleanup_live_handles() noexcept;
[[nodiscard]] bool cleanup_in_progress() noexcept;

template<class value_type, class... arguments>
auto* make_handle(arguments&&... values)
{
    static_assert(std::is_same_v<value_type, std::string> ||
                  std::is_same_v<value_type, std::any>);
    if constexpr (std::is_same_v<value_type, std::string>)
    {
        auto result = std::make_unique<owned_text_handle_record>(
            std::forward<arguments>(values)...);
        register_handle(result.get(), handle_kind::text);
        return static_cast<text_handle_record*>(result.release());
    }
    else
    {
        auto result = std::make_unique<value_handle_record>(
            std::forward<arguments>(values)...);
        register_handle(result.get(), handle_kind::value);
        return static_cast<std::any*>(result.release());
    }
}

template<class value_type>
void destroy_handle(value_type* value) noexcept
{
    if (!value)
    {
        return;
    }
    static_assert(std::is_same_v<value_type, text_handle_record> ||
                  std::is_same_v<value_type, std::any>);
    if constexpr (std::is_same_v<value_type, text_handle_record>)
    {
        unregister_handle(value);
        release_text_root(value);
    }
    else
    {
        auto* record = static_cast<value_handle_record*>(value);
        unregister_handle(record);
        error_cleanup_guard error_guard;
        delete record;
    }
}

enum class error_effect
{
    local_only,
    may_run_user_code
};

inline void set_checked_error(runtime_context* known_context,
                              tx::error_kind kind, const char* code,
                              const char* message) noexcept
{
    if (known_context)
    {
        set_error(*known_context, kind, code, message);
    }
    else
    {
        set_error(kind, code, message);
    }
}

template<error_effect effect = error_effect::may_run_user_code, class operation>
int invoke_checked(operation&& run,
                   tx::error_kind fallback = tx::error_kind::runtime,
                   runtime_context* known_context = nullptr) noexcept
{
    try
    {
        std::forward<operation>(run)();
        // 回调中的 TX 函数可能通过状态返回；成功不清空错误，避免丢失析构错误。
        if constexpr (effect == error_effect::may_run_user_code)
        {
            const auto& context = known_context ? *known_context :
                current_runtime_context();
            return static_cast<int>(context.last_error_kind);
        }
        return 0;
    }
    catch (const runtime_failure& error)
    {
        set_checked_error(known_context, error.error().kind,
                          error.error().code.c_str(), error.what());
    }
    catch (const std::bad_alloc& error)
    {
        set_checked_error(known_context, tx::error_kind::runtime,
                          "allocation_failed", error.what());
    }
    catch (const std::exception& error)
    {
        set_checked_error(known_context, fallback,
                          "operation_failed", error.what());
    }
    catch (...)
    {
        set_checked_error(known_context, tx::error_kind::runtime,
                          "unknown_error", "未知运行时错误");
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
