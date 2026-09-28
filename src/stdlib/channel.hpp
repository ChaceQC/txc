#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>

namespace tx_generated
{

template<class value_type>
struct channel_state
{
    std::mutex mutex;
    std::condition_variable can_send;
    std::condition_variable can_recv;
    std::deque<value_type> queue;
    std::size_t capacity = 0;
    std::string type_name;
    bool closed = false;
};

} // namespace tx_generated
