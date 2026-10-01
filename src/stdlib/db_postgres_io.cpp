#ifdef _WIN32
#include <winsock2.h>
#else
#include <poll.h>
#include <cerrno>
#endif

#include "stdlib/db_internal.hpp"
#include "stdlib/db_operation.hpp"

#include <algorithm>

namespace tx_generated
{
namespace
{

const char* stable_code(std::string_view state, bool opening)
{
    for (const auto& [prefix, code] : {
        std::pair{"28", "authentication_failed"}, {"23", "constraint_violation"},
        {"25P02", "transaction_failed"}, {"25006", "read_only"}, {"40", "conflict"},
        {"57014", "cancelled"}, {"55P03", "busy"}, {"53", "limit_exceeded"},
        {"54", "limit_exceeded"}})
    {
        if (state.starts_with(prefix))
        {
            return code;
        }
    }
    return opening ? "connection_failed" : "query_failed";
}

} // namespace

void db_pg_deleter::operator()(PGconn* value) const noexcept
{
    PQfinish(value);
}

void db_pg_result_deleter::operator()(PGresult* value) const noexcept
{
    PQclear(value);
}

[[noreturn]] void db_pg_lost(db_connection_state& connection, const char* code)
{
    connection.postgres.reset();
    connection.cleanup_failed = true;
    db_fail(code, "PostgreSQL 连接不可继续使用；已发送操作的外部效果可能无法确定");
}

void db_pg_wait(db_connection_state& connection, bool writing)
{
    for (;;)
    {
        db_pg_check_operation(connection);
        const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(
            connection.operation_deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0)
        {
            db_pg_lost(connection, "timeout");
        }
        const auto socket = PQsocket(connection.postgres.get());
        if (socket < 0)
        {
            db_pg_lost(connection);
        }
        const auto slice = db_current_operation ? std::min<std::int64_t>(remaining, 10000) : remaining;
#ifdef _WIN32
        fd_set descriptors;
        FD_ZERO(&descriptors);
        FD_SET(socket, &descriptors);
        // Windows 非阻塞 connect 失败通过 exceptfds 唤醒，不能把轮询间隔误当成就绪。
        auto exceptional = descriptors;
        timeval timeout{static_cast<long>(slice / 1000000), static_cast<long>(slice % 1000000)};
        const auto status = select(socket + 1, writing ? nullptr : &descriptors,
            writing ? &descriptors : nullptr, &exceptional, &timeout);
#else
        pollfd descriptor{socket, static_cast<short>(writing ? POLLOUT : POLLIN), 0};
        const auto status = poll(&descriptor, 1, static_cast<int>(std::min<std::int64_t>(
            (slice + 999) / 1000, 2147483647)));
        if (status < 0 && errno == EINTR)
        {
            continue;
        }
#endif
        if (status < 0)
        {
            db_pg_lost(connection);
        }
        if (status > 0)
        {
            return;
        }
    }
}

void db_pg_start(db_connection_state& connection)
{
    db_pg_check_operation(connection);
    if (!connection.postgres || PQstatus(connection.postgres.get()) != CONNECTION_OK)
    {
        db_pg_lost(connection);
    }
    connection.operation_deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(connection.operation_timeout_ms);
    if (connection.connect_deadline)
    {
        connection.operation_deadline = std::min(connection.operation_deadline,
                                                  *connection.connect_deadline);
    }
}

db_pg_result db_pg_receive(db_connection_state& connection)
{
    auto* native = connection.postgres.get();
    for (;;)
    {
        const auto flush = PQflush(native);
        if (flush < 0)
        {
            db_pg_lost(connection);
        }
        if (flush == 0)
        {
            break;
        }
        db_pg_wait(connection, true);
    }
    while (PQisBusy(native))
    {
        db_pg_wait(connection, false);
        if (!PQconsumeInput(native))
        {
            db_pg_lost(connection);
        }
    }
    db_pg_result result(PQgetResult(native));
    if (PQstatus(native) != CONNECTION_OK)
    {
        const auto* state = result ? PQresultErrorField(result.get(), PG_DIAG_SQLSTATE) : nullptr;
        connection.last_sqlstate = state ? state : "";
        db_pg_lost(connection);
    }
    return result;
}

void db_pg_check(db_connection_state& connection, PGresult* result, bool opening)
{
    if (!result)
    {
        db_pg_lost(connection);
    }
    const auto status = PQresultStatus(result);
    if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK ||
        status == PGRES_SINGLE_TUPLE)
    {
        return;
    }
    if (status == PGRES_COPY_IN || status == PGRES_COPY_OUT || status == PGRES_COPY_BOTH)
    {
        db_pg_lost(connection, "invalid_state");
    }
    const auto* raw = PQresultErrorField(result, PG_DIAG_SQLSTATE);
    connection.last_sqlstate = raw ? raw : "";
    const auto& state = connection.last_sqlstate;
    const auto code = stable_code(state, opening);
    if (state.starts_with("08") || state == "57P01" || state == "57P02")
    {
        db_pg_lost(connection);
    }
    // 错误结果后的终止包也必须消费，才允许使用保存点恢复事务。
    while (db_pg_receive(connection))
    {
    }
    db_fail(code, "PostgreSQL 操作失败；可通过 sqlstate 查询服务器错误类别");
}

void db_pg_control(db_connection_state& connection, const std::string& sql)
{
    db_pg_start(connection);
    if (!PQsendQueryParams(connection.postgres.get(), sql.c_str(), 0,
                            nullptr, nullptr, nullptr, nullptr, 0))
    {
        db_pg_lost(connection);
    }
    auto result = db_pg_receive(connection);
    db_pg_check(connection, result.get());
    while (db_pg_receive(connection))
    {
    }
}

} // namespace tx_generated
