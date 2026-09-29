#include "stdlib/db_internal.hpp"
#include "stdlib/crypto.hpp"
#include "common/utf8.hpp"

namespace tx_generated
{
namespace
{

std::string history_table(const db_connection& connection)
{
    return connection->pg_driver ? "public.tx_schema_migrations" : "main.tx_schema_migrations";
}

db_transaction lock_history(const db_connection& connection)
{
    auto transaction = db_begin(connection, connection->pg_driver ? "read_committed" : "immediate");
    if (connection->pg_driver)
    {
        // 事务级锁连同提交/回滚释放；锁定先于建表，处理首次部署的竞争。
        db_control_execute(connection, "SELECT pg_advisory_xact_lock(1954037095, 1)");
    }
    db_execute(db_prepare(connection, "CREATE TABLE IF NOT EXISTS " + history_table(connection) +
        " (version BIGINT PRIMARY KEY, checksum TEXT NOT NULL)"));
    return transaction;
}

std::vector<std::string> read_history(const db_connection& connection)
{
    auto cursor = db_query(db_prepare(connection, "SELECT version, checksum FROM " +
        history_table(connection) + " ORDER BY version"));
    std::vector<std::string> history;
    while (auto row = db_next(cursor))
    {
        const auto version = db_as_int(db_column(row, 0));
        const auto checksum = db_as_str(db_column(row, 1));
        if (!version || *version != static_cast<std::int64_t>(history.size()) + 1)
        {
            db_fail("migration_order", "迁移历史版本不连续");
        }
        if (!checksum || checksum->size() != 64 ||
            checksum->find_first_not_of("0123456789abcdef") != std::string::npos)
        {
            db_fail("migration_mismatch", "迁移历史校验值无效");
        }
        if (history.size() >= 1000000)
        {
            db_fail("limit_exceeded", "迁移历史超过一百万个版本");
        }
        history.push_back(*checksum);
    }
    return history;
}

} // namespace

std::string db_migration_checksum(const std::vector<std::string>& statements)
{
    if (statements.empty() || statements.size() > 4096)
    {
        db_fail("invalid_argument", "迁移必须包含 1～4096 条 SQL");
    }
    std::string input = "tx-migration-v1\n";
    std::size_t total = 0;
    for (const auto& sql : statements)
    {
        total += sql.size();
        if (sql.empty() || sql.size() > 1048576 || total > 16777216 ||
            sql.find('\0') != std::string::npos || !tx::scan_utf8(sql).valid)
        {
            db_fail("invalid_argument", "迁移 SQL 为空、过大或不是有效 UTF-8");
        }
        input += std::to_string(sql.size()) + ":" + sql;
    }
    return bytes_to_hex(crypto::sha256(make_bytes(
        std::vector<std::uint8_t>(input.begin(), input.end()))));
}

bool db_migrate(const db_connection& connection, std::int64_t version,
    const std::vector<std::string>& statements, std::string_view checksum)
{
    if (version < 1 || checksum != db_migration_checksum(statements))
    {
        db_fail("migration_mismatch", "迁移版本或 SQL 校验值不匹配");
    }
    auto transaction = lock_history(connection);
    const auto history = read_history(connection);
    if (version <= static_cast<std::int64_t>(history.size()))
    {
        if (history[static_cast<std::size_t>(version - 1)] != checksum)
        {
            db_fail("migration_mismatch", "已应用迁移与当前 SQL 的校验值不一致");
        }
        db_commit(transaction);
        return false;
    }
    if (version != static_cast<std::int64_t>(history.size()) + 1)
    {
        db_fail("migration_order", "迁移版本必须连续递增");
    }
    for (const auto& sql : statements)
    {
        db_execute(db_prepare(connection, sql));
    }
    auto record = db_prepare(connection, "INSERT INTO " + history_table(connection) +
        (connection->pg_driver ? " VALUES($1, $2)" : " VALUES(?, ?)"));
    db_bind_all(record, {{db_value_kind::integer, version},
                         {db_value_kind::text, std::string(checksum)}});
    db_execute(record);
    db_commit(transaction);
    return true;
}

std::int64_t db_schema_version(const db_connection& connection)
{
    auto transaction = lock_history(connection);
    const auto version = static_cast<std::int64_t>(read_history(connection).size());
    db_commit(transaction);
    return version;
}

} // namespace tx_generated
