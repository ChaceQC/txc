#include "stdlib/crypto.hpp"
#include "stdlib/error.hpp"
#include "stdlib/password.hpp"
#include "stdlib/public_key.hpp"
#include "stdlib/secret.hpp"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

std::atomic<int> calls_until_failure{-1};

using namespace tx_generated;

template<class operation>
void expect_failure(operation&& action, tx::error_kind kind,
                    const char* code)
{
    try
    {
        action();
    }
    catch (const runtime_failure& failure)
    {
        if (failure.error().kind == kind && failure.error().code == code)
        {
            return;
        }
        throw std::runtime_error("unexpected random failure code");
    }
    throw std::runtime_error("random failure was not reported");
}

DWORD handle_count()
{
    DWORD count = 0;
    if (!GetProcessHandleCount(GetCurrentProcess(), &count))
    {
        throw std::runtime_error("GetProcessHandleCount failed");
    }
    return count;
}

void require_original(const std::filesystem::path& target)
{
    std::ifstream input(target, std::ios::binary);
    const std::string value(std::istreambuf_iterator<char>{input}, {});
    if (value != "original target")
    {
        throw std::runtime_error("failed operation changed target");
    }
    const auto prefix = target.filename().string() + ".tx-crypt-";
    for (const auto& entry : std::filesystem::directory_iterator(
             target.parent_path()))
    {
        if (entry.path().filename().string().starts_with(prefix))
        {
            throw std::runtime_error("failed operation left temporary file");
        }
    }
}

void check_random_entry_points(const secret::handle& key,
                               const byte_value& raw_key,
                               const byte_value& empty)
{
    calls_until_failure = 0;
    expect_failure([]
    {
        (void)crypto::random_bytes(32);
    }, tx::error_kind::runtime, "random_failed");

    calls_until_failure = 1;
    expect_failure([]
    {
        (void)crypto::random_bytes(512);
    }, tx::error_kind::runtime, "random_failed");

    calls_until_failure = 0;
    expect_failure([]
    {
        (void)secret::random(32);
    }, tx::error_kind::security, "random_failed");

    calls_until_failure = 0;
    expect_failure([&]
    {
        (void)password::hash_password_with_params(key, 19456, 2, 1);
    }, tx::error_kind::security, "random_failed");

    calls_until_failure = 0;
    expect_failure([]
    {
        (void)public_key::ed25519_generate();
    }, tx::error_kind::security, "random_failed");

    calls_until_failure = 0;
    expect_failure([]
    {
        (void)public_key::x25519_generate();
    }, tx::error_kind::security, "random_failed");

    calls_until_failure = 0;
    expect_failure([&]
    {
        (void)crypto::encrypt(raw_key, empty, empty);
    }, tx::error_kind::runtime, "random_failed");
    calls_until_failure = -1;
}

void check_file_cleanup(const std::filesystem::path& directory,
                        const secret::handle& key,
                        const byte_value& empty)
{
    const auto source = directory / "random-source.bin";
    const auto encrypted = directory / "random-encrypted.bin";
    const auto target = directory / "random-target.bin";
    {
        std::ofstream output(source, std::ios::binary);
        output << "input data";
    }
    {
        std::ofstream output(target, std::ios::binary);
        output << "original target";
    }
    crypto::encrypt_file(key, source.string(), encrypted.string(), empty,
                         "random-test");
    for (int attempt = 0; attempt < 16; ++attempt)
    {
        // 第一次取随机数创建临时文件名，第二次生成文件 salt。
        calls_until_failure = 1;
        expect_failure([&]
        {
            crypto::encrypt_file(key, source.string(), target.string(),
                                 empty, "random-test");
        }, tx::error_kind::runtime, "random_failed");
        require_original(target);

        calls_until_failure = 0;
        expect_failure([&]
        {
            crypto::decrypt_file(key, encrypted.string(), target.string(),
                                 empty, "random-test");
        }, tx::error_kind::runtime, "random_failed");
        require_original(target);
    }
    calls_until_failure = -1;
}

} // namespace

extern "C" std::int32_t __real_psa_generate_random(std::uint8_t*, std::size_t);

extern "C" std::int32_t __wrap_psa_generate_random(std::uint8_t* output,
                                                      std::size_t size)
{
    const auto remaining = calls_until_failure.load();
    if (remaining == 0)
    {
        return -1;
    }
    if (remaining > 0)
    {
        calls_until_failure.fetch_sub(1);
    }
    return __real_psa_generate_random(output, size);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 1;
    }
    try
    {
        const std::filesystem::path directory(argv[1]);
        const std::vector<std::uint8_t> material(32, 0x25);
        const auto raw_key = make_bytes(material);
        auto key = secret::from_bytes(raw_key);
        const auto empty = make_bytes({});
        // 先初始化 PSA，随后只注入取数失败。
        (void)crypto::random_bytes(1);
        check_random_entry_points(key, raw_key, empty);
        const auto before = handle_count();
        check_file_cleanup(directory, key, empty);
        const auto after = handle_count();
        secret::close(key);
        if (after > before + 8)
        {
            throw std::runtime_error("resource handles grew after failure");
        }
        std::cout << "CRYPTO_RANDOM_CLEANUP_OK\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        calls_until_failure = -1;
        std::cerr << error.what() << '\n';
        return 2;
    }
}
