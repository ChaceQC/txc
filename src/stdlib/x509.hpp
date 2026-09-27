#pragma once

#include "stdlib/secret.hpp"
#include "stdlib/vector.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace tx_generated::x509
{

struct verification
{
    std::string status;
    std::string revocation = "not_checked";
    std::vector<byte_value> chain;
};

std::vector<byte_value> parse_pem(std::string_view text);
byte_value parse_der(const byte_value& data);
std::vector<byte_value> parse_pkcs12(const byte_value& data,
                                    const secret::handle& password);
secret::handle pkcs12_private_key(const byte_value& data,
                                  const secret::handle& password);
verification verify(const byte_value& leaf, const bytes_vector& intermediates,
                    const bytes_vector& trust_anchors, std::string_view hostname,
                    std::string_view purpose, bool system_trust);

} // namespace tx_generated::x509
