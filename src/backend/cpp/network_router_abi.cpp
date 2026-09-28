#include "backend/cpp/network_abi_helpers.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/httpx_router.hpp"

#include <any>
#include <utility>

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using namespace tx_generated::network_abi;

extern "C" int txrt_httpx_match_route(const void* route_method,
    const void* pattern, const void* request_method, const void* target,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto value = tx_generated::httpx_router::match(text_at(route_method),
            text_at(pattern), text_at(request_method), text_at(target));
        tx_generated::struct_fields fields(2);
        fields[0] = {"matched", value.matched};
        fields[1] = {"params", write_headers(value.params)};
        *result = make_handle<std::any>(tx_generated::dynamic_struct(
            {type_name, "route_match", std::move(fields)}));
    }, tx::error_kind::io);
}
