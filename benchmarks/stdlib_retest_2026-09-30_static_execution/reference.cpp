#include "common.hpp"
#include "stdlib/profile.hpp"
#include "stdlib/log.hpp"
#include "backend/cpp/runtime_context.hpp"
#include <atomic>
#include <array>
#include <cstring>

void concurrency()
{
    auto start = bench_clock::now();
    std::int64_t total = 0;
    for (int i = 0; i < 100; ++i)
    {
        std::thread worker([&total]
        {
            ++total;
        });
        worker.join();
    }
    report("thread_spawn_join", start, total);
    std::mutex mutex;
    total = 0;
    start = bench_clock::now();
    for (int i = 0; i < 100000; ++i)
    {
        std::lock_guard lock(mutex);
        ++total;
    }
    report("mutex_uncontended", start, total);
    std::atomic<std::int64_t> value{0};
    start = bench_clock::now();
    for (int i = 0; i < 100000; ++i)
    {
        value.fetch_add(1, std::memory_order_seq_cst);
    }
    report("atomic_add", start, value.load());
    std::queue<std::int64_t> queue;
    std::condition_variable ready;
    total = 0;
    start = bench_clock::now();
    for (int i = 1; i <= 20000; ++i)
    {
        {
            std::lock_guard lock(mutex);
            queue.push(i);
        }
        ready.notify_one();
        {
            std::unique_lock lock(mutex);
            ready.wait_for(lock, std::chrono::seconds(1), [&queue]
            {
                return !queue.empty();
            });
            total += queue.front();
            queue.pop();
        }
        ready.notify_one();
    }
    report("channel_send_recv", start, total);
    start = bench_clock::now();
    total = 0;
    {
        executor pool;
        for (int i = 0; i < 500; ++i)
        {
            total += pool.submit([]
            {
                return 1;
            }).get();
        }
    }
    report("task_spawn_wait", start, total);
}

void async_file()
{
    auto payload = read_file("block.bin");
    executor pool;
    std::int64_t total = 0;
    auto start = bench_clock::now();
    for (int i = 0; i < 32; ++i)
    {
        total += pool.submit([&payload, i]
        {
            std::fstream file("async.bin", std::ios::binary | std::ios::in | std::ios::out);
            if (!file)
            {
                std::ofstream create("async.bin", std::ios::binary);
                create.close();
                file.open("async.bin", std::ios::binary | std::ios::in | std::ios::out);
            }
            file.seekp(i * 65536);
            file.write(reinterpret_cast<const char*>(payload.data()), payload.size());
            file.flush();
            return file ? 65536 : 0;
        }).get();
        total += pool.submit([&payload, i]
        {
            std::ifstream file("async.bin", std::ios::binary);
            file.seekg(i * 65536);
            std::vector<std::uint8_t> data(65536);
            file.read(reinterpret_cast<char*>(data.data()), data.size());
            return data == payload ? 65536 : 0;
        }).get();
    }
    report("async_file_rw", start, total);
}

void diagnostics()
{
    std::int64_t total = 0;
    std::function<void(int)> callback = [&total](int index)
    {
        total += index;
    };
    auto start = bench_clock::now();
    for (int i = 0; i < 20000; ++i)
    {
        callback(i);
    }
    report("test_parameterized", start, total);
    total = 0;
    std::function<bool(int)> predicate = [&total](int value)
    {
        ++total;
        return value >= 1000;
    };
    std::function<int(int, int)> generate = [](int seed, int index)
    {
        return seed + index;
    };
    start = bench_clock::now();
    for (int i = 0; i < 20000; ++i)
    {
        if (!predicate(generate(1000, i)))
        {
            throw std::runtime_error("property failed");
        }
    }
    report("test_property", start, total);
    // 走真实日志后端的级别过滤；回调仅在启用时求值，与其余语言保持惰性语义。
    tx_generated::tx_log_set_level("info");
    start = bench_clock::now();
    for (int i = 0; i < 100000; ++i)
    {
        if (tx_generated::tx_log_enabled("debug"))
        {
            throw std::runtime_error("unexpected log");
        }
    }
    report("log_filtered", start, 100000);
    tx_generated::tx_log_set_file("events.jsonl", 10485760, 2);
    tx_generated::tx_log_set_level("info");
    tx_generated::detail::current_runtime_context().log_request_id = "bench";
    start = bench_clock::now();
    for (int i = 1; i <= 2000; ++i)
    {
        tx_generated::tx_dict fields;
        fields.emplace_back(std::string("index"), std::int64_t{i});
        fields.emplace_back(std::string("password"), std::string("synthetic"));
        tx_generated::tx_log_event("info", "entry", fields, {});
    }
    tx_generated::tx_log_flush();
    report("log_file", start, 2000);
    tx_generated::tx_log_set_stderr();
}

int main(int argc, char** argv)
{
    try
    {
        const std::string group = argc > 1 ? argv[1] : "";
        if (group == "concurrency")
        {
            concurrency();
        }
        else if (group == "sqlite" || group == "postgres")
        {
            bench_database(group == "postgres");
        }
        else if (group == "security")
        {
            bench_security();
        }
        else if (group == "network")
        {
            bench_network();
        }
        else if (group == "async_file")
        {
            async_file();
        }
        else if (group == "diagnostics")
        {
            diagnostics();
        }
        else if (group == "profile")
        {
            tx_generated::profile_start(10, 100000);
            auto start = bench_clock::now();
            std::int64_t total = 0;
            for (int i = 0; i < 1000; ++i)
            {
                auto id = tx_generated::profile_begin_span("bench");
                total += tx_generated::profile_end_span(id) >= 0;
            }
            report("profile_spans", start, total);
            tx_generated::profile_snapshot(true);
        }
        else
        {
            return 2;
        }
    }
    catch (const std::exception&)
    {
        std::cerr << "reference failed; no environment or credentials printed\n";
        return 1;
    }
}
