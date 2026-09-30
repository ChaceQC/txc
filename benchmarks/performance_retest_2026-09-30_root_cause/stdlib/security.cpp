#include "common.hpp"
#include "stdlib/secret.hpp"
#include "stdlib/password.hpp"
#include "stdlib/public_key.hpp"
#include "stdlib/x509.hpp"
using namespace tx_generated;

void bench_security()
{
    auto data = make_bytes(read_file("message.bin"));
    auto first = secret::from_bytes(data);
    auto second = secret::from_bytes(data);
    std::int64_t total = 0;
    auto start = bench_clock::now();
    for (int i = 0; i < 20000; ++i)
    {
        total += secret::equal(first, second);
    }
    report("secret_equal", start, total);
    auto credential = secret::from_bytes(make_bytes(read_file("password.bin")));
    start = bench_clock::now();
    total = 0;
    for (int i = 0; i < 3; ++i)
    {
        auto encoded = password::hash_password_with_params(credential, 19456, 2, 1);
        total += password::verify_password(credential, encoded);
    }
    report("argon2_hash_verify", start, total);
    auto key = public_key::ed25519_import_seed(make_bytes(read_file("ed_seed.bin")));
    auto pub = public_key::ed25519_public(key);
    auto signature = public_key::ed25519_sign(key, data);
    start = bench_clock::now();
    total = 0;
    for (int i = 0; i < 500; ++i)
    {
        signature = public_key::ed25519_sign(key, data);
        total += signature->size();
    }
    report("ed25519_sign", start, total);
    start = bench_clock::now();
    total = 0;
    for (int i = 0; i < 500; ++i)
    {
        total += public_key::ed25519_verify(pub, data, signature);
    }
    report("ed25519_verify", start, total);
    auto cert = make_bytes(read_file("server.der"));
    start = bench_clock::now();
    total = 0;
    for (int i = 0; i < 500; ++i)
    {
        total += x509::parse_der(cert)->size();
    }
    report("x509_parse_der", start, total);
}
