#include "stdlib/password.hpp"

#include "stdlib/error.hpp"

// 与 libsodium 静态归档中的 Argon2 实现隔离公共符号。
#define argon2id_hash_encoded tx_argon2id_hash_encoded
#define argon2id_verify tx_argon2id_verify
#include <argon2.h>

#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace tx_generated::password
{
namespace
{

constexpr std::uint32_t default_memory_kib = 65536;
constexpr std::uint32_t default_iterations = 3;
constexpr std::uint32_t default_parallelism = 1;
constexpr std::uint32_t min_new_memory_kib = 19 * 1024;
constexpr std::uint32_t max_memory_kib = 256 * 1024;
constexpr std::uint32_t max_iterations = 10;
constexpr std::uint32_t max_parallelism = 4;
constexpr std::uint32_t salt_size = 16;
constexpr std::uint32_t tag_size = 32;

struct parameters
{
    std::uint32_t version = ARGON2_VERSION_13;
    std::uint32_t memory_kib = 0;
    std::uint32_t iterations = 0;
    std::uint32_t parallelism = 0;
};

[[noreturn]] void fail(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::security, code, message});
}

parameters validate_new(std::int64_t memory_kib,
                        std::int64_t iterations,
                        std::int64_t parallelism)
{
    if (memory_kib < min_new_memory_kib || memory_kib > max_memory_kib ||
        iterations < 2 || iterations > max_iterations ||
        parallelism < 1 || parallelism > max_parallelism)
    {
        fail("invalid_argument", "Argon2id 参数超出安全范围");
    }
    return {ARGON2_VERSION_13, static_cast<std::uint32_t>(memory_kib),
        static_cast<std::uint32_t>(iterations),
        static_cast<std::uint32_t>(parallelism)};
}

std::uint32_t parse_number(std::string_view text)
{
    std::uint32_t value = 0;
    const auto [end, error] = std::from_chars(text.data(),
        text.data() + text.size(), value);
    if (text.empty() || error != std::errc{} ||
        end != text.data() + text.size())
    {
        fail("invalid_format", "Argon2id PHC 字符串无效");
    }
    return value;
}

std::array<std::string_view, 6> split_phc(std::string_view encoded)
{
    if (encoded.size() > 512)
    {
        fail("invalid_format", "Argon2id PHC 字符串过长");
    }
    std::array<std::string_view, 6> parts;
    for (std::size_t index = 0; index < 5; ++index)
    {
        const auto separator = encoded.find('$');
        if (separator == std::string_view::npos)
        {
            fail("invalid_format", "Argon2id PHC 字符串无效");
        }
        parts[index] = encoded.substr(0, separator);
        encoded.remove_prefix(separator + 1);
    }
    if (encoded.find('$') != std::string_view::npos)
    {
        fail("invalid_format", "Argon2id PHC 字符串无效");
    }
    parts[5] = encoded;
    return parts;
}

bool valid_phc_base64(std::string_view text)
{
    if (text.size() < 22 || text.size() > 86 ||
        (text.size() * 6) / 8 < 16 || (text.size() * 6) / 8 > 64 ||
        text.size() % 4 == 1)
    {
        return false;
    }
    for (const unsigned char ch : text)
    {
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
              (ch >= '0' && ch <= '9') || ch == '+' || ch == '/'))
        {
            return false;
        }
    }
    const auto final = text.back();
    const int last_digit = final >= 'A' && final <= 'Z' ? final - 'A' :
        final >= 'a' && final <= 'z' ? final - 'a' + 26 :
        final >= '0' && final <= '9' ? final - '0' + 52 :
        final == '+' ? 62 : 63;
    if ((text.size() % 4 == 2 && (last_digit & 15) != 0) ||
        (text.size() % 4 == 3 && (last_digit & 3) != 0))
    {
        return false;
    }
    return true;
}

