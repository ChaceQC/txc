#pragma once

#include <functional>

namespace tx_generated
{

void enqueue_task(std::function<void()> work);
[[nodiscard]] bool help_task_executor();
[[nodiscard]] bool on_task_executor_thread() noexcept;

} // namespace tx_generated
