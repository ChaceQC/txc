#include "backend/cpp/runtime_abi_internal.hpp"

#include <cstdint>
#include <stdexcept>

namespace tx_generated::detail
{
namespace
{

// 容器槽位保留一个指针宽度；最低位标记借用的内容指针，以区别线程本地根。
text_payload* payload_from(const void* value) noexcept
{
    const auto address = reinterpret_cast<std::uintptr_t>(value);
    if ((address & 1U) != 0)
    {
        return reinterpret_cast<text_payload*>(address & ~std::uintptr_t{1});
    }
    return static_cast<const text_handle_record*>(value)->content;
}

} // namespace

static_assert(alignof(text_payload) > 3);
static_assert(alignof(text_handle_record) > 3);
static_assert(alignof(std::string) > 3);

const void* borrowed_string_pointer(const std::string& value) noexcept
{
    return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(&value) | std::uintptr_t{2});
}

const void* borrowed_text_pointer(const text_payload* value) noexcept
{
    if (!value)
    {
        return nullptr;
    }
    return reinterpret_cast<const void*>(
        reinterpret_cast<std::uintptr_t>(value) | std::uintptr_t{1});
}

void retain_text_payload(text_payload* value) noexcept
{
    if (value)
    {
        value->references.fetch_add(1, std::memory_order_relaxed);
    }
}

void release_text_payload(text_payload* value) noexcept
{
    if (value && value->references.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        delete static_cast<owned_text_handle_record*>(value->owner);
    }
}

text_payload* retain_text_reference(const void* value) noexcept
{
    if (!value)
    {
        return nullptr;
    }
    auto* content = payload_from(value);
    retain_text_payload(content);
    return content;
}

void release_text_reference(const void* value) noexcept
{
    if (value)
    {
        release_text_payload(payload_from(value));
    }
}

const std::string& text_value(const void* value) noexcept
{
    const auto address = reinterpret_cast<std::uintptr_t>(value);
    if ((address & 3U) == 2)
    {
        return *reinterpret_cast<const std::string*>(address & ~std::uintptr_t{3});
    }
    return payload_from(value)->value;
}

text_handle_record* copy_text_handle(const void* value)
{
    if ((reinterpret_cast<std::uintptr_t>(value) & 3U) == 2)
    {
        return make_handle<std::string>(text_value(value));
    }
    if ((reinterpret_cast<std::uintptr_t>(value) & 1U) == 0)
    {
        publish_text_builder(const_cast<void*>(value));
    }
    auto* content = payload_from(value);
    auto result = std::make_unique<text_handle_record>(content);
    register_handle(result.get(), handle_kind::text);
    return result.release();
}

void release_text_root(text_handle_record* value) noexcept
{
    if (value->owns_content)
    {
        release_text_payload(value->content);
    }
    else
    {
        delete value;
    }
}

text_handle_record* make_text_builder()
{
    auto* result = make_handle<std::string>();
    result->building = true;
    return result;
}

std::string& text_builder(void* value)
{
    auto* root = static_cast<text_handle_record*>(value);
    if (!root->building ||
        root->content->references.load(std::memory_order_relaxed) != 1)
    {
        throw std::logic_error("已发布文本不能继续追加");
    }
    return root->content->value;
}

void publish_text_builder(void* value) noexcept
{
    static_cast<text_handle_record*>(value)->building = false;
}

} // namespace tx_generated::detail
