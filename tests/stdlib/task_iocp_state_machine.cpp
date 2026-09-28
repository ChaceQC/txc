#include "task_iocp_test_support.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

namespace
{

using namespace std::chrono_literals;
using tx_generated::cancellation_state;
using tx_iocp_test::pipe_read_operation;
using tx_iocp_test::submit_mode;

struct pipe_pair
{
    HANDLE server = INVALID_HANDLE_VALUE;
    HANDLE client = INVALID_HANDLE_VALUE;

    ~pipe_pair()
    {
        if (client != INVALID_HANDLE_VALUE)
        {
            CloseHandle(client);
        }
        if (server != INVALID_HANDLE_VALUE)
        {
            CloseHandle(server);
        }
    }
};

bool open_pipe_pair(pipe_pair& pair, unsigned int index)
{
    const auto name = L"\\\\.\\pipe\\tx_iocp_state_" +
        std::to_wstring(GetCurrentProcessId()) + L"_" + std::to_wstring(index);
    pair.server = CreateNamedPipeW(name.c_str(),
        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 4096, 4096,
        0, nullptr);
    if (pair.server == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    OVERLAPPED connection{};
    connection.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!connection.hEvent)
    {
        return false;
    }
    const auto connected = ConnectNamedPipe(pair.server, &connection);
    const auto connect_error = connected ? ERROR_SUCCESS : GetLastError();
    if (!connected && connect_error != ERROR_IO_PENDING &&
        connect_error != ERROR_PIPE_CONNECTED)
    {
        CloseHandle(connection.hEvent);
        return false;
    }

    pair.client = CreateFileW(name.c_str(), GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (pair.client == INVALID_HANDLE_VALUE)
    {
        CloseHandle(connection.hEvent);
        return false;
    }
    if (connect_error == ERROR_IO_PENDING)
    {
        const auto wait_result = WaitForSingleObject(connection.hEvent, 3000);
        DWORD ignored = 0;
        const auto completed = wait_result == WAIT_OBJECT_0 &&
            GetOverlappedResult(pair.server, &connection, &ignored, FALSE);
        CloseHandle(connection.hEvent);
        return completed;
    }

    CloseHandle(connection.hEvent);
    return true;
}

struct operation_context
{
    std::shared_ptr<tx_generated::task_scope_state> scope =
        std::make_shared<tx_generated::task_scope_state>();
    std::shared_ptr<cancellation_state> token =
        std::make_shared<cancellation_state>();

    operation_context()
    {
        scope->cancellation = std::make_shared<cancellation_state>();
    }
};

void attach(pipe_read_operation& operation, pipe_pair& pipe,
            const operation_context& context)
{
    operation.file = pipe.server;
    pipe.server = INVALID_HANDLE_VALUE;
    operation.scope = context.scope;
    operation.token = context.token;
    operation.child = std::make_shared<tx_generated::task_state>();
}

void cancel(const std::shared_ptr<cancellation_state>& state)
{
    {
        std::lock_guard lock(state->mutex);
        state->cancelled = true;
    }
    state->changed.notify_all();
}

bool check_completion(pipe_read_operation& operation, DWORD expected_error,
                      DWORD expected_bytes)
{
    return operation.wait_for_completion(3s) &&
        operation.wait_for_quiet(50ms) && operation.completion_count() == 1 &&
        operation.child_completed() && operation.file_closed() &&
        operation.completion_error() == expected_error &&
        operation.completion_bytes() == expected_bytes;
}

bool test_submit_window_cancellation()
{
    pipe_pair pipe;
    if (!open_pipe_pair(pipe, 1))
    {
        return false;
    }
    operation_context context;
    auto operation = std::make_shared<pipe_read_operation>(
        submit_mode::wait_before_submit, ERROR_SUCCESS);
    attach(*operation, pipe, context);
    std::thread submitter([operation]
    {
        tx_generated::submit_io_operation(operation);
    });
    if (!operation->wait_for_preflight(3s))
    {
        operation->release_submit();
        submitter.join();
        return false;
    }
    const auto preflight_was_clear = !operation->preflight_cancelled();
    cancel(context.token);
    const auto cancellation_seen_while_submitting =
        operation->wait_for_cancel_request(3s);
    operation->release_submit();
    submitter.join();
    return preflight_was_clear && cancellation_seen_while_submitting &&
        operation->submit_result() == ERROR_IO_PENDING &&
        check_completion(*operation, ERROR_OPERATION_ABORTED, 0);
}

bool test_pending_cancellation()
{
    pipe_pair pipe;
    if (!open_pipe_pair(pipe, 2))
    {
        return false;
    }
    operation_context context;
    auto operation = std::make_shared<pipe_read_operation>(
        submit_mode::direct, ERROR_SUCCESS);
    attach(*operation, pipe, context);
    tx_generated::submit_io_operation(operation);
    if (operation->submit_result() != ERROR_IO_PENDING)
    {
        return false;
    }
    cancel(context.scope->cancellation);
    return check_completion(*operation, ERROR_OPERATION_ABORTED, 0);
}

bool test_deadline_cancellation()
{
    pipe_pair pipe;
    if (!open_pipe_pair(pipe, 3))
    {
        return false;
    }
    operation_context context;
    {
        std::lock_guard lock(context.scope->cancellation->mutex);
        context.scope->cancellation->deadline =
            std::chrono::steady_clock::now() + 500ms;
    }
    auto operation = std::make_shared<pipe_read_operation>(
        submit_mode::direct, ERROR_SUCCESS);
    attach(*operation, pipe, context);
    tx_generated::submit_io_operation(operation);
    return operation->submit_result() == ERROR_IO_PENDING &&
        check_completion(*operation, ERROR_OPERATION_ABORTED, 0);
}

bool test_pending_success()
{
    pipe_pair pipe;
    if (!open_pipe_pair(pipe, 4))
    {
        return false;
    }
    operation_context context;
    auto operation = std::make_shared<pipe_read_operation>(
        submit_mode::direct, ERROR_SUCCESS);
    attach(*operation, pipe, context);
    tx_generated::submit_io_operation(operation);
    if (operation->submit_result() != ERROR_IO_PENDING)
    {
        return false;
    }
    const char value = 'Q';
    DWORD written = 0;
    if (!WriteFile(pipe.client, &value, sizeof(value), &written, nullptr) ||
        written != sizeof(value))
    {
        return false;
    }
    return check_completion(*operation, ERROR_SUCCESS, sizeof(value));
}

bool test_synchronous_completion()
{
    pipe_pair pipe;
    if (!open_pipe_pair(pipe, 5))
    {
        return false;
    }
    operation_context context;
    auto operation = std::make_shared<pipe_read_operation>(
        submit_mode::return_success_after_completion, ERROR_SUCCESS);
    attach(*operation, pipe, context);
    std::thread submitter([operation]
    {
        tx_generated::submit_io_operation(operation);
    });
    if (!operation->wait_for_kernel_submit(3s))
    {
        operation->release_success_return();
        submitter.join();
        return false;
    }
    const char value = 'S';
    DWORD written = 0;
    if (!WriteFile(pipe.client, &value, sizeof(value), &written, nullptr) ||
        written != sizeof(value))
    {
        cancel(context.token);
        operation->release_success_return();
        submitter.join();
        return false;
    }
    DWORD read = 0;
    const auto io_completed = GetOverlappedResult(operation->file,
        &operation->overlapped, &read, TRUE);
    const auto packet_received = operation->wait_for_completion_packet(3s);
    operation->release_success_return();
    submitter.join();
    return io_completed && read == sizeof(value) && packet_received &&
        operation->submit_result() == ERROR_SUCCESS &&
        check_completion(*operation, ERROR_SUCCESS, sizeof(value));
}

bool test_immediate_failure()
{
    pipe_pair pipe;
    if (!open_pipe_pair(pipe, 6))
    {
        return false;
    }
    operation_context context;
    auto operation = std::make_shared<pipe_read_operation>(submit_mode::direct,
        ERROR_ACCESS_DENIED);
    attach(*operation, pipe, context);
    tx_generated::submit_io_operation(operation);
    return check_completion(*operation, ERROR_ACCESS_DENIED, 0);
}

bool test_presubmission_cancellation()
{
    pipe_pair pipe;
    if (!open_pipe_pair(pipe, 7))
    {
        return false;
    }
    operation_context context;
    cancel(context.token);
    auto operation = std::make_shared<pipe_read_operation>(
        submit_mode::direct, ERROR_SUCCESS);
    attach(*operation, pipe, context);
    tx_generated::submit_io_operation(operation);
    return operation->submit_result() == ERROR_OPERATION_ABORTED &&
        operation->preflight_cancelled() &&
        check_completion(*operation, ERROR_OPERATION_ABORTED, 0);
}

int test_loop_shutdown()
{
    // 保留客户端句柄到进程退出，让事件循环析构主动取消挂起读取。
    auto* pipe = new pipe_pair();
    if (!open_pipe_pair(*pipe, 8))
    {
        return 1;
    }
    operation_context context;
    auto operation = std::make_shared<pipe_read_operation>(
        submit_mode::direct, ERROR_SUCCESS);
    attach(*operation, *pipe, context);
    tx_generated::submit_io_operation(operation);
    if (operation->submit_result() != ERROR_IO_PENDING)
    {
        return 2;
    }
    std::cout << "IOCP_STOP_PENDING_OK\n";
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc == 2 && std::string_view(argv[1]) == "--shutdown-pending")
    {
        return test_loop_shutdown();
    }
    if (argc != 1)
    {
        return 64;
    }
    if (!test_submit_window_cancellation() || !test_pending_cancellation() ||
        !test_deadline_cancellation() || !test_pending_success() ||
        !test_synchronous_completion() || !test_immediate_failure() ||
        !test_presubmission_cancellation())
    {
        std::cerr << "IOCP state machine scenario failed\n";
        return 1;
    }
    std::cout << "IOCP_STATE_MACHINE_OK\n";
    return 0;
}
