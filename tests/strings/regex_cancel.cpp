#include "stdlib/error.hpp"
#include "stdlib/regex.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

int main()
{
    auto cancellation = std::make_shared<tx_generated::cancellation_state>();
    auto pattern = tx_generated::regex_compile("(*NO_START_OPT)(*NO_AUTO_POSSESS)^(a+)+b", "",
        4096, 10'000'000, 100'000, cancellation);
    std::string input(300, 'a');
    std::atomic<bool> started = false;
    std::atomic<bool> finished = false;
    std::atomic<bool> cancelled = false;
    std::string failure_code;
    std::thread worker([&]
    {
        started.store(true);
        try
        {
            (void)tx_generated::regex_search(pattern, input, 0);
        }
        catch (const tx_generated::runtime_failure& error)
        {
            failure_code = error.error().code;
            cancelled.store(error.error().kind == tx::error_kind::cancelled &&
                            error.error().code == "cancelled");
        }
        finished.store(true);
    });
    while (!started.load())
    {
        std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const bool active = !finished.load();
    {
        std::lock_guard lock(cancellation->mutex);
        cancellation->cancelled = true;
    }
    worker.join();
    if (!active || !cancelled.load())
    {
        std::cerr << "active=" << active << " code=" << failure_code << '\n';
    }
    return active && cancelled.load() ? 0 : 1;
}
