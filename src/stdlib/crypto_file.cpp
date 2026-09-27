#include "stdlib/crypto_file_io.hpp"
#include "stdlib/crypto_internal.hpp"
#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_extended.hpp"

#include <mbedtls/gcm.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace tx_generated::crypto
{
namespace
{

constexpr std::size_t block_size = 64 * 1024;
constexpr std::size_t fixed_header_size = 27;
constexpr std::size_t record_header_size = 9;
constexpr std::size_t tag_size = 16;
constexpr std::array<std::uint8_t, 6> prefix{'T', 'X', 'C', 'F', 1, 1};

struct file_header
{
    std::vector<std::uint8_t> encoded;
    std::array<std::uint8_t, 16> salt{};
    std::string key_id;
};

using gcm_owner = std::unique_ptr<mbedtls_gcm_context,
    decltype(&mbedtls_gcm_free)>;

void append_u32(std::span<std::uint8_t> target, std::uint32_t value)
{
    for (int index = 3; index >= 0; --index)
    {
        target[static_cast<std::size_t>(index)] =
            static_cast<std::uint8_t>(value);
        value >>= 8;
    }
}

std::uint32_t read_u32(std::span<const std::uint8_t> source)
{
    return (static_cast<std::uint32_t>(source[0]) << 24) |
           (static_cast<std::uint32_t>(source[1]) << 16) |
           (static_cast<std::uint32_t>(source[2]) << 8) | source[3];
}

void validate_key_id(std::string_view key_id, bool input)
{
    if (key_id.size() > 128)
    {
        fail(input ? "invalid_format" : "invalid_argument",
             "加密文件密钥标识超过 128 字节");
    }
    try
    {
        detail::validate_utf8(key_id);
    }
    catch (const std::runtime_error&)
    {
        fail(input ? "invalid_format" : "invalid_argument",
             "加密文件密钥标识不是有效 UTF-8");
    }
}

file_header create_header(std::string_view key_id)
{
    validate_key_id(key_id, false);
    file_header header;
    header.key_id = key_id;
    header.encoded.resize(fixed_header_size + key_id.size());
    std::copy(prefix.begin(), prefix.end(), header.encoded.begin());
    append_u32(std::span(header.encoded).subspan(6, 4), block_size);
    fill_random(header.salt);
    std::copy(header.salt.begin(), header.salt.end(),
              header.encoded.begin() + 10);
    header.encoded[26] = static_cast<std::uint8_t>(key_id.size());
    std::copy(key_id.begin(), key_id.end(), header.encoded.begin() + 27);
    return header;
}

file_header read_header(file_input& source)
{
    file_header header;
    header.encoded.resize(fixed_header_size);
    source.read_exact(header.encoded);
    if (!std::equal(prefix.begin(), prefix.end(), header.encoded.begin()) ||
        read_u32(std::span(header.encoded).subspan(6, 4)) != block_size ||
        header.encoded[26] > 128)
    {
        fail("invalid_format", "加密文件头格式或版本无效");
    }
    std::copy_n(header.encoded.begin() + 10, header.salt.size(),
                header.salt.begin());
    const auto length = header.encoded[26];
    header.encoded.resize(fixed_header_size + length);
    source.read_exact(std::span(header.encoded).subspan(fixed_header_size));
    header.key_id.assign(header.encoded.begin() + fixed_header_size,
                         header.encoded.end());
    validate_key_id(header.key_id, true);
    return header;
}

void require_key(const secret::handle& key)
{
    if (key->view().size() != 32)
    {
        fail("invalid_argument", "文件 AES-256-GCM 密钥必须恰好为 32 字节");
    }
}

void derive_file_key(const secret::handle& master, const file_header& header,
                     sensitive_buffer& derived)
{
    const auto material = master->view();
    std::string info = "TXCF-1";
    info += header.key_id;
    const auto* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md)
    {
        fail("operation_failed", "文件密钥派生算法不可用");
    }
    require_success(mbedtls_hkdf(md, header.salt.data(), header.salt.size(),
        material.data(), material.size(),
        reinterpret_cast<const unsigned char*>(info.data()), info.size(),
        derived.data(), derived.size()), "派生文件加密密钥失败");
}

std::array<std::uint8_t, record_header_size> record_header(
    std::uint32_t sequence, bool final, std::uint32_t length)
{
    std::array<std::uint8_t, record_header_size> result{};
    append_u32(std::span(result).first(4), sequence);
    result[4] = final ? 1 : 0;
    append_u32(std::span(result).subspan(5, 4), length);
    return result;
}

std::array<std::uint8_t, 12> record_nonce(std::uint32_t sequence)
{
    std::array<std::uint8_t, 12> result{};
    append_u32(std::span(result).subspan(8, 4), sequence);
    return result;
}

std::vector<std::uint8_t> authentication_data(
    const file_header& header, const byte_value& aad,
    std::span<const std::uint8_t> record)
{
    if (aad->size() > 1024 * 1024)
    {
        fail("invalid_argument", "文件附加认证数据超过 1 MiB");
    }
    std::vector<std::uint8_t> result;
    result.reserve(header.encoded.size() + aad->size() + record.size());
    result.insert(result.end(), header.encoded.begin(), header.encoded.end());
    result.insert(result.end(), aad->begin(), aad->end());
    result.insert(result.end(), record.begin(), record.end());
    return result;
}

void set_file_key(mbedtls_gcm_context& context,
                  sensitive_buffer& derived)
{
    require_success(mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES,
        derived.data(), 256), "设置文件 AES-256-GCM 密钥失败");
}

