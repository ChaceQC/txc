#pragma once

#include <string>
#include <utility>

namespace tx_generated
{

namespace detail
{
void retain_text_reference(const void* value) noexcept;
void release_text_reference(const void* value) noexcept;
}

// 只持有文本载荷，不登记为错误退出时独立清理的根句柄。
class text_reference
{
public:
    explicit text_reference(const void* value = nullptr) noexcept
        : value_(static_cast<const std::string*>(value))
    {
        detail::retain_text_reference(value_);
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
        return *value_;
    }
    [[nodiscard]] const void* handle() const noexcept
    {
        return value_;
    }

private:
    const std::string* value_;
};

static_assert(sizeof(text_reference) == sizeof(void*));

} // namespace tx_generated
