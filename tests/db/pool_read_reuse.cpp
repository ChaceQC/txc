#include "stdlib/db_internal.hpp"
#include "stdlib/db_pool_internal.hpp"
#include <cassert>
#include <iostream>

using namespace tx_generated;

int main()
{
    db_options options;
    options.path = "pool_read_reuse.sqlite";
    auto pool = db_make_pool(options, 1, 1);
    auto first = db_acquire(pool, 0);
    auto* physical = first->native.get();
    assert(db_release(first));
    auto second = db_acquire(pool, 0);
    assert(second.get() != first.get());
    assert(second->native.get() == physical);
    assert(!first->native && first->cleanup_failed);
    auto temporary = db_prepare(second, "CREATE TEMP TABLE private_state(value INTEGER)");
    db_execute(temporary);
    assert(db_release(second));
    assert(pool->idle.empty());
    auto third = db_acquire(pool, 0);
    bool rejected = false;
    try
    {
        db_prepare(third, "SELECT * FROM private_state");
    }
    catch (const runtime_failure&)
    {
        rejected = true;
    }
    assert(rejected);
    db_release(third);
    db_close_pool(pool);
    std::cout << "POOL_READ_REUSE_OK\n";
}
