#pragma once

#include "stdlib/task.hpp"

#include <memory>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace tx_generated
{

#ifdef _WIN32
class task_io_operation
{
public:
    task_io_operation() = default;
    virtual ~task_io_operation();
    task_io_operation(const task_io_operation&) = delete;
    task_io_operation& operator=(const task_io_operation&) = delete;

    [[nodiscard]] virtual DWORD begin() noexcept = 0;
    virtual void complete(DWORD bytes, DWORD error) noexcept = 0;
    [[nodiscard]] bool should_cancel() const;
    void close_file() noexcept;

    OVERLAPPED overlapped{};
    HANDLE file = INVALID_HANDLE_VALUE;
    std::shared_ptr<task_scope_state> scope;
    std::shared_ptr<task_state> child;
    std::shared_ptr<cancellation_state> token;
    bool cancel_requested = false;
};

void submit_io_operation(std::shared_ptr<task_io_operation> operation);
#endif

} // namespace tx_generated
