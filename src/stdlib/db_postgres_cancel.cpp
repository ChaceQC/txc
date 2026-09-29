#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#include "stdlib/db_internal.hpp"
#include "stdlib/db_operation.hpp"

namespace tx_generated
{

void db_pg_cancel_pending(db_connection_state& connection) noexcept
{
    if (!connection.postgres || PQtransactionStatus(connection.postgres.get()) != PQTRANS_ACTIVE)
    {
        return;
    }
    // libpq 17+ 独立取消连接沿用 TLS 配置；取消传输本身也必须有界。
    std::unique_ptr<PGcancelConn, decltype(&PQcancelFinish)> cancellation(
        PQcancelCreate(connection.postgres.get()), &PQcancelFinish);
    if (!cancellation || !PQcancelStart(cancellation.get()))
    {
        return;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto status = PQcancelPoll(cancellation.get());
        if (status == PGRES_POLLING_OK || status == PGRES_POLLING_FAILED)
        {
            return;
        }
        const auto socket = PQcancelSocket(cancellation.get());
        if (socket < 0)
        {
            return;
        }
        fd_set descriptors;
        FD_ZERO(&descriptors);
        FD_SET(socket, &descriptors);
        auto exceptional = descriptors;
        timeval timeout{0, 10000};
        const bool writing = status == PGRES_POLLING_WRITING;
        if (select(socket + 1, writing ? nullptr : &descriptors,
            writing ? &descriptors : nullptr, &exceptional, &timeout) < 0)
        {
            return;
        }
    }
}

void db_pg_check_operation(db_connection_state& connection)
{
    if (const auto code = db_operation_status())
    {
        db_pg_cancel_pending(connection);
        db_pg_lost(connection, code);
    }
}

} // namespace tx_generated
