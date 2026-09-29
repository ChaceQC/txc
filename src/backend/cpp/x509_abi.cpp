#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/x509.hpp"

#include <any>
#include <string>
#include <utility>

namespace
{

const tx_generated::byte_value& byte_argument(const void* value)
{
    return tx_generated::bytes_of(*static_cast<const std::any*>(value));
}

const tx_generated::secret::handle& secret_argument(const void* value)
{
    return std::any_cast<const tx_generated::secret::handle&>(
        *static_cast<const std::any*>(value));
}

const tx_generated::bytes_vector& vector_argument(const void* value)
{
    return std::any_cast<const tx_generated::bytes_vector&>(
        *static_cast<const std::any*>(value));
}

void* certificate_vector(std::vector<tx_generated::byte_value> values)
{
    tx_generated::bytes_vector vector;
    vector.data().values = std::move(values);
    vector.data().refresh();
    return tx_generated::detail::make_handle<std::any>(std::move(vector));
}

} // namespace

extern "C" int txrt_x509_parse_pem(const void* text, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = certificate_vector(tx_generated::x509::parse_pem(
            tx_generated::detail::text_value(text)));
    });
}

extern "C" int txrt_x509_parse_der(const void* data, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::x509::parse_der(byte_argument(data)));
    });
}

extern "C" int txrt_x509_parse_pkcs12(const void* data,
    const void* password, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = certificate_vector(tx_generated::x509::parse_pkcs12(
            byte_argument(data), secret_argument(password)));
    });
}

extern "C" int txrt_x509_pkcs12_private_key(const void* data,
    const void* password, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::x509::pkcs12_private_key(
                byte_argument(data), secret_argument(password)));
    });
}

extern "C" int txrt_x509_verify(const void* leaf,
    const void* intermediates, const void* trust_anchors, const void* hostname,
    const void* purpose, bool system_trust, const char* type_name,
    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto verified = tx_generated::x509::verify(byte_argument(leaf),
            vector_argument(intermediates), vector_argument(trust_anchors),
            tx_generated::detail::text_value(hostname),
            tx_generated::detail::text_value(purpose), system_trust);
        tx_generated::bytes_vector chain;
        chain.data().values = std::move(verified.chain);
        chain.data().refresh();
        tx_generated::struct_fields fields(3);
        fields[0] = {"status", std::move(verified.status)};
        fields[1] = {"revocation", std::move(verified.revocation)};
        fields[2] = {"chain", std::move(chain)};
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::dynamic_struct(tx_generated::dynamic_struct_data{
                type_name, "verification", std::move(fields)}));
    });
}
