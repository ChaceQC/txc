#pragma once

#include "common/error_kind.hpp"

namespace tx_generated::detail
{

extern thread_local char last_error[256];
extern thread_local char last_error_code[64];
extern thread_local tx::error_kind last_error_kind;
extern thread_local bool propagate_errors;

void set_error(tx::error_kind kind, const char* code, const char* message) noexcept;

// 析构回调在干净的错误状态下运行，退出时恢复最初的错误。
class error_cleanup_guard
{
public:
    error_cleanup_guard() noexcept;
    ~error_cleanup_guard();

private:
    tx::error_kind kind_;
    char code_[64];
    char message_[256];
};

} // namespace tx_generated::detail

extern "C"
{

int txrt_error_status() noexcept;
void txrt_error_propagation(bool enabled) noexcept;
int txrt_error_take(const char* type_name, void** result) noexcept;
int txrt_error_fail_io(const void* code, const void* message) noexcept;

}
