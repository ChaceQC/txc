#include "stdlib/tls_material.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/error.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>

using namespace tx_generated;

int main(int count, char** arguments)
{
    assert(count == 2);
    std::ifstream input(arguments[1], std::ios::binary);
    std::vector<std::uint8_t> package{std::istreambuf_iterator<char>(input), {}};
    const std::string password_text = "test-only-password";
    auto password = secret::from_bytes(make_bytes({password_text.begin(), password_text.end()}));
    const auto id = tls::import_identity(make_bytes(std::move(package)), password);
    secret::close(password);
    auto owner = tls::get_identity(id);
    auto first = tls::acquire_identity_material(owner);
    auto second = tls::acquire_identity_material(owner);
    assert(first.get() != second.get());
    auto* reused = first.get();
    tls::release_identity_material(owner, std::move(first));
    auto third = tls::acquire_identity_material(owner);
    assert(third.get() == reused);
    std::weak_ptr<tls::identity_material> idle = third;
    tls::release_identity_material(owner, std::move(third));
    tls::close_identity(id);
    assert(idle.expired());
    assert(owner->available_materials.empty());
    assert(mbedtls_pk_get_type(&second->key) != MBEDTLS_PK_NONE);
    tls::release_identity_material(owner, std::move(second));
    assert(owner->available_materials.empty());
    bool rejected = false;
    try
    {
        tls::acquire_identity_material(owner);
    }
    catch (const runtime_failure&)
    {
        rejected = true;
    }
    assert(rejected);
    std::cout << "TLS_MATERIAL_CACHE_OK\n";
}
