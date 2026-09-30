#pragma once

#include "stdlib/x509_win.hpp"

#ifdef _WIN32
#include <mutex>
#include <vector>

namespace tx_generated::x509
{

struct verification_context
{
    byte_value leaf_bytes;
    std::vector<byte_value> intermediate_bytes;
    std::vector<byte_value> anchor_bytes;
    std::mutex mutex;
    cert_ptr leaf{nullptr, &CertFreeCertificateContext};
    store_ptr additional;
    store_ptr roots;
    engine_ptr engine;
};

std::shared_ptr<verification_context> acquire_verification_context(
    const byte_value& leaf, const bytes_vector& intermediates,
    const bytes_vector& anchors, bool system_trust);

} // namespace tx_generated::x509
#endif
