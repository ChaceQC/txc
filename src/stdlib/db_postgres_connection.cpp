#include "stdlib/db_internal.hpp"
#include "common/utf8.hpp"
#include "stdlib/dns.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace tx_generated
{
namespace
{

std::pair<std::string, std::string> resolve_endpoints(const db_postgres_options& options)
{
    std::vector<dns::address> addresses;
    try
    {
        addresses = dns::resolve(options.host, options.connect_timeout_ms, {});
    }
    catch (const runtime_failure& failure)
    {
        db_fail(failure.error().code == "timeout" ? "timeout" : "connection_failed",
                "PostgreSQL 地址解析失败或超时");
    }
    std::pair<std::string, std::string> result;
    for (const auto& address : addresses)
    {
        if (!result.first.empty())
        {
            result.first += ',';
            result.second += ',';
        }
        result.first += options.host;
        result.second += address.ip;
    }
    return result;
}

} // namespace

void db_pg_wait(db_connection_state& connection, bool writing);

void db_pg_validate_options(const db_postgres_options& options)
{
    for (const auto* value : {&options.host, &options.database, &options.user, &options.ca_file})
    {
        if (value->empty() || value->size() > 4096 ||
            value->find('\0') != std::string::npos || !tx::scan_utf8(*value).valid)
        {
            db_fail("invalid_argument", "PostgreSQL 连接字段必须是非空有效 UTF-8");
        }
    }
    if (options.host.find_first_of("/\\,= \t\r\n") != std::string::npos ||
        options.port < 1 || options.port > 65535 ||
        options.connect_timeout_ms < 1 || options.connect_timeout_ms > 2147483647 ||
        options.operation_timeout_ms < 1 || options.operation_timeout_ms > 2147483647 ||
        options.max_value_bytes < 1 || options.max_value_bytes > 67108864)
    {
        db_fail("invalid_argument", "PostgreSQL 地址、端口、超时或字节上限不合法");
    }
}

void db_pg_configure(db_connection_state& connection)
{
    db_pg_control(connection, "SET client_encoding TO 'UTF8'");
    db_pg_control(connection, "SET DateStyle TO 'ISO, YMD'");
    db_pg_control(connection, "SET TimeZone TO 'UTC'");
    db_pg_control(connection, "SET bytea_output TO 'hex'");
    db_pg_control(connection, "SET standard_conforming_strings TO on");
    db_pg_control(connection, std::string("SET default_transaction_read_only TO ") +
        (connection.pg_read_only ? "on" : "off"));
}

db_connection db_open_postgres(const db_postgres_options& options,
                               const secret::handle& password)
{
    db_pg_validate_options(options);
    if (!password)
    {
        db_fail("invalid_argument", "PostgreSQL 密码句柄无效");
    }
    const auto source = password->view();
    if (source.empty() || source.size() > 4096 ||
        std::find(source.begin(), source.end(), 0) != source.end())
    {
        db_fail("invalid_argument", "PostgreSQL 密码必须为 1～4096 字节且不能含 NUL");
    }
    secret::buffer temporary(source.size() + 1);
    std::copy(source.begin(), source.end(), temporary.writable().begin());
    auto connection = std::make_shared<db_connection_state>();
    connection->pg_driver = true;
    connection->pg_read_only = options.read_only;
    connection->value_limit = options.max_value_bytes;
    connection->operation_timeout_ms = options.operation_timeout_ms;
    connection->operation_deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(options.connect_timeout_ms);
    connection->connect_deadline = connection->operation_deadline;
    // 先走已有的有界 DNS，再把数字地址交给 libpq；TLS 仍验证原始主机名。
    const auto [hosts, addresses] = resolve_endpoints(options);
    const auto port = std::to_string(options.port);
    // 展开连接串和环境提供的 TLS 降级均关闭；密码只通过独立参数传入。
    const char* keys[] = {"host", "hostaddr", "port", "dbname", "user", "password",
        "sslmode", "sslrootcert", "ssl_min_protocol_version", "gssencmode",
        "client_encoding", "application_name", "options", "passfile", nullptr};
    const char* values[] = {hosts.c_str(), addresses.c_str(), port.c_str(), options.database.c_str(),
        options.user.c_str(), reinterpret_cast<const char*>(temporary.view().data()),
        "verify-full", options.ca_file.c_str(), "TLSv1.2", "disable", "UTF8", "tx",
        "-c client_encoding=UTF8", "", nullptr};
    connection->postgres.reset(PQconnectStartParams(keys, values, 0));
    if (!connection->postgres)
    {
        throw std::bad_alloc();
    }
    PQsetNoticeProcessor(connection->postgres.get(), [](void*, const char*)
    {
        // NOTICE 也可能带有 SQL 或参数，默认不写入终端。
    }, nullptr);
    // libpq 要求首次 poll 前先等待可写；Windows 未完成 connect 时不能提前检查。
    db_pg_wait(*connection, true);
    for (;;)
    {
        const auto status = PQconnectPoll(connection->postgres.get());
        if (status == PGRES_POLLING_OK)
        {
            break;
        }
        if (status == PGRES_POLLING_FAILED)
        {
            // libpq 的连接阶段没有结构化 SQLSTATE，仅在内部分类且不输出原文。
            const std::string error(PQerrorMessage(connection->postgres.get()));
            const bool tls = error.find("SSL") != std::string::npos ||
                error.find("certificate") != std::string::npos;
            db_fail(tls ? "tls_failed" :
                (PQconnectionNeedsPassword(connection->postgres.get()) ||
                 error.find("authentication") != std::string::npos
                    ? "authentication_failed" : "connection_failed"),
                "PostgreSQL 连接、认证或证书验证失败");
        }
        db_pg_wait(*connection, status == PGRES_POLLING_WRITING);
    }
    temporary.close();
    if (!PQsslInUse(connection->postgres.get()) ||
        PQsetnonblocking(connection->postgres.get(), 1) != 0)
    {
        db_fail("tls_failed", "PostgreSQL 未建立经过验证的 TLS 连接");
    }
    db_pg_configure(*connection);
    connection->connect_deadline.reset();
    return connection;
}

std::string db_sqlstate(const db_connection& connection)
{
    db_check_thread(connection);
    return connection->last_sqlstate;
}

std::string db_transaction_status(const db_connection& connection)
{
    db_check_thread(connection);
    if (!db_connection_open(*connection) || connection->cleanup_failed)
    {
        return "closed";
    }
    if (connection->postgres && PQtransactionStatus(connection->postgres.get()) == PQTRANS_INERROR)
    {
        return "failed";
    }
    return db_in_transaction(*connection) ? "active" : "idle";
}

} // namespace tx_generated
