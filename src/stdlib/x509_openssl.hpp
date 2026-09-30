#pragma once

#include "stdlib/x509.hpp"

#include <openssl/x509.h>
#include <memory>

namespace tx_generated::x509
{

constexpr std::size_t max_certificate_bytes = 1024 * 1024;
constexpr std::size_t max_input_bytes = 16 * 1024 * 1024;
constexpr std::size_t max_certificate_count = 64;

using cert_ptr = std::unique_ptr<X509, decltype(&X509_free)>;

struct certificates_closer
{
    void operator()(STACK_OF(X509)* value) const noexcept
    {
        sk_X509_pop_free(value, X509_free);
    }
};

using certificates_ptr = std::unique_ptr<STACK_OF(X509), certificates_closer>;

[[noreturn]] void fail(const char* code, const char* message);
void check_input_size(std::size_t size);
void validate_text(std::string_view text);
cert_ptr certificate(const byte_value& data);
byte_value certificate_bytes(X509* value);

} // namespace tx_generated::x509
