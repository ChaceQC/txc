#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>

namespace tx_generated
{

struct cancellation_state
{
    std::mutex mutex;
    std::condition_variable changed;
    bool cancelled = false;
    std::optional<std::chrono::steady_clock::time_point> deadline;
};

struct cancel_source
{
    std::shared_ptr<cancellation_state> state;
};

struct cancel_token
{
    std::shared_ptr<cancellation_state> state;
};

} // namespace tx_generated
