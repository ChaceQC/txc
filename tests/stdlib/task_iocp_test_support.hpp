#pragma once

#include "backend/cpp/task_iocp.hpp"
#include "backend/cpp/task_runtime_internal.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace tx_iocp_test
{

using tx_generated::cancellation_state;
using tx_generated::task_io_operation;

enum class submit_mode
{
    direct,
    wait_before_submit,
    return_success_after_completion
};

class pipe_read_operation final : public task_io_operation
{
public:
    pipe_read_operation(submit_mode mode, DWORD forced_error)
        : mode_(mode), forced_error_(forced_error)
    {
    }

    DWORD begin() noexcept override
    {
        const auto cancelled_before_submit = should_cancel();
        {
            std::lock_guard lock(mutex_);
            preflight_finished_ = true;
            preflight_cancelled_ = cancelled_before_submit;
        }
        changed_.notify_all();

        if (mode_ == submit_mode::wait_before_submit)
        {
            std::unique_lock lock(mutex_);
            changed_.wait(lock, [&] { return submit_released_; });
        }
        if (cancelled_before_submit)
        {
            submit_result_ = ERROR_OPERATION_ABORTED;
            return ERROR_OPERATION_ABORTED;
        }
        if (forced_error_ != ERROR_SUCCESS)
        {
            submit_result_ = forced_error_;
            return forced_error_;
        }

        const auto started = ReadFile(file, buffer_, sizeof(buffer_), nullptr,
            &overlapped);
        const auto result = started ? ERROR_SUCCESS : GetLastError();
        if (mode_ == submit_mode::return_success_after_completion &&
            result == ERROR_IO_PENDING)
        {
            {
                std::lock_guard lock(mutex_);
                kernel_submit_returned_ = true;
            }
            changed_.notify_all();
            std::unique_lock lock(mutex_);
            changed_.wait(lock, [&] { return success_return_released_; });
            submit_result_ = ERROR_SUCCESS;
            return ERROR_SUCCESS;
        }
        submit_result_ = result;
        return submit_result_;
    }

    void complete(DWORD bytes, DWORD error) noexcept override
    {
        close_file();
        bool state_completed = true;
        try
        {
            tx_generated::complete_task(child, {}, {});
            std::lock_guard child_lock(child->mutex);
            state_completed = child->completed;
        }
        catch (...)
        {
            state_completed = false;
        }
        {
            std::lock_guard lock(mutex_);
            ++completion_count_;
            completion_bytes_ = bytes;
            completion_error_ = error;
            child_completed_ = state_completed;
        }
        changed_.notify_all();
    }

    bool wait_for_preflight(std::chrono::milliseconds timeout)
    {
        std::unique_lock lock(mutex_);
        return changed_.wait_for(lock, timeout,
            [&] { return preflight_finished_; });
    }

    bool wait_for_kernel_submit(std::chrono::milliseconds timeout)
    {
        std::unique_lock lock(mutex_);
        return changed_.wait_for(lock, timeout,
            [&] { return kernel_submit_returned_; });
    }

    bool wait_for_cancel_request(std::chrono::milliseconds timeout) const
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (cancel_requested.load())
            {
                return true;
            }
            Sleep(1);
        }
        return cancel_requested.load();
    }

    bool wait_for_completion_packet(std::chrono::milliseconds timeout) const
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (completion_received.load())
            {
                return true;
            }
            Sleep(1);
        }
        return completion_received.load();
    }

    void release_submit()
    {
        {
            std::lock_guard lock(mutex_);
            submit_released_ = true;
        }
        changed_.notify_all();
    }

    void release_success_return()
    {
        {
            std::lock_guard lock(mutex_);
            success_return_released_ = true;
        }
        changed_.notify_all();
    }

    bool wait_for_completion(std::chrono::milliseconds timeout)
    {
        std::unique_lock lock(mutex_);
        return changed_.wait_for(lock, timeout,
            [&] { return completion_count_ != 0; });
    }

    bool wait_for_quiet(std::chrono::milliseconds duration)
    {
        std::unique_lock lock(mutex_);
        changed_.wait_for(lock, duration,
            [&] { return completion_count_ > 1; });
        return completion_count_ == 1;
    }

    [[nodiscard]] bool preflight_cancelled() const
    {
        std::lock_guard lock(mutex_);
        return preflight_cancelled_;
    }

    [[nodiscard]] bool child_completed() const
    {
        std::lock_guard lock(mutex_);
        return child_completed_;
    }

    [[nodiscard]] unsigned int completion_count() const
    {
        std::lock_guard lock(mutex_);
        return completion_count_;
    }

    [[nodiscard]] DWORD completion_bytes() const
    {
        std::lock_guard lock(mutex_);
        return completion_bytes_;
    }

    [[nodiscard]] DWORD completion_error() const
    {
        std::lock_guard lock(mutex_);
        return completion_error_;
    }

    [[nodiscard]] DWORD submit_result() const noexcept
    {
        return submit_result_;
    }

    [[nodiscard]] bool file_closed() const
    {
        std::lock_guard lock(mutex_);
        return file == INVALID_HANDLE_VALUE;
    }

private:
    submit_mode mode_ = submit_mode::direct;
    DWORD forced_error_ = ERROR_SUCCESS;
    char buffer_[8]{};
    DWORD submit_result_ = ERROR_SUCCESS;
    mutable std::mutex mutex_;
    std::condition_variable changed_;
    bool preflight_finished_ = false;
    bool preflight_cancelled_ = false;
    bool submit_released_ = false;
    bool kernel_submit_returned_ = false;
    bool success_return_released_ = false;
    unsigned int completion_count_ = 0;
    DWORD completion_bytes_ = 0;
    DWORD completion_error_ = ERROR_SUCCESS;
    bool child_completed_ = false;
};

} // namespace tx_iocp_test
