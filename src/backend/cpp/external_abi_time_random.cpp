#include "backend/cpp/external_abi_time_random.hpp"

#include "stdlib/stdlib.hpp"

#include <any>
#include <stdexcept>
#include <string>
#include <string_view>

namespace tx_generated::detail
{
namespace
{

template<class value_type>
value_type argument(const void* const* arguments, std::size_t index)
{
    return std::any_cast<const value_type&>(
        *static_cast<const std::any*>(arguments[index]));
}

} // namespace

std::any dispatch_time_random(std::string_view name,
                              const void* const* arguments)
{
    if (name == "time.unix_millis")
    {
        return tx_fn_unix_millis();
    }
    if (name == "time.monotonic_millis")
    {
        return tx_fn_monotonic_millis();
    }
    if (name == "time.sleep_millis")
    {
        tx_fn_sleep_millis(argument<tx_int>(arguments, 0));
        return {};
    }
    if (name == "random.seed")
    {
        tx_fn_seed(argument<tx_int>(arguments, 0));
        return {};
    }
    if (name == "random.random_int")
    {
        return tx_fn_random_int(argument<tx_int>(arguments, 0),
                                argument<tx_int>(arguments, 1));
    }
    if (name == "random.random_float")
    {
        return tx_fn_random_float();
    }
    throw std::runtime_error("未知时间或随机数函数：" + std::string(name));
}

} // namespace tx_generated::detail
