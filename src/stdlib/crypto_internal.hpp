#pragma once

#include "stdlib/crypto.hpp"

#include <mbedtls/platform_util.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace tx_generated::crypto
{

// 空输入仍交给底层一个有效地址，避免 API 对空指针的额外约束。
const unsigned char* byte_data(const byte_value& value) noexcept;
bool psa_ready();
void fill_random(std::span<std::uint8_t> output);
[[noreturn]] void fail(const char* code, const char* message);
void require_success(int result, const char* message);

class sensitive_buffer
{
public:
    explicit sensitive_buffer(std::size_t size) : data_(size)
    {
    }

    ~sensitive_buffer()
    {
        if (!data_.empty())
        {
            mbedtls_platform_zeroize(data_.data(), data_.size());
        }
    }

    sensitive_buffer(const sensitive_buffer&) = delete;
    sensitive_buffer& operator=(const sensitive_buffer&) = delete;

    [[nodiscard]] std::uint8_t* data() noexcept
    {
        return data_.data();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return data_.size();
    }

    [[nodiscard]] std::span<std::uint8_t> span() noexcept
    {
        return data_;
    }

    [[nodiscard]] byte_value release()
    {
        // make_shared 先分配控制块；分配失败时仍由析构函数清零原缓冲。
        return std::make_shared<const byte_storage>(std::move(data_), true);
    }

private:
    std::vector<std::uint8_t> data_;
};

} // namespace tx_generated::crypto
