#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace tx_generated
{

// 不可变字节载荷；短值和共享计数一次分配，长值接管已有缓冲区。
class byte_storage
{
public:
    explicit byte_storage(std::vector<std::uint8_t> value, bool preserve_buffer = false) : size_(value.size())
    {
        if (!preserve_buffer && size_ <= local_.size())
        {
            std::copy(value.begin(), value.end(), local_.begin());
        }
        else
        {
            large_ = std::move(value);
        }
    }

    byte_storage(std::span<const std::uint8_t> value, std::span<const std::uint8_t> prefix = {})
        : size_(prefix.size() + value.size())
    {
        if (size_ > local_.size())
        {
            large_.resize(size_);
        }
        auto* output = large_.empty() ? local_.data() : large_.data();
        std::copy(prefix.begin(), prefix.end(), output);
        std::copy(value.begin(), value.end(), output + prefix.size());
    }

    const std::uint8_t* data() const noexcept
    {
        return large_.empty() ? local_.data() : large_.data();
    }
    std::size_t size() const noexcept
    {
        return size_;
    }
    bool empty() const noexcept
    {
        return size_ == 0;
    }
    const std::uint8_t* begin() const noexcept
    {
        return data();
    }
    const std::uint8_t* end() const noexcept
    {
        return data() + size_;
    }
    std::uint8_t operator[](std::size_t index) const noexcept
    {
        return data()[index];
    }
    std::uint8_t front() const noexcept
    {
        return data()[0];
    }
    std::uint8_t back() const noexcept
    {
        return data()[size_ - 1];
    }
    std::uint8_t at(std::size_t index) const
    {
        if (index >= size_)
        {
            throw std::out_of_range("字节值索引越界");
        }
        return data()[index];
    }
    const std::uint8_t* cbegin() const noexcept
    {
        return begin();
    }
    const std::uint8_t* cend() const noexcept
    {
        return end();
    }
    operator std::vector<std::uint8_t>() const
    {
        return {begin(), end()};
    }
    bool operator==(const byte_storage& other) const
    {
        return std::equal(begin(), end(), other.begin(), other.end());
    }

private:
    std::array<std::uint8_t, 32> local_{};
    std::vector<std::uint8_t> large_;
    std::size_t size_;
};

} // namespace tx_generated
