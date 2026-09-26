#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/priority_entry.hpp"

#include <any>
#include <cstdint>

namespace
{

using namespace tx_generated;
using namespace tx_generated::detail;

const std::any& object_value(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("priority_entry 值不能为 none");
    }
    return *static_cast<const std::any*>(value);
}

} // namespace

#define TX_PRIORITY_ENTRY(SUFFIX, ABI, VALUE) \
extern "C" int txrt_priority_entry_new_##SUFFIX(const char* type_name, \
    std::int64_t priority, ABI value, void** result) noexcept \
{ \
    return invoke_checked([&] \
    { \
        *result = make_handle<std::any>(make_priority_entry( \
            type_name, priority, VALUE)); \
    }); \
}

TX_PRIORITY_ENTRY(i64, std::int64_t, value)
TX_PRIORITY_ENTRY(f64, double, value)
TX_PRIORITY_ENTRY(bool, bool, value)
TX_PRIORITY_ENTRY(str, const void*, text_reference(value))
TX_PRIORITY_ENTRY(object, const void*, object_value(value))

#undef TX_PRIORITY_ENTRY
