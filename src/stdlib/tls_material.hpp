#pragma once

#include "stdlib/tls.hpp"

#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>

namespace tx_generated::tls
{

struct identity_material
{
    identity_material();
    ~identity_material();
    identity_material(const identity_material&) = delete;
    identity_material& operator=(const identity_material&) = delete;
    mbedtls_x509_crt certificates;
    mbedtls_pk_context key;
};

int tls_random(void*, unsigned char* output, std::size_t size) noexcept;
std::shared_ptr<identity_material> acquire_identity_material(const std::shared_ptr<const identity_state>& identity);
void release_identity_material(const std::shared_ptr<const identity_state>& identity,
    std::shared_ptr<identity_material> material) noexcept;

} // namespace tx_generated::tls
