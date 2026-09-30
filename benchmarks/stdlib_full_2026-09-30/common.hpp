#pragma once
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using bench_clock = std::chrono::steady_clock;

inline void report(const std::string& name, bench_clock::time_point start, std::int64_t checksum)
{
    auto elapsed = std::chrono::duration<double, std::micro>(bench_clock::now() - start).count();
    std::cout << name << '\n' << elapsed << '\n' << checksum << '\n';
    std::cout.flush();
}

inline std::vector<std::uint8_t> read_file(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("fixture unavailable");
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

// 与另外两种语言一样复用单工作线程；测量提交与等待，不反复创建系统线程。
class executor
{
public:
    executor() : worker_([this]
    {
        for (;;)
        {
            std::function<void()> job;
            {
                std::unique_lock lock(mutex_);
                ready_.wait(lock, [this]
                {
                    return stop_ || !jobs_.empty();
                });
                if (stop_ && jobs_.empty())
                {
                    return;
                }
                job = std::move(jobs_.front());
                jobs_.pop();
            }
            job();
        }
    })
    {
    }

    ~executor()
    {
        {
            std::lock_guard lock(mutex_);
            stop_ = true;
        }
        ready_.notify_one();
        worker_.join();
    }

    template<class operation>
    auto submit(operation work)
    {
        using result = decltype(work());
        auto job = std::make_shared<std::packaged_task<result()>>(std::move(work));
        auto future = job->get_future();
        {
            std::lock_guard lock(mutex_);
            jobs_.push([job]
            {
                (*job)();
            });
        }
        ready_.notify_one();
        return future;
    }

private:
    std::mutex mutex_;
    std::condition_variable ready_;
    std::queue<std::function<void()>> jobs_;
    bool stop_ = false;
    std::thread worker_;
};

void bench_database(bool postgres);
void bench_security();
void bench_network();
