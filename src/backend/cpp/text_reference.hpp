#pragma once

#include <string>
#include <utility>

namespace tx_generated
{

namespace detail
{
struct text_payload;
text_payload* retain_text_reference(const void* value) noexcept;
void retain_text_payload(text_payload* value) noexcept;
void release_text_payload(text_payload* value) noexcept;
void release_text_reference(const void* value) noexcept;
const std::string& text_value(const void* value) noexcept;
const void* borrowed_text_pointer(const text_payload* value) noexcept;
}

// 只持有文本载荷，不登记为错误退出时独立清理的根句柄。
class text_reference
{
public:
    explicit text_reference(const void* value = nullptr) noexcept
        : value_(detail::borrowed_text_pointer(
            detail::retain_text_reference(value)))
    {
    }
    text_reference(const text_reference& other) noexcept : text_reference(other.value_)
    {
    }
    text_reference(text_reference&& other) noexcept
        : value_(std::exchange(other.value_, nullptr))
    {
    }
    text_reference& operator=(text_reference other) noexcept
    {
        std::swap(value_, other.value_);
        return *this;
    }
    ~text_reference()
    {
        detail::release_text_reference(value_);
    }
    [[nodiscard]] const std::string& get() const noexcept
    {
        return detail::text_value(value_);
    }
    [[nodiscard]] const void* handle() const noexcept
    {
        return value_;
    }

private:
    const void* value_;
};

static_assert(sizeof(text_reference) == sizeof(void*));

} // namespace tx_generated
