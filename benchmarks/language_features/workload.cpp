#include "workload.hpp"

namespace workload
{

std::int64_t bump(std::int64_t value)
{
    return value + 7;
}

std::int64_t measure(const pair& value)
{
    return value->left + value->right;
}

} // namespace workload
