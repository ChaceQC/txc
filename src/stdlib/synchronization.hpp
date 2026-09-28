#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <type_traits>

namespace tx_generated
{

template<class value_type>
struct mutex_state
{
    std::mutex mutex;
    value_type value;
    std::string type_name;
};

template<class value_type>
struct mutex_guard_state
{
    std::shared_ptr<mutex_state<value_type>> owner;
    std::unique_lock<std::mutex> lock;

    explicit mutex_guard_state(std::shared_ptr<mutex_state<value_type>> value)
        : owner(std::move(value)), lock(owner->mutex)
    {
    }
};

template<class value_type>
struct rw_lock_state
{
    std::shared_mutex mutex;
    value_type value;
    std::string type_name;
};

template<class value_type, bool write>
struct rw_guard_state
{
    std::shared_ptr<rw_lock_state<value_type>> owner;
    std::conditional_t<write, std::unique_lock<std::shared_mutex>,
                       std::shared_lock<std::shared_mutex>> lock;

    explicit rw_guard_state(std::shared_ptr<rw_lock_state<value_type>> value)
        : owner(std::move(value)), lock(owner->mutex)
    {
    }
};

struct condition_state
{
    std::condition_variable_any changed;
};

struct semaphore_state
{
    std::mutex mutex;
    std::condition_variable changed;
    std::int64_t count = 0;
    std::int64_t maximum = 0;
};

struct once_state
{
    std::mutex mutex;
    std::condition_variable changed;
    bool running = false;
    bool done = false;
    std::thread::id owner;
};

template<class value_type>
using atomic_state = std::atomic<value_type>;

} // namespace tx_generated
