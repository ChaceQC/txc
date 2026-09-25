#include "stdlib/stdlib.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <thread>

namespace tx_generated
{

tx_int tx_fn_unix_millis()
{
    const auto elapsed = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

tx_int tx_fn_monotonic_millis()
{
    const auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

tx_int tx_fn_monotonic_micros()
{
    const auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

void tx_fn_sleep_millis(tx_int duration)
{
    if (duration < 0)
    {
        throw std::runtime_error("sleep_millis 的时长不能为负");
    }
    constexpr tx_int day_in_millis = 24 * 60 * 60 * 1000;
    while (duration > 0)
    {
        // 分段等待，避免底层把极大的毫秒数换算为纳秒时溢出。
        const auto chunk = std::min(duration, day_in_millis);
        std::this_thread::sleep_for(std::chrono::milliseconds(chunk));
        duration -= chunk;
    }
}

} // namespace tx_generated
