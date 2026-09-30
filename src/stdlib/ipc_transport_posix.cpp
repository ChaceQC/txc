#include "stdlib/ipc.hpp"

#include <algorithm>
#include <cerrno>
#include <poll.h>
#include <sys/socket.h>

namespace tx_generated
{
namespace
{

ipc_transfer transfer(int descriptor, void* data, std::size_t size, bool writing,
    bool socket, ipc_clock::time_point deadline,
    const std::shared_ptr<cancellation_state>& token)
{
    for (;;)
    {
        if (ipc_cancelled(token))
        {
            return {ipc_transfer_state::cancelled, 0, {}};
        }
        const auto count = writing
            ? socket ? ::send(descriptor, data, size, MSG_NOSIGNAL)
                     : process_detail::write_no_signal(descriptor, data, size)
            : ::read(descriptor, data, size);
        if (count >= 0)
        {
            return {count == 0 && !writing ? ipc_transfer_state::eof : ipc_transfer_state::data,
                static_cast<std::size_t>(count), {}};
        }
        if (errno == EPIPE || errno == ECONNRESET)
        {
            return {ipc_transfer_state::eof, 0, {}};
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
        {
            return {ipc_transfer_state::error, 0, writing ? "ipc_write_failed" : "ipc_read_failed"};
        }
        if (ipc_clock::now() >= deadline)
        {
            return {ipc_transfer_state::timeout, 0, {}};
        }
        pollfd item{descriptor, static_cast<short>(writing ? POLLOUT : POLLIN), 0};
        if (::poll(&item, 1, 1) < 0 && errno != EINTR)
        {
            return {ipc_transfer_state::error, 0, "ipc_wait_failed"};
        }
    }
}

} // namespace

ipc_transfer ipc_read_some(const ipc_stream& stream, std::uint8_t* data,
    std::size_t size, ipc_clock::time_point deadline,
    const std::shared_ptr<cancellation_state>& token)
{
    return transfer(stream->native.valid() ? stream->native.get() : stream->reader->handle.get(),
        data, std::min<std::size_t>(size, 65536), false, stream->native.valid(), deadline, token);
}

ipc_transfer ipc_write_some(const ipc_stream& stream, const std::uint8_t* data,
    std::size_t size, ipc_clock::time_point deadline,
    const std::shared_ptr<cancellation_state>& token)
{
    return transfer(stream->native.valid() ? stream->native.get() : stream->writer->handle.get(),
        const_cast<std::uint8_t*>(data), std::min<std::size_t>(size, 65536), true,
        stream->native.valid(), deadline, token);
}

} // namespace tx_generated
