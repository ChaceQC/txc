#include "stdlib/ws.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <limits>
#include <mutex>
#include <thread>

namespace tx_generated
{
namespace
{

struct stream_event
{
    std::string data;
    bool complete = false;
    bool closed = false;
    std::exception_ptr error;
};

struct stream_pipe
{
    std::mutex mutex;
    std::condition_variable ready;
    std::condition_variable space;
    std::deque<stream_event> events;
    bool stopped = false;
};

struct pipe_guard
{
    std::shared_ptr<stream_pipe> pipe;

    ~pipe_guard()
    {
        {
            std::lock_guard lock(pipe->mutex);
            pipe->stopped = true;
        }
        pipe->space.notify_all();
        pipe->ready.notify_all();
    }
};

void publish(const std::shared_ptr<stream_pipe>& pipe, stream_event event)
{
    std::unique_lock lock(pipe->mutex);
    pipe->space.wait(lock, [&]
    {
        return pipe->stopped || pipe->events.size() < 2;
    });
    if (!pipe->stopped)
    {
        pipe->events.push_back(std::move(event));
        pipe->ready.notify_one();
    }
}

void read_client_chunks(std::shared_ptr<ws_connection_state> state,
                        std::shared_ptr<stream_pipe> pipe)
{
    try
    {
        while (true)
        {
            char buffer[16 * 1024];
            DWORD received = 0;
            WINHTTP_WEB_SOCKET_BUFFER_TYPE type{};
            const auto result = WinHttpWebSocketReceive(state->websocket.get(),
                buffer, sizeof(buffer), &received, &type);
            if (result != NO_ERROR)
            {
                network::fail(result == ERROR_WINHTTP_TIMEOUT ? "timeout" :
                              "operation_failed", "读取 WebSocket 消息失败，WinHTTP 错误码 " +
                              std::to_string(result));
            }
            if (type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
            {
                publish(pipe, {{}, false, true, {}});
                return;
            }
            if (type != WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE &&
                type != WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE)
            {
                network::fail("unsupported_frame", "收到非二进制 WebSocket 消息");
            }
            const bool complete =
                type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE;
            publish(pipe, {std::string(buffer, received), complete, false, {}});
            if (complete)
            {
                return;
            }
        }
    }
    catch (...)
    {
        publish(pipe, {{}, false, false, std::current_exception()});
    }
}

} // namespace

void ws_client_send_stream(ws_connection_state& state,
                           const binary_stream& source, std::int64_t length)
{
    if (length == 0)
    {
        ws_client_send(state, {}, true);
        return;
    }
    constexpr std::size_t block_size = 16 * 1024;
    char buffer[block_size];
    auto remaining = static_cast<std::uint64_t>(length);
    while (remaining > 0)
    {
        const auto count = source->file.read_into(buffer,
            static_cast<std::size_t>(std::min<std::uint64_t>(remaining,
                                                            block_size)));
        if (count == 0)
        {
            network::fail("operation_failed", "WebSocket 消息源流提前结束");
        }
        remaining -= count;
        const auto type = remaining == 0
            ? WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE
            : WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;
        const auto result = WinHttpWebSocketSend(state.websocket.get(), type,
            buffer, static_cast<DWORD>(count));
        if (result != NO_ERROR)
        {
            network::fail(result == ERROR_WINHTTP_TIMEOUT ? "timeout" :
                          "operation_failed", "发送 WebSocket 分片失败，WinHTTP 错误码 " +
                          std::to_string(result));
        }
    }
}

ws_stream_message_data ws_client_receive_stream(
    std::shared_ptr<ws_connection_state> state,
    const binary_stream& destination, std::int64_t max_message_bytes,
    std::int64_t timeout_ms)
{
    if (timeout_ms > std::numeric_limits<int>::max())
    {
        network::fail("invalid_argument", "WebSocket 读取超时过大");
    }
    auto pipe = std::make_shared<stream_pipe>();
    pipe_guard guard{pipe};
    std::thread([state, pipe]
    {
        read_client_chunks(state, pipe);
    }).detach();
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    std::int64_t total = 0;
    while (true)
    {
        std::unique_lock lock(pipe->mutex);
        const auto available = [&]
        {
            return !pipe->events.empty();
        };
        const bool received = timeout_ms == 0
            ? (pipe->ready.wait(lock, available), true)
            : pipe->ready.wait_until(lock, deadline, available);
        if (!received)
        {
            network::fail("timeout", "读取 WebSocket 消息超时");
        }
        auto event = std::move(pipe->events.front());
        pipe->events.pop_front();
        lock.unlock();
        pipe->space.notify_one();
        if (event.error)
        {
            std::rethrow_exception(event.error);
        }
        if (event.closed)
        {
            return {false, 0};
        }
        if (event.data.size() >
            static_cast<std::uint64_t>(max_message_bytes - total))
        {
            network::fail("size_limit", "WebSocket 消息超过接收上限");
        }
        destination->file.write(event.data);
        total += static_cast<std::int64_t>(event.data.size());
        if (event.complete)
        {
            return {true, total};
        }
    }
}

} // namespace tx_generated
