#pragma once

#include "common/error_kind.hpp"
#include "backend/cpp/runtime_context.hpp"
#include "backend/cpp/concurrency_result.hpp"
#include "stdlib/cancellation.hpp"

#include <any>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <variant>
#include <vector>

namespace tx_generated
{

struct task_error
{
    tx::error_kind kind = tx::error_kind::none;
    std::string code;
    std::string message;
    std::vector<detail::source_frame> stack;
};

struct task_state
{
    task_state();
    ~task_state();

    std::mutex mutex;
#ifdef _WIN32
    void* completed_event = nullptr;
#else
    std::condition_variable completed_signal;
#endif
    concurrent_result result;
    task_error error;
    bool completed = false;
    bool observed = false;
};

struct task_scope_state
{
    std::mutex mutex;
    std::vector<std::shared_ptr<task_state>> children;
    std::shared_ptr<cancellation_state> cancellation;
    std::size_t maximum = 0;
    bool closed = false;
};

struct task_scope_handle
{
    std::shared_ptr<task_scope_state> state;
};

struct task_handle
{
    std::shared_ptr<task_state> state;
    std::int64_t kind = 0;
    std::string type_name;
};

[[nodiscard]] std::shared_ptr<task_scope_state> current_task_scope();
void set_current_task_scope(std::shared_ptr<task_scope_state> scope);
[[nodiscard]] bool task_cancelled(task_scope_state& scope);
bool cancel_task_scope(const std::shared_ptr<task_scope_state>& scope);
[[nodiscard]] std::shared_ptr<task_state> submit_task(
    const std::shared_ptr<task_scope_state>& scope, std::any callback,
    std::int64_t kind);
[[nodiscard]] std::shared_ptr<task_state> schedule_task_timer(
    const std::shared_ptr<task_scope_state>& scope, std::int64_t delay_ms);
void wait_task(const std::shared_ptr<task_state>& state);
[[nodiscard]] task_error close_task_scope(
    const std::shared_ptr<task_scope_state>& scope, bool cancel_first);

} // namespace tx_generated
