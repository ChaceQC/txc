#pragma once

#include "stdlib/bytes.hpp"
#include "stdlib/secret.hpp"

namespace tx_generated::public_key
{

secret::handle ed25519_generate();
secret::handle ed25519_import_seed(const byte_value& seed);
byte_value ed25519_public(const secret::handle& private_key);
byte_value ed25519_sign(const secret::handle& private_key,
                        const byte_value& message);
bool ed25519_verify(const byte_value& public_value, const byte_value& message,
                    const byte_value& signature);
secret::handle x25519_generate();
secret::handle x25519_import_private(const byte_value& raw);
byte_value x25519_public(const secret::handle& private_key);
secret::handle x25519_derive(const secret::handle& private_key,
                            const byte_value& peer_public,
                            const byte_value& salt, const byte_value& info);

} // namespace tx_generated::public_key
