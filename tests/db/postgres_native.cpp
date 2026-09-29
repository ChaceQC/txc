#include "stdlib/db.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <thread>

using namespace tx_generated;
using namespace std::chrono_literals;

namespace
{

void require(bool value, const char* message)
{
    if (!value)
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
    throw std::runtime_error(std::string("expected ") + code);
}

db_postgres_options options()
{
    db_postgres_options value;
    value.host = "localhost";
    value.database = "postgres";
    value.user = "tx_test";
    value.port = std::stoll(std::getenv("TX_DB_PORT"));
    value.ca_file = std::getenv("TX_DB_CA");
    return value;
}

secret::handle password()
{
    const std::string text(std::getenv("TX_DB_PASSWORD"));
    return secret::from_bytes(make_bytes({text.begin(), text.end()}));
}

db_row row_of(const db_connection& connection, const std::string& sql)
{
    auto statement = db_prepare(connection, sql);
    auto cursor = db_query(statement);
    auto row = db_next(cursor);
    require(!db_next(cursor), "unexpected extra row");
    return row;
}

std::int64_t scalar(const db_connection& connection, const std::string& sql)
{
    return *db_as_int(db_column(row_of(connection, sql), 0));
}

void tls_and_failures()
{
    auto config = options();
    auto secret = password();
    auto bad = config;
    bad.host = "127.0.0.1";
    expect("tls_failed", [&]
    {
        db_open_postgres(bad, secret);
    });
    bad = config;
    bad.ca_file = std::getenv("TX_DB_WRONG_CA");
    expect("tls_failed", [&]
    {
        db_open_postgres(bad, secret);
    });
    expect("authentication_failed", [&]
    {
        db_open_postgres(config, secret::from_bytes(make_bytes({1, 2, 3})));
    });
    auto connection = db_open_postgres(config, secret);
    require(*db_as_bool(db_column(row_of(connection,
        "SELECT ssl FROM pg_stat_ssl WHERE pid=pg_backend_pid()"), 0)), "TLS not enabled");
    expect("query_failed", [&]
    {
        db_prepare(connection, "SELECT 1; SELECT 2");
    });
    expect("invalid_state", [&]
    {
        db_prepare(connection, "/* nested /* inner */ end */ ; COMMIT");
    });
    expect("parameter_missing", [&]
    {
        db_query(db_prepare(connection, "SELECT $1::text"));
    });
    auto empty = db_prepare(connection, "SELECT $1::text, $2::bytea");
    db_bind_all(empty, {{db_value_kind::text, std::string{}},
        {db_value_kind::binary, make_bytes({})}});
    auto cursor = db_query(empty);
    auto row = db_next(cursor);
    require(db_as_str(db_column(row, 0))->empty(), "empty text");
    require((*db_as_bytes(db_column(row, 1)))->empty(), "empty bytea");
    require(!db_next(cursor), "empty result end");
    expect("type_mismatch", [&]
    {
        row_of(connection, "SELECT 'NaN'::float8");
    });
    require(db_transaction_status(connection) == "closed", "failed row must discard unread results");
    config.max_value_bytes = 64;
    connection = db_open_postgres(config, secret);
    expect("limit_exceeded", [&]
    {
        row_of(connection, "SELECT repeat('x', 512)");
    });
    config.max_value_bytes = 1048576;
    config.operation_timeout_ms = 70;
    connection = db_open_postgres(config, secret);
    expect("timeout", [&]
    {
        row_of(connection, "SELECT 1::bigint FROM pg_sleep(2)");
    });
    require(db_transaction_status(connection) == "closed", "timeout connection reused");
    config = options();
    config.read_only = true;
    connection = db_open_postgres(config, secret);
    expect("read_only", [&]
    {
        db_execute(db_prepare(connection, "CREATE TABLE rejected(id int)"));
    });
    db_close(connection);
    std::cout << "PASS PostgreSQL TLS, authentication, binding, limits and timeout\n";
}

void postgres_cleanup()
{
    auto secret = password();
    auto pool = db_make_postgres_pool(options(), secret, 1, 2);
    secret->close();
    auto connection = db_acquire(pool, 5000);
    const auto pid = scalar(connection, "SELECT pg_backend_pid()::bigint");
    db_execute(db_prepare(connection, "CREATE TEMP TABLE dirty(value int)"));
    row_of(connection, "SELECT set_config('application_name','dirty',false)");
    auto transaction = db_begin(connection, "read_committed");
    auto statement = db_prepare(connection, "INSERT INTO dirty VALUES(1)");
    db_execute(statement);
    auto alias = connection;
    require(db_release(connection), "release");
    require(!db_release(alias), "duplicate release");
    expect("invalid_state", [&]
    {
        db_execute(statement);
    });
    connection = db_acquire(pool, 1000);
    require(scalar(connection, "SELECT pg_backend_pid()::bigint") == pid, "healthy session not reused");
    require(scalar(connection, "SELECT count(*) FROM pg_tables WHERE tablename='dirty'") == 0, "temp table leaked");
    require(*db_as_str(db_column(row_of(connection, "SELECT current_setting('application_name')"), 0)) == "tx",
        "session setting leaked");
    require(db_transaction_status(connection) == "idle", "transaction leaked");
    auto select = db_prepare(connection, "SELECT generate_series(1,100)::bigint");
    auto cursor = db_query(select);
    auto retained = db_next(cursor);
    db_release(connection);
    require(*db_as_int(db_column(retained, 0)) == 1, "row lifetime");
    connection = db_acquire(pool, 5000);
    require(scalar(connection, "SELECT pg_backend_pid()::bigint") != pid, "unread result reused");
    const auto killed = scalar(connection, "SELECT pg_backend_pid()::bigint");
    auto admin = db_open_postgres(options(), password());
    row_of(admin, "SELECT pg_terminate_backend(" + std::to_string(killed) + ")");
    expect("connection_lost", [&]
    {
        scalar(connection, "SELECT 1::bigint");
    });
    db_release(connection);
    connection = db_acquire(pool, 5000);
    require(scalar(connection, "SELECT 1::bigint") == 1, "dead slot not replaced");
    db_release(connection);
    require(db_close_pool(pool), "close pool");
    std::cout << "PASS PostgreSQL session cleanup, cursor discard and server disconnect\n";
}

void pool_waiting()
{
    db_options config;
    config.path = "native_pool.sqlite";
    auto pool = db_make_pool(config, 1, 1);
    auto first = db_acquire(pool, 0);
    expect("pool_exhausted", [&]
    {
        db_acquire(pool, 0);
    });
    expect("timeout", [&]
    {
        db_acquire(pool, 30);
    });
    std::promise<void> started;
    auto future = std::async(std::launch::async, [&]
    {
        started.set_value();
        auto second = db_acquire(pool, 1500);
        const auto value = scalar(second, "SELECT 42");
        db_release(second);
        return value;
    });
    started.get_future().wait();
    std::this_thread::sleep_for(50ms);
    expect("pool_exhausted", [&]
    {
        db_acquire(pool, 10);
    });
    db_release(first);
    require(future.get() == 42, "waiter not woken on release");
    first = db_acquire(pool, 0);
    std::promise<void> closing;
    auto waiter = std::async(std::launch::async, [&]
    {
        closing.set_value();
        expect("pool_closed", [&]
        {
            db_acquire(pool, 1500);
        });
    });
    closing.get_future().wait();
    std::this_thread::sleep_for(50ms);
    db_close_pool(pool);
    waiter.get();
    require(scalar(first, "SELECT 9") == 9, "close revoked active lease");
    db_release(first);
    require(!db_close_pool(pool), "close idempotence");
    auto automatic = db_make_pool(config, 1, 0);
    {
        auto lease = db_acquire(automatic, 0);
        db_execute(db_prepare(lease, "CREATE TEMP TABLE temporary_state(id int)"));
        auto transaction = db_begin(lease, "deferred");
        db_execute(db_prepare(lease, "INSERT INTO temporary_state VALUES(1)"));
    }
    auto clean = db_acquire(automatic, 0);
    require(scalar(clean, "SELECT count(*) FROM sqlite_temp_master") == 0, "automatic return leaked temp state");
    db_release(clean);
    db_close_pool(automatic);
    std::cout << "PASS pool capacity, wait limits, timeout and close wakeup\n";
}

} // namespace

int main()
{
    try
    {
        tls_and_failures();
        postgres_cleanup();
        pool_waiting();
    }
    catch (const runtime_failure& failure)
    {
        std::cerr << failure.error().code << '\n';
        return 1;
    }
    catch (const std::exception& failure)
    {
        std::cerr << failure.what() << '\n';
        return 1;
    }
}
