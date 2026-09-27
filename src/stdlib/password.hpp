#pragma once

#include "stdlib/secret.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace tx_generated::password
{

std::string hash_password(const secret::handle& value);
std::string hash_password_with_params(const secret::handle& value,
                                      std::int64_t memory_kib,
                                      std::int64_t iterations,
                                      std::int64_t parallelism);
bool verify_password(const secret::handle& value, std::string_view encoded);
bool needs_rehash(std::string_view encoded, std::int64_t memory_kib,
                  std::int64_t iterations, std::int64_t parallelism);

} // namespace tx_generated::password
