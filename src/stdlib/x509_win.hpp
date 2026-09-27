#pragma once

#include "stdlib/x509.hpp"

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>

#include <memory>
#include <string_view>

namespace tx_generated::x509
{

constexpr std::size_t max_certificate_bytes = 1024 * 1024;
constexpr std::size_t max_input_bytes = 16 * 1024 * 1024;
constexpr std::size_t max_certificate_count = 64;

struct store_closer
{
    void operator()(void* value) const noexcept
    {
        CertCloseStore(value, 0);
    }
};

struct engine_closer
{
    void operator()(void* value) const noexcept
    {
        CertFreeCertificateChainEngine(value);
    }
};

using cert_ptr = std::unique_ptr<const CERT_CONTEXT,
    decltype(&CertFreeCertificateContext)>;
using store_ptr = std::unique_ptr<void, store_closer>;
using engine_ptr = std::unique_ptr<void, engine_closer>;
using chain_ptr = std::unique_ptr<const CERT_CHAIN_CONTEXT,
    decltype(&CertFreeCertificateChain)>;

class certificate_cursor
{
public:
    certificate_cursor() = default;
    certificate_cursor(const certificate_cursor&) = delete;
    certificate_cursor& operator=(const certificate_cursor&) = delete;

    ~certificate_cursor()
    {
        if (current_)
        {
            CertFreeCertificateContext(current_);
        }
    }

    PCCERT_CONTEXT next(HCERTSTORE store)
    {
        current_ = CertEnumCertificatesInStore(store, current_);
        return current_;
    }

private:
    PCCERT_CONTEXT current_ = nullptr;
};

[[noreturn]] void fail(const char* code, const char* message);
void check_input_size(std::size_t size);
cert_ptr certificate(const byte_value& data);
byte_value certificate_bytes(PCCERT_CONTEXT value);
store_ptr memory_store();
std::wstring utf8_wide(std::string_view text);

} // namespace tx_generated::x509
#endif
