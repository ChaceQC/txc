#include "stdlib/db.hpp"

#include <atomic>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

using namespace tx_generated;

namespace
{

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

std::int64_t count(const db_connection& connection)
{
    const auto statement = db_prepare(connection, "SELECT count(*) FROM items");
    const auto cursor = db_query(statement);
    return *db_as_int(db_column(db_next(cursor), 0));
}

void expect_nonfinite(double number)
{
    try
    {
        db_validate_value({db_value_kind::floating, number}, 1024);
        throw std::runtime_error("nonfinite accepted");
    }
    catch (const runtime_failure& failure)
    {
        require(failure.error().kind == tx::error_kind::database &&
            failure.error().code == "invalid_argument", "nonfinite identity");
    }
}

} // namespace

int main()
{
    try
    {
        db_options options;
        options.path = ":memory:";
        const auto connection = db_open(options);
        std::atomic<bool> rejected = false;
        std::thread worker([&]
        {
            try
            {
                db_prepare(connection, "SELECT 1");
            }
            catch (const runtime_failure& failure)
            {
                rejected = failure.error().kind == tx::error_kind::database &&
                           failure.error().code == "thread_violation";
            }
        });
        worker.join();
        require(rejected, "cross-thread operation accepted");
        db_execute(db_prepare(connection, "CREATE TABLE items(id INTEGER PRIMARY KEY)"));
        {
            const auto statement = db_prepare(connection, "PRAGMA TABLE_INFO(items)");
            const auto cursor = db_query(statement);
            require(db_next(cursor) != nullptr, "uppercase read PRAGMA rejected");
        }
        {
            auto transaction = db_begin(connection, "deferred");
            auto alias = transaction;
            db_execute(db_prepare(connection, "INSERT INTO items VALUES(1)"));
            transaction.reset();
            require(count(connection) == 1, "first alias release rolled back");
            alias.reset();
        }
        require(count(connection) == 0, "last alias did not roll back");
        {
            const auto transaction = db_begin(connection, "deferred");
            db_execute(db_prepare(connection, "INSERT INTO items VALUES(2)"));
            const auto statement = db_prepare(connection, "SELECT * FROM items");
            const auto cursor = db_query(statement);
            require(db_next(cursor) != nullptr, "missing active row");
        }
        require(count(connection) == 0, "resource scope did not roll back");
        const auto statement = db_prepare(connection, "SELECT * FROM items");
        {
            const auto cursor = db_query(statement);
        }
        require(count(connection) == 0, "cursor destructor retained active slot");
        expect_nonfinite(std::numeric_limits<double>::quiet_NaN());
        expect_nonfinite(std::numeric_limits<double>::infinity());
        expect_nonfinite(-std::numeric_limits<double>::infinity());
        require(db_close(connection), "close failed");
        require(!db_close_statement(statement), "closed statement remained open");
        std::cout << "SQLite native thread, alias cleanup, nonfinite: PASS\n";
        return 0;
    }
    catch (const std::exception& failure)
    {
        std::cerr << failure.what() << '\n';
        return 1;
    }
}
