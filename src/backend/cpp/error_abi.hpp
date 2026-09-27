#pragma once

#include "backend/cpp/runtime_context.hpp"

#include <cstddef>
#include <vector>

namespace tx_generated::detail
{

void set_error(tx::error_kind kind, const char* code, const char* message) noexcept;

// 析构回调在干净的错误状态下运行，退出时恢复最初的错误。
class error_cleanup_guard
{
public:
    error_cleanup_guard() noexcept;
    ~error_cleanup_guard();

private:
    runtime_context& context_;
    tx::error_kind kind_;
    char code_[64];
    char message_[256];
    std::vector<source_frame> stack_;
};

} // namespace tx_generated::detail

extern "C"
{

int txrt_error_status() noexcept;
void txrt_error_propagation(bool enabled) noexcept;
int txrt_error_take(const char* type_name, void** result) noexcept;
int txrt_error_fail_io(const void* code, const void* message) noexcept;
int txrt_error_stack_trace(void** result) noexcept;
void txrt_stack_error_location(void* context, const char* file, std::size_t line,
                                std::size_t column) noexcept;

}
