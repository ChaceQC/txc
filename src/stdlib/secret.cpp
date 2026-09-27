#include "stdlib/secret.hpp"

#include "stdlib/crypto_internal.hpp"
#include "stdlib/error.hpp"

#include <mbedtls/platform_util.h>
extern "C"
{
#include <mbedtls/constant_time.h>
}

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace tx_generated::secret
{
namespace
{

constexpr std::size_t max_secret_size = 1024 * 1024;

[[noreturn]] void fail(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::security, code, message});
}

bool lock_pages(void* data, std::size_t size) noexcept
{
    if (size == 0)
    {
        return false;
    }
#ifdef _WIN32
    return VirtualLock(data, size) != 0;
#else
    return mlock(data, size) == 0;
#endif
}

void unlock_pages(void* data, std::size_t size) noexcept
{
#ifdef _WIN32
    (void)VirtualUnlock(data, size);
#else
    (void)munlock(data, size);
#endif
}

std::size_t checked_size(std::int64_t count)
{
    if (count < 0 || count > static_cast<std::int64_t>(max_secret_size))
    {
        fail("size_limit", "秘密字节长度必须在 0 到 1 MiB 之间");
    }
    return static_cast<std::size_t>(count);
}

} // namespace

buffer::buffer(std::size_t size) : data_(size)
{
    locked_ = lock_pages(data_.data(), data_.size());
}

buffer::~buffer()
{
    close();
}

std::span<std::uint8_t> buffer::writable()
{
    if (closed_)
    {
        fail("invalid_state", "秘密字节已关闭");
    }
    return data_;
}

std::span<const std::uint8_t> buffer::view() const
{
    if (closed_)
    {
        fail("invalid_state", "秘密字节已关闭");
    }
    return data_;
}

void buffer::close() noexcept
{
    if (closed_)
    {
        return;
    }
    if (!data_.empty())
    {
        // 在解除页锁之前擦除所有别名共享的同一片存储。
        mbedtls_platform_zeroize(data_.data(), data_.size());
        if (locked_)
        {
            unlock_pages(data_.data(), data_.size());
        }
    }
    closed_ = true;
}

handle from_bytes(const byte_value& data)
{
    if (data->size() > max_secret_size)
    {
        fail("size_limit", "秘密字节长度必须在 0 到 1 MiB 之间");
    }
    auto result = std::make_shared<buffer>(data->size());
    std::copy(data->begin(), data->end(), result->writable().begin());
    return result;
}

handle random(std::int64_t count)
{
    auto result = std::make_shared<buffer>(checked_size(count));
    try
    {
        crypto::fill_random(result->writable());
    }
    catch (const runtime_failure& error)
    {
        fail(error.error().code == "random_failed" ? "random_failed" :
            "operation_failed", "生成秘密随机字节失败");
    }
    return result;
}

byte_value to_bytes(const handle& value)
{
    const auto data = value->view();
    return make_bytes({data.begin(), data.end()});
}

std::int64_t size(const handle& value)
{
    return static_cast<std::int64_t>(value->view().size());
}

bool equal(const handle& left, const handle& right)
{
    const auto left_data = left->view();
    const auto right_data = right->view();
    if (left_data.size() != right_data.size())
    {
        return false;
    }
    static const std::uint8_t empty = 0;
    const auto* a = left_data.empty() ? &empty : left_data.data();
    const auto* b = right_data.empty() ? &empty : right_data.data();
    return mbedtls_ct_memcmp(a, b, left_data.size()) == 0;
}

void close(const handle& value) noexcept
{
    if (value)
    {
        value->close();
    }
}

} // namespace tx_generated::secret
