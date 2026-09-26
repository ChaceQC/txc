#pragma once

#include "stdlib/http2_transport.hpp"
#include "stdlib/httpx.hpp"

#include <nghttp2/nghttp2.h>

#include <cstdint>
#include <exception>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated::http2
{

http_response_data client_send(std::string_view method,
                               std::string_view url,
                               const network::header_map& headers,
                               std::string_view body,
                               const binary_stream& source,
                               std::int64_t body_length,
                               const binary_stream& destination,
                               std::int64_t max_response_bytes,
                               std::int64_t timeout_ms,
                               bool binary);

class client_session
{
public:
    client_session(network::socket_handle socket, std::int64_t timeout_ms);
    ~client_session() noexcept;
    client_session(const client_session&) = delete;
    client_session& operator=(const client_session&) = delete;

    http_response_data run(std::string_view method,
                           std::string_view target,
                           std::string_view authority,
                           const network::header_map& headers,
                           std::string_view body,
                           const binary_stream& source,
                           std::int64_t body_length,
                           const binary_stream& destination,
                           std::int64_t max_response_bytes,
                           bool binary);

private:
    static int on_header(nghttp2_session*, const nghttp2_frame*,
                         const std::uint8_t*, std::size_t,
                         const std::uint8_t*, std::size_t, std::uint8_t, void*);
    static int on_data_chunk(nghttp2_session*, std::uint8_t,
                             std::int32_t, const std::uint8_t*,
                             std::size_t, void*);
    static int on_frame_recv(nghttp2_session*, const nghttp2_frame*, void*);
    static nghttp2_ssize read_request_data(nghttp2_session*, std::int32_t,
        std::uint8_t*, std::size_t, std::uint32_t*, nghttp2_data_source*, void*);

    void header(std::string_view name, std::string_view value);
    void data_chunk(std::string_view bytes);
    void frame_received(const nghttp2_frame* frame);
    void flush_output();
    void read_input();
    void check_error();
    void record_error() noexcept;
    [[nodiscard]] std::vector<std::pair<std::string, std::string>>
        request_headers(std::string_view method, std::string_view target,
                        std::string_view authority,
                        const network::header_map& headers,
                        std::int64_t body_length);

    transport io_;
    nghttp2_session* session_ = nullptr;
    http_response_data response_;
    binary_stream destination_;
    binary_stream source_;
    std::string_view body_;
    std::uint64_t request_remaining_ = 0;
    std::size_t request_offset_ = 0;
    std::int64_t max_response_bytes_ = 0;
    std::size_t header_bytes_ = 0;
    std::int32_t stream_id_ = 0;
    bool complete_ = false;
    bool binary_ = false;
    std::exception_ptr error_;
};

} // namespace tx_generated::http2
