#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/tls_abi_helpers.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/tls.hpp"
#include "stdlib/x509.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace
{

using tx_generated::bytes_vector;
using tx_generated::dynamic_struct;
using tx_generated::struct_fields;

template<class value_type>
const value_type& value(const void* handle)
{
    return std::any_cast<const value_type&>(*static_cast<const std::any*>(handle));
}

template<class value_type>
const value_type& field(const dynamic_struct& object, std::size_t index)
{
    return std::any_cast<const value_type&>(object->fields[index].value);
}

[[noreturn]] void fail(const char* code, const char* message)
{
    throw tx_generated::runtime_failure({tx::error_kind::security, code, message});
}

void* make_struct(const char* type_name, const char* local_name,
                  struct_fields fields)
{
    return tx_generated::detail::make_handle<std::any>(dynamic_struct(
        tx_generated::dynamic_struct_data{type_name, local_name,
            std::move(fields)}));
}

std::int64_t identity_id(const dynamic_struct& object)
{
    return field<std::int64_t>(object, 0);
}

dynamic_struct identity_value(const char* type_name, std::int64_t id)
{
    struct_fields fields(1);
    fields[0] = {"id", id};
    return dynamic_struct(tx_generated::dynamic_struct_data{
        type_name, "identity", std::move(fields)});
}

struct trust_value
{
    bool system_roots = false;
    bytes_vector anchors;
};

void validate_trust(const trust_value& roots)
{
    const auto& anchors = roots.anchors.data().values;
    if (!roots.system_roots && anchors.empty())
    {
        fail("invalid_argument", "TLS 信任配置不能同时关闭系统根且不提供锚");
    }
    if (anchors.size() > 64)
    {
        fail("size_limit", "TLS 信任锚超过 64 张");
    }
    for (const auto& anchor : anchors)
    {
        (void)tx_generated::x509::parse_der(anchor);
    }
}

trust_value read_trust(const dynamic_struct& object)
{
    trust_value roots{field<bool>(object, 0),
        field<bytes_vector>(object, 1).copy()};
    validate_trust(roots);
    return roots;
}

dynamic_struct trust_struct(const char* type_name, const trust_value& roots)
{
    struct_fields fields(2);
    fields[0] = {"system_roots", roots.system_roots};
    fields[1] = {"anchors", roots.anchors.copy()};
    return dynamic_struct(tx_generated::dynamic_struct_data{
        type_name, "trust", std::move(fields)});
}

void validate_hostname(std::string_view hostname)
{
    if (hostname.empty() || hostname.size() > 253)
    {
        fail("invalid_argument", "TLS 主机名为空或超过 253 字节");
    }
    const auto first_colon = hostname.find(':');
    if (first_colon != std::string_view::npos &&
        first_colon == hostname.rfind(':'))
    {
        fail("invalid_argument", "TLS 主机名不能包含端口");
    }
    for (const auto character : hostname)
    {
        const auto byte = static_cast<unsigned char>(character);
        if (byte <= 0x20 || byte == 0x7f || character == '/' ||
            character == '\\' || character == '@' || character == '?' ||
            character == '#' || character == '[' || character == ']')
        {
            fail("invalid_argument", "TLS 主机名包含 URL、空白或控制字符");
        }
    }
}

struct client_value
{
    std::string hostname;
    trust_value roots;
    std::int64_t identity = 0;
};

client_value read_client(const dynamic_struct& object)
{
    client_value result{field<std::string>(object, 0),
        read_trust(field<dynamic_struct>(object, 1)),
        identity_id(field<dynamic_struct>(object, 2))};
    validate_hostname(result.hostname);
    if (result.identity != 0)
    {
        (void)tx_generated::tls::get_identity(result.identity);
    }
    return result;
}

dynamic_struct client_struct(const char* type_name, const client_value& config,
                             const char* trust_type, const char* identity_type)
{
    struct_fields fields(3);
    fields[0] = {"hostname", config.hostname};
    fields[1] = {"trust", trust_struct(trust_type, config.roots)};
    fields[2] = {"client_identity", identity_value(identity_type,
        config.identity)};
    return dynamic_struct(tx_generated::dynamic_struct_data{
        type_name, "client_config", std::move(fields)});
}

struct server_value
{
    std::int64_t identity;
    trust_value roots;
    bool require_client_identity;
};

server_value read_server(const dynamic_struct& object)
{
    server_value result{identity_id(field<dynamic_struct>(object, 0)),
        read_trust(field<dynamic_struct>(object, 1)),
        field<bool>(object, 2)};
    (void)tx_generated::tls::get_identity(result.identity);
    return result;
}

void* verified_peer(const char* type_name, const tx_generated::byte_value& leaf,
                    const bytes_vector& intermediates, const trust_value& roots,
                    std::string_view hostname, std::string_view purpose)
{
    if (leaf->empty())
    {
        fail("certificate_required", "TLS 对端没有提供证书");
    }
    auto checked = tx_generated::x509::verify(leaf, intermediates,
        roots.anchors, hostname, purpose, roots.system_roots);
    if (checked.status != "valid")
    {
        throw tx_generated::runtime_failure({tx::error_kind::security,
            checked.status, "TLS 对端证书验证失败：" + checked.status});
    }
    bytes_vector chain;
    chain.data().values = std::move(checked.chain);
    chain.data().refresh();
    struct_fields fields(3);
    fields[0] = {"status", std::move(checked.status)};
    fields[1] = {"revocation", std::move(checked.revocation)};
    fields[2] = {"chain", std::move(chain)};
    return make_struct(type_name, "verification", std::move(fields));
}

} // namespace