parameters parse_phc(std::string_view encoded)
{
    const auto parts = split_phc(encoded);
    if (!parts[0].empty() || parts[1] != "argon2id" ||
        !parts[2].starts_with("v=") || !parts[3].starts_with("m=") ||
        !valid_phc_base64(parts[4]) || !valid_phc_base64(parts[5]))
    {
        fail("invalid_format", "Argon2id PHC 字符串无效");
    }
    const auto first = parts[3].find(',');
    const auto second = first == std::string_view::npos
        ? first : parts[3].find(',', first + 1);
    if (first == std::string_view::npos ||
        second == std::string_view::npos ||
        !parts[3].substr(first + 1).starts_with("t=") ||
        !parts[3].substr(second + 1).starts_with("p="))
    {
        fail("invalid_format", "Argon2id PHC 参数无效");
    }
    const parameters result{
        parse_number(parts[2].substr(2)),
        parse_number(parts[3].substr(2, first - 2)),
        parse_number(parts[3].substr(first + 3, second - first - 3)),
        parse_number(parts[3].substr(second + 3))};
    if ((result.version != ARGON2_VERSION_10 &&
         result.version != ARGON2_VERSION_13) ||
        result.parallelism < 1 || result.parallelism > max_parallelism ||
        result.memory_kib < 8 * result.parallelism ||
        result.memory_kib > max_memory_kib ||
        result.iterations < 1 || result.iterations > max_iterations)
    {
        fail("invalid_format", "Argon2id PHC 参数超出可验证范围");
    }
    return result;
}

std::string calculate(const secret::handle& value, const parameters& options)
{
    const auto password_bytes = value->view();
    const auto salt = secret::random(salt_size);
    const auto salt_bytes = salt->view();
    std::string encoded(argon2_encodedlen(options.iterations,
        options.memory_kib, options.parallelism, salt_size, tag_size,
        Argon2_id), '\0');
    const int status = argon2id_hash_encoded(options.iterations,
        options.memory_kib, options.parallelism, password_bytes.data(),
        password_bytes.size(), salt_bytes.data(), salt_bytes.size(), tag_size,
        encoded.data(), encoded.size());
    if (status != ARGON2_OK)
    {
        fail("operation_failed", "计算 Argon2id 密码哈希失败");
    }
    encoded.resize(std::strlen(encoded.c_str()));
    return encoded;
}

} // namespace

std::string hash_password(const secret::handle& value)
{
    return calculate(value, {ARGON2_VERSION_13, default_memory_kib,
        default_iterations, default_parallelism});
}

std::string hash_password_with_params(const secret::handle& value,
                                      std::int64_t memory_kib,
                                      std::int64_t iterations,
                                      std::int64_t parallelism)
{
    return calculate(value, validate_new(memory_kib, iterations,
                                         parallelism));
}

bool verify_password(const secret::handle& value, std::string_view encoded)
{
    (void)parse_phc(encoded);
    const auto password_bytes = value->view();
    const std::string text(encoded);
    const int status = argon2id_verify(text.c_str(), password_bytes.data(),
                                       password_bytes.size());
    if (status == ARGON2_VERIFY_MISMATCH)
    {
        return false;
    }
    if (status == ARGON2_DECODING_FAIL ||
        status == ARGON2_DECODING_LENGTH_FAIL ||
        status == ARGON2_INCORRECT_TYPE)
    {
        fail("invalid_format", "Argon2id PHC 字符串无效");
    }
    if (status != ARGON2_OK)
    {
        fail("operation_failed", "验证 Argon2id 密码哈希失败");
    }
    return true;
}

bool needs_rehash(std::string_view encoded, std::int64_t memory_kib,
                  std::int64_t iterations, std::int64_t parallelism)
{
    const auto target = validate_new(memory_kib, iterations, parallelism);
    const auto stored = parse_phc(encoded);
    return stored.version != target.version ||
        stored.memory_kib != target.memory_kib ||
        stored.iterations != target.iterations ||
        stored.parallelism != target.parallelism;
}

} // namespace tx_generated::password