void write_record(file_output& target, mbedtls_gcm_context& context,
                  const file_header& header, const byte_value& aad,
                  std::uint32_t sequence, std::span<const std::uint8_t> plaintext,
                  bool final)
{
    const auto record = record_header(sequence, final,
        static_cast<std::uint32_t>(plaintext.size()));
    const auto auth = authentication_data(header, aad, record);
    const auto nonce = record_nonce(sequence);
    std::array<std::uint8_t, block_size> ciphertext{};
    std::array<std::uint8_t, tag_size> tag{};
    std::uint8_t empty = 0;
    require_success(mbedtls_gcm_crypt_and_tag(&context, MBEDTLS_GCM_ENCRYPT,
        plaintext.size(), nonce.data(), nonce.size(), auth.data(), auth.size(),
        plaintext.empty() ? &empty : plaintext.data(),
        plaintext.empty() ? &empty : ciphertext.data(), tag.size(), tag.data()),
        "加密文件块失败");
    target.write(record);
    target.write(std::span(ciphertext).first(plaintext.size()));
    target.write(tag);
}

bool read_record(file_input& source, file_output& target,
                 mbedtls_gcm_context& context, const file_header& header,
                 const byte_value& aad, std::uint32_t expected_sequence)
{
    std::array<std::uint8_t, record_header_size> record{};
    source.read_exact(record);
    const auto sequence = read_u32(std::span(record).first(4));
    const auto length = read_u32(std::span(record).subspan(5, 4));
    const bool final = record[4] == 1;
    if (sequence != expected_sequence || record[4] > 1 ||
        length > block_size || (final && length != 0) ||
        (!final && length == 0))
    {
        fail("invalid_format", "加密文件块序号、标志或长度无效");
    }
    std::array<std::uint8_t, block_size> ciphertext{};
    source.read_exact(std::span(ciphertext).first(length));
    std::array<std::uint8_t, tag_size> tag{};
    source.read_exact(tag);
    const auto auth = authentication_data(header, aad, record);
    const auto nonce = record_nonce(sequence);
    sensitive_buffer plaintext(length);
    std::uint8_t empty = 0;
    const int status = mbedtls_gcm_auth_decrypt(&context, length,
        nonce.data(), nonce.size(), auth.data(), auth.size(), tag.data(),
        tag.size(), length == 0 ? &empty : ciphertext.data(),
        length == 0 ? &empty : plaintext.data());
    if (status == MBEDTLS_ERR_GCM_AUTH_FAILED)
    {
        fail("authentication_failed", "加密文件块认证失败");
    }
    require_success(status, "解密文件块失败");
    if (!final)
    {
        target.write(plaintext.span());
    }
    return final;
}

} // namespace

void encrypt_file(const secret::handle& key, std::string_view source_path,
                  std::string_view target_path, const byte_value& aad,
                  std::string_view key_id)
{
    require_key(key);
    const auto source_name = detail::checked_path(std::string(source_path));
    const auto target_name = detail::checked_path(std::string(target_path));
    require_distinct_paths(source_name, target_name);
    file_input source(source_name);
    file_output target(target_name);
    const auto header = create_header(key_id);
    sensitive_buffer derived(32);
    derive_file_key(key, header, derived);
    mbedtls_gcm_context raw{};
    mbedtls_gcm_init(&raw);
    const gcm_owner context(&raw, &mbedtls_gcm_free);
    set_file_key(*context, derived);
    target.write(header.encoded);
    sensitive_buffer plaintext(block_size);
    std::uint32_t sequence = 0;
    while (true)
    {
        const auto count = source.read(plaintext.span());
        if (count == 0)
        {
            break;
        }
        if (sequence == std::numeric_limits<std::uint32_t>::max())
        {
            fail("size_limit", "加密文件块数量超出格式范围");
        }
        write_record(target, *context, header, aad, sequence++,
                     plaintext.span().first(count), false);
    }
    write_record(target, *context, header, aad, sequence, {}, true);
    target.commit(target_name);
}

void decrypt_file(const secret::handle& key, std::string_view source_path,
                  std::string_view target_path, const byte_value& aad,
                  std::string_view expected_key_id)
{
    require_key(key);
    validate_key_id(expected_key_id, false);
    const auto source_name = detail::checked_path(std::string(source_path));
    const auto target_name = detail::checked_path(std::string(target_path));
    require_distinct_paths(source_name, target_name);
    file_input source(source_name);
    const auto header = read_header(source);
    if (header.key_id != expected_key_id)
    {
        fail("authentication_failed", "加密文件密钥标识不匹配");
    }
    file_output target(target_name);
    sensitive_buffer derived(32);
    derive_file_key(key, header, derived);
    mbedtls_gcm_context raw{};
    mbedtls_gcm_init(&raw);
    const gcm_owner context(&raw, &mbedtls_gcm_free);
    set_file_key(*context, derived);
    for (std::uint32_t sequence = 0;; ++sequence)
    {
        if (read_record(source, target, *context, header, aad, sequence))
        {
            std::uint8_t trailing = 0;
            if (source.read(std::span(&trailing, 1)) != 0)
            {
                fail("invalid_format", "加密文件最终块后仍有数据");
            }
            target.commit(target_name);
            return;
        }
        if (sequence == std::numeric_limits<std::uint32_t>::max())
        {
            fail("size_limit", "加密文件块数量超出格式范围");
        }
    }
}

std::string file_key_id(std::string_view path)
{
    file_input source(detail::checked_path(std::string(path)));
    return read_header(source).key_id;
}

} // namespace tx_generated::crypto
