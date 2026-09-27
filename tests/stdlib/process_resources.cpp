#include "stdlib/process_io.hpp"
#include "stdlib/process_internal.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"

#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>

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

DWORD handle_count()
{
    DWORD result = 0;
    require(GetProcessHandleCount(GetCurrentProcess(), &result), "handle count failed");
    return result;
}

process_options options(const std::string& executable, const std::string& mode)
{
    process_options result;
    result.executable = executable;
    result.args = {mode};
    return result;
}

void resource_cycles(const std::string& helper)
{
    const auto limits = process_limits{make_bytes({}), 5000, 32, "kill"};
    process_run(options(helper, "binary"), limits);
    // 首次失败启动会触发 Windows 的惰性内部初始化，先纳入稳定基线。
    try
    {
        process_spawn(options("missing-child.exe", "exit"));
    }
    catch (const runtime_failure&)
    {
    }
    const auto before = handle_count();
    for (int index = 0; index < 24; ++index)
    {
        auto result = process_run(options(helper, "binary"), limits);
        require(result.reason == "completed" && result.stdout_data->size() == 3,
            "capture cycle failed");
        auto pipe = process_get_pipe(result.child, 1);
        auto alias = result.child;
        process_close(result.child);
        process_close(alias);
        require(!pipe->handle.valid(), "pipe alias outlived close");
        try
        {
            process_spawn(options("missing-child.exe", "exit"));
            throw std::runtime_error("missing executable unexpectedly started");
        }
        catch (const runtime_failure& error)
        {
            require(error.error().code == "spawn_failed", "wrong spawn error");
        }
    }
    const auto after = handle_count();
    if (after != before)
    {
        throw std::runtime_error("process or pipe handle count: " +
            std::to_string(before) + " -> " + std::to_string(after));
    }
    std::cout << "PROCESS_HANDLE_CLEANUP_OK\n";
}

void close_preserves_child(const std::string& helper)
{
    auto child = process_spawn(options(helper, "sleep"));
    process_detail::native_handle observer(OpenProcess(
        SYNCHRONIZE | PROCESS_TERMINATE, FALSE, static_cast<DWORD>(process_id(child))));
    require(observer.valid(), "observer open failed");
    child.reset();
    require(WaitForSingleObject(observer.get(), 0) == WAIT_TIMEOUT,
        "last reference killed child");
    require(TerminateProcess(observer.get(), 0) != 0, "observer terminate failed");
    require(WaitForSingleObject(observer.get(), 5000) == WAIT_OBJECT_0,
        "observer wait failed");
    std::cout << "PROCESS_DETACH_OK\n";
}

void cancellation_wake(const std::string& helper)
{
    auto state = std::make_shared<cancellation_state>();
    std::jthread canceller([state]
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        {
            std::lock_guard lock(state->mutex);
            state->cancelled = true;
        }
        state->changed.notify_all();
    });
    const auto start = std::chrono::steady_clock::now();
    auto result = process_run(options(helper, "sleep"),
        {make_bytes({}), 5000, 32, "kill"}, state);
    require(result.reason == "cancelled" && result.status.state == "terminated",
        "active cancellation failed");
    require(std::chrono::steady_clock::now() - start < std::chrono::seconds(1),
        "cancellation did not wake process capture");
    std::cout << "PROCESS_ACTIVE_CANCEL_OK\n";
}

void handle_inheritance(const std::string& helper)
{
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    process_detail::native_handle private_event(CreateEventW(&security, TRUE, FALSE, nullptr));
    require(private_event.valid(), "private event failed");
    auto config = options(helper, "check_handle");
    config.args.push_back(std::to_string(reinterpret_cast<std::uintptr_t>(private_event.get())));
    auto result = process_run(config, {make_bytes({}), 5000, 32, "kill"});
    require(result.status.exit_code == 0, "unselected handle inherited");
    std::cout << "PROCESS_INHERITANCE_OK\n";
}

void cooperative_termination(const std::string& helper)
{
    require(GetConsoleCP() != 0, "native test requires its own hidden console");
    auto config = options(helper, "control");
    config.new_process_group = true;
    auto child = process_spawn(config);
    struct child_cleanup
    {
        process_child child;
        ~child_cleanup()
        {
            try
            {
                process_kill(child);
                process_wait(child, 5000);
            }
            catch (...)
            {
            }
        }
    } cleanup{child};
    const auto ready = process_read_pipe(process_get_pipe(child, 1), 5, 5000);
    require(ready.data->size() == 5, "console child not ready");
    process_terminate(child);
    const auto status = process_wait(child, 5000);
    if (status.state == "timeout")
    {
        process_kill(child);
        process_wait(child, 5000);
    }
    require(status.state == "exited" && status.exit_code == 23,
        "CTRL_BREAK_EVENT did not reach independent child group");
    std::cout << "PROCESS_COOPERATIVE_TERMINATE_OK\n";
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc == 2, "helper path missing");
        const auto helper = detail::wide_to_utf8(argv[1]);
        resource_cycles(helper);
        close_preserves_child(helper);
        cancellation_wake(helper);
        handle_inheritance(helper);
        cooperative_termination(helper);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
