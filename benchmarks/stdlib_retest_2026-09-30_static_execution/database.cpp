#include "common.hpp"
#include "stdlib/db.hpp"
#include "stdlib/db_async.hpp"
using namespace tx_generated;

void execute_sql(const db_connection& connection, const std::string& sql)
{
    auto statement = db_prepare(connection, sql);
    db_execute(statement);
    db_close_statement(statement);
}

void workload(const db_connection& connection, const std::string& prefix, const std::string& marker)
{
    execute_sql(connection, "CREATE TEMP TABLE bench_items(id BIGINT, label TEXT)");
    auto mode = prefix == "postgres" ? "read_committed" : "deferred";
    auto transaction = db_begin(connection, mode);
    auto insert = db_prepare(connection, "INSERT INTO bench_items VALUES(" + marker + ", 'payload')");
    auto start = bench_clock::now();
    std::int64_t total = 0;
    for (std::int64_t i = 1; i <= 2000; ++i)
    {
        db_bind(insert, 1, {db_value_kind::integer, i});
        total += db_execute(insert).affected_rows;
    }
    db_commit(transaction);
    report(prefix + "_insert", start, total);
    db_close_statement(insert);
    auto query = db_prepare(connection, "SELECT id, label FROM bench_items ORDER BY id");
    start = bench_clock::now();
    total = 0;
    for (int i = 0; i < 10; ++i)
    {
        auto cursor = db_query(query);
        while (auto row = db_next(cursor))
        {
            total += db_as_int(db_column(row, 0)).value() + db_as_str(db_column(row, 1))->size();
        }
        db_close_cursor(cursor);
    }
    report(prefix + "_read", start, total);
    db_close_statement(query);
    start = bench_clock::now();
    total = 0;
    for (int i = 0; i < 100; ++i)
    {
        auto tx = db_begin(connection, mode);
        db_savepoint(tx, "point");
        execute_sql(connection, "INSERT INTO bench_items VALUES(9999, 'rollback')");
        db_rollback_to(tx, "point");
        db_release_savepoint(tx, "point");
        db_commit(tx);
        ++total;
    }
    report(prefix + "_savepoint", start, total);
    db_close(connection);
}

void bench_database(bool postgres)
{
    if (postgres)
    {
        db_postgres_options config;
        config.host = "localhost";
        config.port = std::stoll(std::getenv("TX_DB_PORT"));
        config.database = "postgres";
        config.user = "tx_test";
        config.ca_file = std::getenv("TX_DB_CA");
        auto password = secret::from_bytes(bytes_from_hex(std::getenv("TX_DB_PASSWORD_HEX")));
        workload(db_open_postgres(config, password), "postgres", "$1");
        secret::close(password);
        return;
    }
    db_options memory;
    memory.path = ":memory:";
    workload(db_open(memory), "sqlite", "?");
    db_options file;
    file.path = "bench.sqlite";
    auto pool = db_make_pool(file, 2, 8);
    auto start = bench_clock::now();
    std::int64_t total = 0;
    for (int i = 0; i < 100; ++i)
    {
        auto connection = db_acquire(pool, 5000);
        auto statement = db_prepare(connection, "SELECT 42");
        auto cursor = db_query(statement);
        total += db_as_int(db_column(db_next(cursor), 0)).value();
        db_close_cursor(cursor);
        db_close_statement(statement);
        db_release(connection);
    }
    report("sqlite_pool", start, total);
    executor worker;
    start = bench_clock::now();
    total = 0;
    for (int i = 0; i < 100; ++i)
    {
        total += worker.submit([pool]
        {
            db_async_request request;
            request.pool = pool;
            request.sql = "SELECT 42";
            request.max_rows = 1;
            request.max_bytes = 4096;
            request.query = true;
            request.operation.deadline = bench_clock::now() + std::chrono::seconds(5);
            auto rows = std::get<std::vector<db_row>>(db_run_async(request));
            return db_as_int(db_column(rows.at(0), 0)).value();
        }).get();
    }
    report("sqlite_async", start, total);
    db_close_pool(pool);
    auto connection = db_open(memory);
    std::vector<std::string> statements{"CREATE TABLE migration_items(id BIGINT)"};
    auto checksum = db_migration_checksum(statements);
    db_migrate(connection, 1, statements, checksum);
    start = bench_clock::now();
    total = 0;
    for (int i = 0; i < 100; ++i)
    {
        db_migrate(connection, 1, statements, checksum);
        total += db_schema_version(connection);
    }
    report("migration_recheck", start, total);
    db_close(connection);
}
