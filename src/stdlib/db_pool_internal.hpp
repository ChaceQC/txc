#pragma once

#include "stdlib/db_internal.hpp"

#include <condition_variable>
#include <mutex>

namespace tx_generated
{

struct db_pool_state
{
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<db_connection> idle;
    std::variant<db_options, db_postgres_options> options;
    secret::handle password;
    std::size_t capacity = 0;
    std::size_t waiter_limit = 0;
    std::size_t total = 0;
    std::size_t waiting = 0;
    bool closed = false;
};

} // namespace tx_generated
