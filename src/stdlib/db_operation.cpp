#include "stdlib/db_operation.hpp"
#include "stdlib/db.hpp"
#include <algorithm>

namespace tx_generated
{

thread_local const db_operation_context* db_current_operation = nullptr;

const char* db_operation_status()
{
    if (!db_current_operation)
    {
        return nullptr;
    }
    for (const auto& state : {db_current_operation->token, db_current_operation->scope})
    {
        if (state)
        {
            std::lock_guard lock(state->mutex);
            if (state->cancelled)
            {
                return "cancelled";
            }
            if (state->deadline && std::chrono::steady_clock::now() >= *state->deadline)
            {
                return "timeout";
            }
        }
    }
    return std::chrono::steady_clock::now() >= db_current_operation->deadline ? "timeout" : nullptr;
}

void db_operation_check()
{
    if (const auto code = db_operation_status())
    {
        db_fail(code, "异步数据库操作已取消或超时；若 COMMIT 已发送，提交结果可能无法确定");
    }
}

std::int64_t db_operation_remaining(std::int64_t maximum)
{
    db_operation_check();
    if (!db_current_operation)
    {
        return maximum;
    }
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        db_current_operation->deadline - std::chrono::steady_clock::now()).count();
    return std::max<std::int64_t>(1, std::min(maximum, remaining));
}

} // namespace tx_generated