extern "C" int txrt_tls_system_trust(const char* type_name,
                                      void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(
            trust_struct(type_name, trust_value{true, bytes_vector{}}));
    }, tx::error_kind::security);
}

extern "C" int txrt_tls_custom_trust(const void* anchors, bool include_system,
                                      const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        trust_value roots{include_system, value<bytes_vector>(anchors).copy()};
        validate_trust(roots);
        *result = tx_generated::detail::make_handle<std::any>(
            trust_struct(type_name, roots));
    }, tx::error_kind::security);
}

extern "C" int txrt_tls_import_identity(const void* package,
    const void* password, const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        std::int64_t id = 0;
        try
        {
            id = tx_generated::tls::import_identity(
                value<tx_generated::byte_value>(package),
                value<tx_generated::secret::handle>(password));
            *result = tx_generated::detail::make_handle<std::any>(
                identity_value(type_name, id));
        }
        catch (...)
        {
            tx_generated::tls::close_identity(id);
            throw;
        }
    }, tx::error_kind::security);
}

extern "C" int txrt_tls_close_identity(const void* identity) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        tx_generated::tls::close_identity(identity_id(value<dynamic_struct>(identity)));
    }, tx::error_kind::security);
}

extern "C" int txrt_tls_client(const void* hostname, const void* roots,
    const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        client_value config{*static_cast<const std::string*>(hostname),
            read_trust(value<dynamic_struct>(roots)), 0};
        validate_hostname(config.hostname);
        // 嵌套结构沿用参数中的真实类型名，避免跨模块结构身份丢失。
        const auto& roots_type = value<dynamic_struct>(roots)->type_name;
        auto identity_type = std::string(type_name);
        identity_type.replace(identity_type.size() - std::string_view("client_config").size(),
            std::string_view("client_config").size(), "identity");
        *result = tx_generated::detail::make_handle<std::any>(client_struct(
            type_name, config, roots_type.c_str(), identity_type.c_str()));
    }, tx::error_kind::security);
}

extern "C" int txrt_tls_with_client_identity(const void* config,
    const void* identity, const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto selected = read_client(value<dynamic_struct>(config));
        selected.identity = identity_id(value<dynamic_struct>(identity));
        (void)tx_generated::tls::get_identity(selected.identity);
        const auto& original = value<dynamic_struct>(config);
        const auto& roots_type = field<dynamic_struct>(original, 1)->type_name;
        const auto& identity_type = value<dynamic_struct>(identity)->type_name;
        *result = tx_generated::detail::make_handle<std::any>(client_struct(
            type_name, selected, roots_type.c_str(), identity_type.c_str()));
    }, tx::error_kind::security);
}

extern "C" int txrt_tls_server(const void* identity, const void* client_roots,
    bool require_client_identity, const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto id = identity_id(value<dynamic_struct>(identity));
        (void)tx_generated::tls::get_identity(id);
        auto roots = read_trust(value<dynamic_struct>(client_roots));
        struct_fields fields(3);
        fields[0] = {"identity", value<dynamic_struct>(identity)};
        fields[1] = {"client_trust", trust_struct(
            value<dynamic_struct>(client_roots)->type_name.c_str(), roots)};
        fields[2] = {"require_client_identity", require_client_identity};
        *result = make_struct(type_name, "server_config", std::move(fields));
    }, tx::error_kind::security);
}

extern "C" int txrt_tls_verify_server(const void* config, const void* leaf,
    const void* intermediates, const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto selected = read_client(value<dynamic_struct>(config));
        *result = verified_peer(type_name, value<tx_generated::byte_value>(leaf),
            value<bytes_vector>(intermediates), selected.roots,
            selected.hostname, "server_auth");
    }, tx::error_kind::security);
}

extern "C" int txrt_tls_verify_client(const void* config, const void* leaf,
    const void* intermediates, const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto selected = read_server(value<dynamic_struct>(config));
        *result = verified_peer(type_name, value<tx_generated::byte_value>(leaf),
            value<bytes_vector>(intermediates), selected.roots, "",
            "client_auth");
    }, tx::error_kind::security);
}

namespace tx_generated::tls_abi
{

tls::client_options checked_client(const void* config)
{
    const auto selected = read_client(value<dynamic_struct>(config));
    return {selected.hostname,
        {selected.roots.system_roots, selected.roots.anchors.copy()},
        selected.identity};
}

tls::server_options checked_server(const void* config)
{
    const auto selected = read_server(value<dynamic_struct>(config));
    return {selected.identity,
        {selected.roots.system_roots, selected.roots.anchors.copy()},
        selected.require_client_identity};
}

} // namespace tx_generated::tls_abi
