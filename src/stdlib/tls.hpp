#pragma once

#include "stdlib/secret.hpp"
#include "stdlib/vector.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace tx_generated::tls
{

struct identity_state
{
    mutable std::mutex mutex;
    std::vector<byte_value> certificates;
    secret::handle private_key;
};

std::int64_t import_identity(const byte_value& package,
                             const secret::handle& password);
std::shared_ptr<const identity_state> get_identity(std::int64_t id);
void close_identity(std::int64_t id) noexcept;

} // namespace tx_generated::tls
