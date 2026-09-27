#pragma once

#include "stdlib/bytes.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace tx_generated::secret
{

class buffer
{
public:
    explicit buffer(std::size_t size);
    ~buffer();

    buffer(const buffer&) = delete;
    buffer& operator=(const buffer&) = delete;

    [[nodiscard]] std::span<std::uint8_t> writable();
    [[nodiscard]] std::span<const std::uint8_t> view() const;
    void close() noexcept;

private:
    std::vector<std::uint8_t> data_;
    bool locked_ = false;
    bool closed_ = false;
};

using handle = std::shared_ptr<buffer>;

handle from_bytes(const byte_value& data);
handle random(std::int64_t count);
byte_value to_bytes(const handle& value);
std::int64_t size(const handle& value);
bool equal(const handle& left, const handle& right);
void close(const handle& value) noexcept;

} // namespace tx_generated::secret
