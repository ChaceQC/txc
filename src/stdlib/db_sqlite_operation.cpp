#include "stdlib/db_internal.hpp"
#include "stdlib/db_operation.hpp"

namespace tx_generated
{

void db_install_sqlite_operation(const db_connection& connection, std::int64_t busy_timeout_ms)
{
    if (!db_current_operation)
    {
        return;
    }
    connection->operation_timeout_ms = busy_timeout_ms;
    sqlite3_progress_handler(connection->native.get(), 1000, [](void*) -> int
    {
        return db_operation_status() ? 1 : 0;
    }, nullptr);
    sqlite3_busy_handler(connection->native.get(), [](void* value, int attempts) -> int
    {
        auto& state = *static_cast<db_connection_state*>(value);
        if (db_operation_status() || static_cast<std::int64_t>(attempts) * 5 >= state.operation_timeout_ms)
        {
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return db_operation_status() ? 0 : 1;
    }, connection.get());
}

} // namespace tx_generated
