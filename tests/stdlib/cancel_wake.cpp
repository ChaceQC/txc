#include "backend/cpp/cancel_abi.hpp"
#include "backend/cpp/value_abi.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

int main()
{
    void* owner = nullptr;
    void* token = nullptr;
    if (txrt_cancel_source(&owner) != 0 ||
        txrt_cancel_token(owner, &token) != 0)
    {
        return 1;
    }
    std::atomic<bool> started = false;
    std::int64_t outcome = -1;
    int wait_status = -1;
    std::thread worker([&]
    {
        started.store(true, std::memory_order_release);
        wait_status = txrt_cancel_wait(token, 5000, &outcome);
    });
    while (!started.load(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    bool changed = false;
    const int cancel_status = txrt_cancel_cancel(owner, &changed);
    worker.join();
    txrt_value_release(token);
    txrt_value_release(owner);
    if (cancel_status != 0 || !changed || wait_status != 0 || outcome != 1)
    {
        std::cerr << "取消没有唤醒等待者\n";
        return 1;
    }

    void* deadline_owner = nullptr;
    void* deadline_token = nullptr;
    if (txrt_cancel_with_deadline_ms(10, &deadline_owner) != 0 ||
        txrt_cancel_token(deadline_owner, &deadline_token) != 0)
    {
        return 1;
    }
    outcome = -1;
    const int deadline_status = txrt_cancel_wait(deadline_token, -1, &outcome);
    txrt_value_release(deadline_token);
    txrt_value_release(deadline_owner);
    if (deadline_status != 0 || outcome != 2)
    {
        std::cerr << "截止时间没有唤醒等待者\n";
        return 1;
    }
    std::cout << "取消唤醒与截止时间通过\n";
    return 0;
}
