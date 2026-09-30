#include "backend/cpp/socket_async_loop.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tx_generated::socket_async
{
namespace
{

constexpr std::size_t maximum_operations = 1024;
using clock_type = std::chrono::steady_clock;

struct direction_key
{
    const socket::state* state;
    bool write;

    bool operator==(const direction_key&) const = default;
};

struct direction_hash
{
    std::size_t operator()(direction_key value) const noexcept
    {
        return (reinterpret_cast<std::uintptr_t>(value.state) >> 4) ^
            static_cast<std::size_t>(value.write);
    }
};

task_error cancelled_error(const operation& value)
{
    if (task_cancelled(*value.scope))
    {
        return {tx::error_kind::cancelled, "cancelled",
            "网络任务作用域已取消", {}};
    }
    std::lock_guard lock(value.token->mutex);
    if (value.token->cancelled)
    {
        return {tx::error_kind::cancelled, "cancelled", "网络操作已取消", {}};
    }
    if (value.token->deadline && clock_type::now() >= *value.token->deadline)
    {
        return {tx::error_kind::cancelled, "deadline_exceeded",
            "网络操作截止时间已到", {}};
    }
    return {};
}

void finish(operation& value, task_result result, task_error error) noexcept
{
    if (value.finished)
    {
        return;
    }
    value.finished = true;
    try
    {
        complete_task(value.child, std::move(result), std::move(error));
    }
    catch (...)
    {
        // 进程级分配失败不能从事件循环线程传播。
    }
}

void advance(operation& value, const step_function& step) noexcept
{
    try
    {
        if (auto result = step(value))
        {
            finish(value, std::move(*result), {});
        }
    }
    catch (const runtime_failure& failure)
    {
        const auto& error = failure.error();
        finish(value, {}, {error.kind, error.code, error.message, {}});
    }
    catch (const std::exception& failure)
    {
        finish(value, {}, {tx::error_kind::io,
            "operation_failed", failure.what(), {}});
    }
    catch (...)
    {
        finish(value, {}, {tx::error_kind::io,
            "operation_failed", "异步网络操作失败", {}});
    }
}

bool check_terminal(operation& value) noexcept
{
    if (value.state && value.state->closed)
    {
        finish(value, {}, {tx::error_kind::io,
            value.state->kind == socket::resource_kind::tcp_stream
                ? "connection_closed" : "closed_handle",
            "网络句柄已关闭", {}});
        return true;
    }
    if (auto error = cancelled_error(value); error.kind != tx::error_kind::none)
    {
        finish(value, {}, std::move(error));
        return true;
    }
    if (value.initialized && clock_type::now() >= value.deadline)
    {
        finish(value, {}, {tx::error_kind::io, "timeout",
            "等待网络操作超时", {}});
        return true;
    }
    return false;
}

class network_loop
{
public:
    network_loop() : worker_([this]
    {
        run();
    })
    {
    }

    ~network_loop()
    {
        {
            std::lock_guard lock(mutex_);
            stopping_ = true;
        }
        changed_.notify_one();
        worker_.join();
    }

    void add(std::shared_ptr<operation> value)
    {
        {
            std::lock_guard lock(mutex_);
            if (stopping_ || operations_.size() >= maximum_operations)
            {
                network::fail("network_queue_full",
                    "异步网络操作登记表已满");
            }
            operations_.push_back(std::move(value));
        }
        changed_.notify_one();
    }

private:
    void run() noexcept
    {
        while (true)
        {
            std::vector<std::shared_ptr<operation>> current;
            bool stopping = false;
            {
                std::unique_lock lock(mutex_);
                changed_.wait(lock, [&]
                {
                    return stopping_ || !operations_.empty();
                });
                stopping = stopping_;
                if (stopping && operations_.empty())
                {
                    return;
                }
                current = operations_;
            }
            if (stopping)
            {
                for (const auto& value : current)
                {
                    finish(*value, {}, {tx::error_kind::cancelled,
                        "cancelled", "网络事件循环已停止", {}});
                }
            }
            else
            {
                process(current);
            }
            {
                std::lock_guard lock(mutex_);
                std::erase_if(operations_, [](const auto& value)
                {
                    return value->finished;
                });
            }
        }
    }

    static void process(const std::vector<std::shared_ptr<operation>>& current)
    {
        // 快照持有每项操作的 socket 和缓冲；等待期间不占通用任务线程。
        std::vector<WSAPOLLFD> descriptors;
        std::vector<std::shared_ptr<operation>> polled;
        descriptors.reserve(current.size());
        polled.reserve(current.size());
        for (const auto& value : current)
        {
            if (check_terminal(*value))
            {
                continue;
            }
            if (!value->initialized)
            {
                value->initialized = true;
                advance(*value, value->start);
                if (!value->finished)
                {
                    value->deadline = clock_type::now() +
                        std::chrono::milliseconds(value->timeout_ms);
                }
            }
            if (!value->finished && value->native)
            {
                descriptors.push_back({value->native->get(),
                    static_cast<short>(value->write ? POLLWRNORM : POLLRDNORM),
                    0});
                polled.push_back(value);
            }
        }
        if (descriptors.empty())
        {
            return;
        }
        const int count = WSAPoll(descriptors.data(),
            static_cast<ULONG>(descriptors.size()), 10);
        if (count == SOCKET_ERROR)
        {
            const auto code = WSAGetLastError();
            task_error failure{tx::error_kind::io, "operation_failed",
                "等待网络就绪失败，Winsock 错误码 " +
                    std::to_string(code), {}};
            try
            {
                WSASetLastError(code);
                socket::socket_error("等待网络就绪");
            }
            catch (const runtime_failure& error)
            {
                const auto& detail = error.error();
                failure = {detail.kind, detail.code, detail.message, {}};
            }
            for (const auto& value : polled)
            {
                finish(*value, {}, failure);
            }
            return;
        }
        std::unordered_set<direction_key, direction_hash> occupied;
        // 同一方向只推进登记顺序中的首项，反向操作可以同时就绪。
        for (std::size_t index = 0; index < polled.size(); ++index)
        {
            auto& value = *polled[index];
            if (check_terminal(value))
            {
                continue;
            }
            if (value.state && !occupied.insert(
                    {value.state.get(), value.write}).second)
            {
                continue;
            }
            if (descriptors[index].revents == 0)
            {
                continue;
            }
            std::unique_lock<std::mutex> io_lock;
            if (value.state)
            {
                io_lock = std::unique_lock(value.write
                    ? value.state->write_mutex : value.state->read_mutex,
                    std::try_to_lock);
                if (!io_lock.owns_lock())
                {
                    continue;
                }
            }
            advance(value, value.ready);
        }
    }

    std::mutex mutex_;
    std::condition_variable changed_;
    std::vector<std::shared_ptr<operation>> operations_;
    bool stopping_ = false;
    std::thread worker_;
};

network_loop& instance()
{
    static network_loop value;
    return value;
}

} // namespace

void submit(std::shared_ptr<operation> value)
{
    instance().add(std::move(value));
}

} // namespace tx_generated::socket_async
