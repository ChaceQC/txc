#pragma once

#include <cstdint>
#include <memory>

namespace workload
{

struct pair_data
{
    std::int64_t left;
    std::int64_t right;
};

using pair = std::shared_ptr<pair_data>;

std::int64_t bump(std::int64_t value);
std::int64_t measure(const pair& value);

} // namespace workload
