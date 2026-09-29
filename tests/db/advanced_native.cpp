#include "stdlib/db_async.hpp"

#include <atomic>
#include <barrier>
#include <cstdlib>
#include <future>
#include <iostream>
#include <thread>

using namespace tx_generated;
using namespace std::chrono_literals;

namespace
{

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template<class function_type>
void expect(const char* code, function_type function)
{
    try
    {
        function();
    }
    catch (const runtime_failure& failure)
    {
        require(failure.error().code == code,
            (std::string("expected ") + code + ", got " + failure.error().code).c_str());
        return;
    }
    throw std::runtime_error(std::string("missing failure: ") + code);
}

db_pool make_pool(bool postgres, int capacity = 2)
{
    if (!postgres)
    {
        db_options options;
        options.path = "advanced.sqlite";
        return db_make_pool(options, capacity, 8);
    }
    db_postgres_options options;
    options.host = "localhost";
    options.database = "postgres";
    options.user = "tx_test";
    options.port = std::stoll(std::getenv("TX_DB_PORT"));
    options.ca_file = std::getenv("TX_DB_CA");
    const std::string password(std::getenv("TX_DB_PASSWORD"));
    return db_make_postgres_pool(options,
        secret::from_bytes(make_bytes({password.begin(), password.end()})), capacity, 8);
}

db_async_request request(const db_pool& pool, std::string sql, int timeout = 5000)
{
    db_async_request value;
    value.pool = pool;
    value.sql = std::move(sql);
    value.query = true;
    value.max_rows = 10;
    value.max_bytes = 4096;
    value.operation = {std::make_shared<cancellation_state>(),
        std::make_shared<cancellation_state>(), std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout)};
    return value;
}

void migrations(bool postgres)
{
    auto pool = make_pool(postgres);
    {
        auto connection = db_acquire(pool, 1000);
        db_execute(db_prepare(connection, "DROP TABLE IF EXISTS tx_schema_migrations"));
    }
    const std::vector<std::string> sql = {"CREATE TABLE concurrent_migration(id BIGINT PRIMARY KEY)",
        "INSERT INTO concurrent_migration VALUES(1)"};
    const auto checksum = db_migration_checksum(sql);
    std::barrier start(2);
    auto worker = [&]
    {
        auto connection = db_acquire(pool, 5000);
        start.arrive_and_wait();
        return db_migrate(connection, 1, sql, checksum);
    };
    auto first = std::async(std::launch::async, worker);
    auto second = std::async(std::launch::async, worker);
    const auto a = first.get();
    const auto b = second.get();
    require(a != b, "migration executed twice");
    auto connection = db_acquire(pool, 1000);
    const std::vector<std::string> broken = {"CREATE TABLE migration_rollback(id BIGINT)",
        "INSERT INTO concurrent_migration VALUES(1)"};
    expect("constraint_violation", [&]
    {
        db_migrate(connection, 2, broken, db_migration_checksum(broken));
    });
    require(db_schema_version(connection) == 1, "failed migration was recorded");
    db_execute(db_prepare(connection, "CREATE TABLE migration_rollback(id BIGINT)"));
    if (postgres)
    {
        const std::vector<std::string> unsupported = {
            "CREATE INDEX CONCURRENTLY migration_index ON concurrent_migration(id)"};
        expect("query_failed", [&]
        {
            db_migrate(connection, 2, unsupported, db_migration_checksum(unsupported));
        });
        require(db_schema_version(connection) == 1, "nontransactional migration recorded");
    }
    db_close_pool(pool);
}

void cancellation(bool postgres)
{
    auto pool = make_pool(postgres, 1);
    auto slow = request(pool, postgres ? "SELECT 1 FROM pg_sleep(5)" :
        "WITH RECURSIVE n(x) AS (VALUES(1) UNION ALL SELECT x+1 FROM n WHERE x<100000000) SELECT sum(x) FROM n");
    auto pending = std::async(std::launch::async, [&]
    {
        expect("cancelled", [&]
        {
            db_run_async(slow);
        });
    });
    std::this_thread::sleep_for(150ms);
    {
        std::lock_guard lock(slow.operation.token->mutex);
        slow.operation.token->cancelled = true;
    }
    require(pending.wait_for(2s) == std::future_status::ready, "running query cancellation was not prompt");
    pending.get();
    slow.operation.token = std::make_shared<cancellation_state>();
    slow.operation.deadline = std::chrono::steady_clock::now() + 100ms;
    expect("timeout", [&]
    {
        db_run_async(slow);
    });
    auto held = db_acquire(pool, 1000);
    auto waiting = request(pool, "SELECT 1");
    auto waiter = std::async(std::launch::async, [&]
    {
        expect("cancelled", [&]
        {
            db_run_async(waiting);
        });
    });
    std::this_thread::sleep_for(50ms);
    {
        std::lock_guard lock(waiting.operation.scope->mutex);
        waiting.operation.scope->cancelled = true;
    }
    require(waiter.wait_for(1s) == std::future_status::ready, "pool cancellation was not prompt");
    waiter.get();
    db_release(held);
    auto normal = request(pool, "SELECT 1");
    const auto rows = std::get<std::vector<db_row>>(db_run_async(normal));
    require(rows.size() == 1 && *db_as_int(db_column(rows[0], 0)) == 1, "pool did not recover");
    normal.max_bytes = 1;
    expect("limit_exceeded", [&]
    {
        db_run_async(normal);
    });
    auto create = request(pool, "CREATE TABLE async_rollback(id BIGINT PRIMARY KEY)");
    create.query = false;
    db_run_async(create);
    auto overflowing = request(pool, "INSERT INTO async_rollback VALUES(1), (2) RETURNING id");
    overflowing.max_rows = 1;
    expect("limit_exceeded", [&]
    {
        db_run_async(overflowing);
    });
    const auto empty = std::get<std::vector<db_row>>(db_run_async(
        request(pool, "SELECT id FROM async_rollback")));
    require(empty.empty(), "bounded query failure committed writes");
    db_close_pool(pool);
}

} // namespace

int main()
{
    try
    {
        migrations(false);
        cancellation(false);
        if (std::getenv("TX_DB_PORT"))
        {
            migrations(true);
            cancellation(true);
        }
        std::cout << "advanced native passed: migration race, DDL rollback, cancel, timeout, pool recovery\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
