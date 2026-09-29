#pragma once

#include "stdlib/cancellation.hpp"
#include <cstdint>

namespace tx_generated
{

// 仅由异步工作项在自己的线程上安装；同步入口不改变原有等待语义。
struct db_operation_context
{
    std::shared_ptr<cancellation_state> token;
    std::shared_ptr<cancellation_state> scope;
    std::chrono::steady_clock::time_point deadline;
};

extern thread_local const db_operation_context* db_current_operation;
const char* db_operation_status();
void db_operation_check();
std::int64_t db_operation_remaining(std::int64_t maximum);

} // namespace tx_generated
