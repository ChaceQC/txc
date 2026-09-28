#pragma once

#include "stdlib/task.hpp"

#include <memory>
#include <variant>

namespace tx_generated
{

using task_result = concurrent_result;

[[nodiscard]] std::shared_ptr<task_state> reserve_task(
    const std::shared_ptr<task_scope_state>& scope);
void discard_task(const std::shared_ptr<task_state>& child);
void complete_task(const std::shared_ptr<task_state>& child,
                   task_result result, task_error error);
void wake_task_timers();

} // namespace tx_generated
