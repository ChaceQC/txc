#pragma once

#include "stdlib/http2_transport.hpp"
#include "stdlib/httpx.hpp"

#include <nghttp2/nghttp2.h>

#include <atomic>
#include <cstdint>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tx_generated::http2
{

struct request_state
{
    std::int32_t stream_id = 0;
    http_request_data request;
    binary_stream destination;
    std::size_t max_body_bytes = network::max_body_bytes;
    std::size_t header_bytes = 0;
    bool binary = true;
    bool complete = false;
    bool response_done = false;
    std::string scheme;
    std::string authority;
    std::string_view response_body;
    binary_stream response_source;
    std::uint64_t response_remaining = 0;
    std::size_t response_offset = 0;
};

class server_session
{
public:
    server_session(network::socket_handle socket,
                   const std::shared_ptr<tls_config>& tls,
                   std::int64_t timeout_ms);
    ~server_session() noexcept;
    server_session(const server_session&) = delete;
    server_session& operator=(const server_session&) = delete;

    [[nodiscard]] std::shared_ptr<request_state> accept(
        const binary_stream& destination, std::size_t max_body_bytes,
        bool binary, std::int64_t timeout_ms);
    void respond(const std::shared_ptr<request_state>& stream,
                 const http_response_data& value,
                 const binary_stream& source, std::int64_t body_length,
                 bool binary);
    void close_stream(const std::shared_ptr<request_state>& stream) noexcept;
    void close() noexcept;
    [[nodiscard]] bool closed() const noexcept;

private:
    static int on_begin_headers(nghttp2_session*, const nghttp2_frame*, void*);
    static int on_header(nghttp2_session*, const nghttp2_frame*,
                         const std::uint8_t*, std::size_t,
                         const std::uint8_t*, std::size_t, std::uint8_t, void*);
    static int on_data_chunk(nghttp2_session*, std::uint8_t, std::int32_t,
                             const std::uint8_t*, std::size_t, void*);
    static int on_frame_recv(nghttp2_session*, const nghttp2_frame*, void*);
    static int on_invalid_frame(nghttp2_session*, const nghttp2_frame*,
                                int, void*);
    static int on_stream_close(nghttp2_session*, std::int32_t,
                               std::uint32_t, void*);
    static nghttp2_ssize read_response_data(nghttp2_session*, std::int32_t,
        std::uint8_t*, std::size_t, std::uint32_t*, nghttp2_data_source*, void*);

    void begin_headers(const nghttp2_frame* frame);
    void header(const nghttp2_frame* frame, std::string_view name,
                std::string_view value);
    void data_chunk(std::int32_t stream_id, std::string_view bytes);
    void frame_received(const nghttp2_frame* frame);
    void flush_output();
    [[nodiscard]] bool read_input();
    void check_callback_error();
    void record_error() noexcept;
    // 持有 io_mutex_ 时调用；nghttp2 回调失败后不能安全复用当前会话。
    void terminate_locked() noexcept;
    void reject_stream_locked(std::int32_t stream_id) noexcept;
    void validate_request(request_state& stream);
    [[nodiscard]] std::vector<std::pair<std::string, std::string>>
        response_headers(const http_response_data& value,
                         std::int64_t body_length);

    transport io_;
    // 对外可用后，nghttp2 会话操作与 streams_ 访问由同一把锁串行化；
    // nghttp2 同步回调沿用调用方持有的锁，不得再次加锁。
    std::mutex io_mutex_;
    std::mutex accept_mutex_;
    bool secure_ = false;
    nghttp2_session* session_ = nullptr;
    std::unordered_map<std::int32_t, std::shared_ptr<request_state>> streams_;
    std::deque<std::int32_t> completed_;
    binary_stream pending_destination_;
    std::size_t pending_limit_ = network::max_body_bytes;
    bool pending_binary_ = false;
    bool pending_assigned_ = false;
    std::atomic<bool> closed_ = false;
    std::exception_ptr callback_error_;
};

} // namespace tx_generated::http2
