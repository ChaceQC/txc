#include "stdlib/bytes.hpp"
#include "stdlib/error.hpp"
#include "stdlib/filesystem_watch.hpp"
#include "stdlib/ipc.hpp"
#include "stdlib/process_io.hpp"
#include "stdlib/profile.hpp"
#include "stdlib/task.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <unistd.h>

namespace
{

using namespace tx_generated;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

int helper(std::string_view mode)
{
    if (mode == "--sleep")
    {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        return 0;
    }
    std::vector<unsigned char> prefix(128 * 1024, 'P');
    std::fwrite(prefix.data(), 1, prefix.size(), stderr);
    std::fwrite(prefix.data(), 1, prefix.size(), stdout);
    std::array<unsigned char, 4096> buffer;
    while (const auto count = std::fread(buffer.data(), 1, buffer.size(), stdin))
    {
        std::fwrite(buffer.data(), 1, count, stdout);
    }
    return 7;
}

void test_process()
{
    process_options options;
    options.executable = "/proc/self/exe";
    options.args = {"--echo"};
    process_limits limits;
    limits.input = make_bytes(std::vector<std::uint8_t>(256 * 1024, 42));
    limits.timeout_ms = 5000;
    limits.max_output_bytes = 512 * 1024;
    limits.stop_action = "kill";
    const auto result = process_run(options, limits);
    require(result.reason == "completed" && result.status.exit_code == 7, "process capture/exit");
    require(result.input_written == 256 * 1024 && result.stdout_data->size() == 384 * 1024 &&
        result.stderr_data->size() == 128 * 1024, "process concurrent pipe backpressure");
    require(result.stdout_data->back() == 42, "process input bytes");
    process_close(result.child);

    options.args = {"--sleep"};
    auto child = process_spawn(options);
    require(process_wait(child, 0).state == "running", "process poll");
    process_terminate(child);
    require(process_wait(child, 1000).exit_code == 143, "process SIGTERM");
    process_close(child);
    child = process_spawn(options);
    auto cancelled = std::make_shared<cancellation_state>();
    cancelled->cancelled = true;
    try
    {
        process_wait(child, -1, cancelled);
        require(false, "process cancellation missing");
    }
    catch (const runtime_failure& failure)
    {
        require(failure.error().kind == tx::error_kind::cancelled, "process cancellation kind");
    }
    process_kill(child);
    require(process_wait(child, 1000).exit_code == 137, "process SIGKILL");
    process_close(child);
}

void test_ipc()
{
    const auto name = "linux_test_" + std::to_string(getpid());
    auto token = std::make_shared<cancellation_state>();
    auto listener = ipc_listen(name, 1024);
    auto client = ipc_connect(name, 1024, 1000, token);
    auto server = ipc_accept(listener, 1000, token);
    require(ipc_send(client, 1, std::any(std::string("中文 IPC")), 1000, token).state == "sent", "ipc send");
    const auto message = ipc_receive(server, 1000, token);
    require(message.state == "data" && std::any_cast<std::string>(message.value) == "中文 IPC", "ipc frame roundtrip");
    require(ipc_receive(server, 0, token).state == "timeout", "ipc timeout");
    const std::array<std::uint8_t, 22> frame{'T', 'X', 'I', 'P', 1, 0, 0, 0,
        42, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0x61, 'x'};
    require(ipc_write_some(client, frame.data(), 10, ipc_deadline(1000), token).count == 10, "ipc partial header send");
    const auto partial = ipc_receive(server, 0, token);
    require(partial.state == "timeout" && partial.received == 10, "ipc partial header retained");
    require(ipc_write_some(client, frame.data() + 10, 12, ipc_deadline(1000), token).count == 12, "ipc remaining frame send");
    const auto recovered = ipc_receive(server, 1000, token);
    require(recovered.state == "restarted" && std::any_cast<std::string>(recovered.value) == "x", "ipc partial frame recovery");
    token->cancelled = true;
    require(ipc_receive(server, -1, token).state == "cancelled", "ipc cancellation");
    ipc_close(client);
    ipc_close(server);
    ipc_close_listener(listener);
    listener = ipc_listen(name, 1024);
    require(ipc_close_listener(listener), "ipc namespace released");
}

void test_watch(const std::filesystem::path& directory)
{
    auto watcher = fs_watch(directory.string(), true);
    require(fs_watch_next(watcher, 0).kind == "timeout", "watch initial timeout");
    {
        std::ofstream output(directory / "中文.txt");
        output << "content";
    }
    bool created = false;
    for (int index = 0; index < 8 && !created; ++index)
    {
        const auto event = fs_watch_next(watcher, 1000);
        created = event.kind == "created" && event.path == "中文.txt";
    }
    require(created, "watch unicode creation");
    while (fs_watch_next(watcher, 0).kind != "timeout")
    {
    }
    auto waiter = std::async(std::launch::async, [watcher]
    {
        try
        {
            fs_watch_next(watcher, -1);
            return false;
        }
        catch (const runtime_failure& failure)
        {
            return failure.error().code == "closed_handle";
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    fs_close_watch(watcher);
    require(waiter.wait_for(std::chrono::seconds(1)) == std::future_status::ready && waiter.get(), "watch close wakes waiter");
}

void test_timer_profile()
{
    auto scope = std::make_shared<task_scope_state>();
    scope->maximum = 100;
    scope->cancellation = std::make_shared<cancellation_state>();
    auto timer = schedule_task_timer(scope, 5);
    wait_task(timer);
    require(timer->error.kind == tx::error_kind::none, "timer complete");
    timer = schedule_task_timer(scope, 30000);
    cancel_task_scope(scope);
    wait_task(timer);
    require(timer->error.kind == tx::error_kind::cancelled, "timer cancellation");
    profile_start(1, 1000);
    const auto span = profile_begin_span("linux");
    for (int index = 0; index < 10000; ++index)
    {
        auto value = std::make_unique<std::array<char, 128>>();
        (*value)[0] = static_cast<char>(index);
        asm volatile("" : : "g"(value.get()) : "memory");
    }
    profile_end_span(span);
    const auto report = profile_snapshot(true);
    require(report.find("linux-x64") != std::string::npos &&
        report.find("thread_cpu_time_weighted_tx_frame") != std::string::npos, "profile backend metadata");
}

} // namespace

int main(int argc, char** argv)
{
    if (argc > 1)
    {
        return helper(argv[1]);
    }
    char template_path[] = "/tmp/tx-linux-runtime-XXXXXX";
    const char* temporary = mkdtemp(template_path);
    if (!temporary)
    {
        return 1;
    }
    const std::filesystem::path directory(temporary);
    try
    {
        test_process();
        test_ipc();
        test_watch(directory);
        test_timer_profile();
        std::filesystem::remove_all(directory);
        std::cout << "Linux process/IPC/watch/timer/profile passed\n";
        return 0;
    }
    catch (const std::exception& failure)
    {
        std::filesystem::remove_all(directory);
        std::cerr << failure.what() << '\n';
        return 1;
    }
}
