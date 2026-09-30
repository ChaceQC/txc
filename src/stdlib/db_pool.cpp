#include "stdlib/db_pool_internal.hpp"
#include "stdlib/db_operation.hpp"

#include <algorithm>

namespace tx_generated
{
namespace
{

db_pool create_pool(std::int64_t capacity, std::int64_t waiters)
{
    if (capacity < 1 || capacity > 256 || waiters < 0 || waiters > 4096)
    {
        db_fail("invalid_argument", "连接池容量必须为 1～256，等待上限必须为 0～4096");
    }
    auto pool = std::make_shared<db_pool_state>();
    pool->capacity = static_cast<std::size_t>(capacity);
    pool->waiter_limit = static_cast<std::size_t>(waiters);
    pool->idle.reserve(pool->capacity);
    return pool;
}

secret::handle copy_secret(const secret::handle& password)
{
    if (!password)
    {
        db_fail("invalid_argument", "PostgreSQL 密码句柄无效");
    }
    const auto source = password->view();
    auto result = std::make_shared<secret::buffer>(source.size());
    std::copy(source.begin(), source.end(), result->writable().begin());
    return result;
}

void wait_slot(const db_pool& pool, std::unique_lock<std::mutex>& lock,
               std::chrono::steady_clock::time_point deadline, bool immediate)
{
    if (pool->closed)
    {
        db_fail("pool_closed", "数据库连接池已关闭");
    }
    if (!pool->idle.empty() || pool->total < pool->capacity)
    {
        return;
    }
    if (immediate || pool->waiting >= pool->waiter_limit)
    {
        db_fail("pool_exhausted", "连接池已耗尽或等待队列已满");
    }
    ++pool->waiting;
    bool ready = false;
    try
    {
        const auto available = [&]
        {
            return pool->closed || !pool->idle.empty() || pool->total < pool->capacity;
        };
        do
        {
            db_operation_check();
            const auto wake = db_current_operation ? std::min(deadline,
                std::chrono::steady_clock::now() + std::chrono::milliseconds(10)) : deadline;
            ready = pool->changed.wait_until(lock, wake, available);
        }
        while (!ready && std::chrono::steady_clock::now() < deadline);
    }
    catch (...)
    {
        --pool->waiting;
        throw;
    }
    --pool->waiting;
    if (pool->closed)
    {
        db_fail("pool_closed", "等待期间数据库连接池已关闭");
    }
    if (!ready)
    {
        db_fail("timeout", "获取数据库连接超时");
    }
}

db_connection open_slot(const db_pool& pool, const secret::handle& password,
                        std::chrono::steady_clock::time_point deadline, bool immediate)
{
    if (const auto* options = std::get_if<db_options>(&pool->options))
    {
        return db_open(*options);
    }
    auto options = std::get<db_postgres_options>(pool->options);
    if (!immediate)
    {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining < 1)
        {
            db_fail("timeout", "获取数据库连接超时");
        }
        options.connect_timeout_ms = std::min(options.connect_timeout_ms, remaining);
        options.operation_timeout_ms = std::min(options.operation_timeout_ms, remaining);
    }
    auto result = db_open_postgres(options, password);
    result->operation_timeout_ms = std::get<db_postgres_options>(pool->options).operation_timeout_ms;
    return result;
}

} // namespace

db_pool db_make_pool(const db_options& options, std::int64_t capacity, std::int64_t waiters)
{
    if (options.driver != "sqlite" || options.path == ":memory:")
    {
        db_fail("invalid_argument", "SQLite 池需要文件数据库");
    }
    auto pool = create_pool(capacity, waiters);
    pool->options = options;
    return pool;
}

db_pool db_make_postgres_pool(const db_postgres_options& options,
    const secret::handle& password, std::int64_t capacity, std::int64_t waiters)
{
    db_pg_validate_options(options);
    auto pool = create_pool(capacity, waiters);
    pool->options = options;
    pool->password = copy_secret(password);
    return pool;
}

db_connection db_acquire(const db_pool& pool, std::int64_t timeout_ms)
{
    db_operation_check();
    if (!pool || timeout_ms < 0 || timeout_ms > 2147483647)
    {
        db_fail("invalid_argument", "数据库池或获取超时无效");
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    std::unique_lock lock(pool->mutex);
    wait_slot(pool, lock, deadline, timeout_ms == 0);
    if (!pool->idle.empty())
    {
        auto connection = std::move(pool->idle.back());
        pool->idle.pop_back();
        connection->owner = std::this_thread::get_id();
        connection->pool = pool;
        if (connection->native)
        {
            db_install_sqlite_operation(connection, connection->sqlite_busy_timeout_ms);
        }
        return connection;
    }
    const auto password = pool->password ? copy_secret(pool->password) : secret::handle{};
    ++pool->total;
    lock.unlock();
    db_connection connection;
    try
    {
        connection = open_slot(pool, password, deadline, timeout_ms == 0);
        if (timeout_ms && std::chrono::steady_clock::now() >= deadline)
        {
            db_fail("timeout", "创建数据库连接超时");
        }
    }
    catch (...)
    {
        lock.lock();
        --pool->total;
        pool->changed.notify_one();
        throw;
    }
    lock.lock();
    if (pool->closed)
    {
        --pool->total;
        db_fail("pool_closed", "连接创建期间数据库池已关闭");
    }
    connection->pool = pool;
    return connection;
}

bool db_close_pool(const db_pool& pool)
{
    if (!pool)
    {
        return false;
    }
    std::lock_guard lock(pool->mutex);
    if (pool->closed)
    {
        return false;
    }
    pool->closed = true;
    pool->total -= pool->idle.size();
    pool->idle.clear();
    if (pool->password)
    {
        pool->password->close();
        pool->password.reset();
    }
    pool->changed.notify_all();
    return true;
}

} // namespace tx_generated
