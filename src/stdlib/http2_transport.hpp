#pragma once

#include "stdlib/network_common.hpp"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace tx_generated::http2
{

class tls_config
{
public:
    tls_config(std::string_view cert_path, std::string_view key_path);
    ~tls_config() noexcept;
    tls_config(const tls_config&) = delete;
    tls_config& operator=(const tls_config&) = delete;

    [[nodiscard]] mbedtls_ssl_config* get() noexcept
    {
        return &config_;
    }

private:
    void release() noexcept;
    mbedtls_entropy_context entropy_{};
    mbedtls_ctr_drbg_context rng_{};
    mbedtls_x509_crt cert_{};
    mbedtls_pk_context key_{};
    mbedtls_ssl_config config_{};
};

class transport
{
public:
    transport(network::socket_handle socket,
              const std::shared_ptr<tls_config>& tls,
              std::int64_t timeout_ms);
    transport(std::shared_ptr<tls::secure_connection> secure,
              std::int64_t timeout_ms);
    ~transport() noexcept;
    transport(const transport&) = delete;
    transport& operator=(const transport&) = delete;

    [[nodiscard]] std::string read_some(std::size_t max_bytes);
    void write_all(std::string_view bytes);
    void set_receive_timeout(std::int64_t timeout_ms);

private:
    static int tls_send(void* context, const unsigned char* data, std::size_t size);
    static int tls_recv(void* context, unsigned char* data, std::size_t size);
    network::socket_handle socket_;
    std::shared_ptr<tls_config> tls_;
    std::shared_ptr<tls::secure_connection> secure_;
    std::int64_t timeout_ms_ = 0;
    mbedtls_ssl_context ssl_{};
};

} // namespace tx_generated::http2
