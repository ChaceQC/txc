#include "stdlib/ipc.hpp"

#include "stdlib/bytes.hpp"
#include "stdlib/process_io.hpp"
#include "stdlib/process_io_internal.hpp"
#include "stdlib/error.hpp"

#include <algorithm>

namespace tx_generated
{
namespace
{

ipc_transfer stopped(ipc_clock::time_point deadline,
                     const std::shared_ptr<cancellation_state>& token)
{
    if (ipc_cancelled(token))
    {
        return {ipc_transfer_state::cancelled, 0, {}};
    }
    if (ipc_clock::now() >= deadline)
    {
        return {ipc_transfer_state::timeout, 0, {}};
    }
    return {ipc_transfer_state::data, 0, {}};
}

ipc_transfer overlapped_transfer(HANDLE handle, void* data, DWORD size,
    bool writing, ipc_clock::time_point deadline,
    const std::shared_ptr<cancellation_state>& token)
{
    if (ipc_cancelled(token))
    {
        return {ipc_transfer_state::cancelled, 0, {}};
    }
    process_detail::native_handle event(CreateEventW(
        nullptr, TRUE, FALSE, nullptr));
    if (!event.valid())
    {
        return {ipc_transfer_state::error, 0, "ipc_event_failed"};
    }
    OVERLAPPED operation{};
    operation.hEvent = event.get();
    const auto started = writing
        ? WriteFile(handle, data, size, nullptr, &operation)
        : ReadFile(handle, data, size, nullptr, &operation);
    DWORD transferred = 0;
    if (started)
    {
        if (GetOverlappedResult(handle, &operation, &transferred, FALSE))
        {
            return {transferred == 0 && !writing
                ? ipc_transfer_state::eof : ipc_transfer_state::data,
                transferred, {}};
        }
        const auto error = GetLastError();
        if (error != ERROR_IO_INCOMPLETE)
        {
            return {process_detail::broken_pipe(error)
                ? ipc_transfer_state::eof : ipc_transfer_state::error,
                transferred, writing ? "ipc_write_failed" : "ipc_read_failed"};
        }
    }
    else
    {
        const auto error = GetLastError();
        if (error != ERROR_IO_PENDING)
        {
            if (process_detail::broken_pipe(error))
            {
                return {ipc_transfer_state::eof, 0, {}};
            }
            return {ipc_transfer_state::error, 0,
                writing ? "ipc_write_failed" : "ipc_read_failed"};
        }
    }
    for (;;)
    {
        const auto stop = stopped(deadline, token);
        if (stop.state != ipc_transfer_state::data)
        {
            CancelIoEx(handle, &operation);
            const auto completed = GetOverlappedResult(
                handle, &operation, &transferred, TRUE);
            if (completed)
            {
                return {ipc_transfer_state::data, transferred, {}};
            }
            return {stop.state, transferred, {}};
        }
        const auto signal = WaitForSingleObject(event.get(), 10);
        if (signal == WAIT_OBJECT_0)
        {
            if (GetOverlappedResult(handle, &operation, &transferred, FALSE))
            {
                return {transferred == 0 && !writing
                    ? ipc_transfer_state::eof : ipc_transfer_state::data,
                    transferred, {}};
            }
            const auto error = GetLastError();
            if (process_detail::broken_pipe(error))
            {
                return {ipc_transfer_state::eof, transferred, {}};
            }
            return {ipc_transfer_state::error, transferred,
                writing ? "ipc_write_failed" : "ipc_read_failed"};
        }
        if (signal == WAIT_FAILED)
        {
            CancelIoEx(handle, &operation);
            GetOverlappedResult(handle, &operation, &transferred, TRUE);
            return {ipc_transfer_state::error, transferred,
                "ipc_wait_failed"};
        }
    }
}

ipc_transfer process_write_some(const process_pipe& pipe,
    const std::uint8_t* data, std::size_t size,
    ipc_clock::time_point deadline,
    const std::shared_ptr<cancellation_state>& token)
{
    if (ipc_cancelled(token))
    {
        return {ipc_transfer_state::cancelled, 0, {}};
    }
    const auto chunk = std::min<std::size_t>(size, 16 * 1024);
    const auto remaining = deadline == ipc_clock::time_point::max()
        ? 10 : std::max<std::int64_t>(0,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - ipc_clock::now()).count());
    try
    {
        const auto result = process_write_pipe(pipe,
            make_bytes(std::vector<std::uint8_t>(data, data + chunk)),
            std::min<std::int64_t>(remaining, 10));
        if (result.state == "written")
        {
            return {ipc_transfer_state::data,
                static_cast<std::size_t>(result.written), {}};
        }
        if (result.state == "closed")
        {
            return {ipc_transfer_state::eof,
                static_cast<std::size_t>(result.written), {}};
        }
        return {ipc_transfer_state::timeout, 0, {}};
    }
    catch (const runtime_failure&)
    {
        return {ipc_transfer_state::error, 0, "ipc_write_failed"};
    }
}

} // namespace

ipc_transfer ipc_read_some(const ipc_stream& stream, std::uint8_t* data,
    std::size_t size, ipc_clock::time_point deadline,
    const std::shared_ptr<cancellation_state>& token)
{
    const auto handle = stream->native.valid()
        ? stream->native.get() : stream->reader->handle.get();
    return overlapped_transfer(handle, data,
        static_cast<DWORD>(std::min<std::size_t>(size, 64 * 1024)),
        false, deadline, token);
}

ipc_transfer ipc_write_some(const ipc_stream& stream,
    const std::uint8_t* data, std::size_t size,
    ipc_clock::time_point deadline,
    const std::shared_ptr<cancellation_state>& token)
{
    if (stream->writer)
    {
        return process_write_some(stream->writer, data, size,
            deadline, token);
    }
    return overlapped_transfer(stream->native.get(),
        const_cast<std::uint8_t*>(data),
        static_cast<DWORD>(std::min<std::size_t>(size, 64 * 1024)),
        true, deadline, token);
}

} // namespace tx_generated
