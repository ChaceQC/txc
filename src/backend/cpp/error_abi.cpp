#include "backend/cpp/error_abi.hpp"
#include "backend/cpp/error_result.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"

#include <any>
#include <cstdio>
#include <cstring>
#include <string>

namespace tx_generated::detail
{

thread_local char last_error_code[64]{};
thread_local tx::error_kind last_error_kind = tx::error_kind::none;
thread_local bool propagate_errors = false;

void set_error(tx::error_kind kind, const char* code, const char* message) noexcept
{
    last_error_kind = kind;
    std::snprintf(last_error_code, sizeof(last_error_code), "%s", code);
    std::snprintf(last_error, 256, "%s", message);
}

error_cleanup_guard::error_cleanup_guard() noexcept : kind_(last_error_kind)
{
    if (kind_ != tx::error_kind::none)
    {
        std::memcpy(code_, last_error_code, sizeof(code_));
        std::memcpy(message_, last_error, sizeof(message_));
        last_error_kind = tx::error_kind::none;
    }
}

error_cleanup_guard::~error_cleanup_guard()
{
    if (kind_ != tx::error_kind::none)
    {
        last_error_kind = kind_;
        std::memcpy(last_error_code, code_, sizeof(code_));
        std::memcpy(last_error, message_, sizeof(message_));
    }
}

dynamic_struct make_error_value(const char* type_name, const error_info& error)
{
    struct_fields fields(3);
    fields[0] = {"kind", std::string(error_kind_name(error.kind))};
    fields[1] = {"code", error.code};
    fields[2] = {"message", error.message};
    return dynamic_struct(dynamic_struct_data{type_name, "error_info", std::move(fields)});
}

} // namespace tx_generated::detail

extern "C" int txrt_error_status() noexcept
{
    return static_cast<int>(tx_generated::detail::last_error_kind);
}

extern "C" void txrt_error_propagation(bool enabled) noexcept
{
    tx_generated::detail::propagate_errors = enabled;
}

extern "C" int txrt_error_take(const char* type_name, void** result) noexcept
{
    using namespace tx_generated::detail;
    // 清除发生在结果构造之前，构造失败将成为交给外层处理器的新错误。
    const auto kind = last_error_kind;
    char code[64];
    char message[256];
    std::memcpy(code, last_error_code, sizeof(code));
    std::memcpy(message, last_error, sizeof(message));
    last_error_kind = tx::error_kind::none;
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(make_error_value(type_name, {kind, code, message}));
    });
}

extern "C" int txrt_error_fail_io(const void* code,
                                   const void* message) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        throw tx_generated::runtime_failure({
            tx::error_kind::io,
            *static_cast<const std::string*>(code),
            *static_cast<const std::string*>(message)});
    });
}
