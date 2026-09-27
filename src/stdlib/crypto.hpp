#pragma once

#include "stdlib/bytes.hpp"
#include "stdlib/file_stream.hpp"
#include "stdlib/secret.hpp"

#include <cstdint>

namespace tx_generated::crypto
{

byte_value random_bytes(std::int64_t count);
byte_value generate_key();
byte_value sha256(const byte_value& data);
byte_value sha512(const byte_value& data);
byte_value hmac_sha256(const byte_value& key, const byte_value& data);
bool secure_equal(const byte_value& left, const byte_value& right);
byte_value hkdf_sha256(const byte_value& ikm, const byte_value& salt,
                       const byte_value& info, std::int64_t length);
byte_value pbkdf2_sha256(const byte_value& password, const byte_value& salt,
                         std::int64_t iterations, std::int64_t length);
byte_value encrypt(const byte_value& key, const byte_value& plaintext,
                   const byte_value& aad);
byte_value decrypt(const byte_value& key, const byte_value& encrypted,
                   const byte_value& aad);
byte_value sha256_stream(const binary_stream& source);
byte_value sha512_stream(const binary_stream& source);
byte_value hmac_sha256_stream(const secret::handle& key,
                              const binary_stream& source);
void encrypt_file(const secret::handle& key, std::string_view source_path,
                  std::string_view target_path, const byte_value& aad,
                  std::string_view key_id);
void decrypt_file(const secret::handle& key, std::string_view source_path,
                  std::string_view target_path, const byte_value& aad,
                  std::string_view expected_key_id);
std::string file_key_id(std::string_view path);

} // namespace tx_generated::crypto
