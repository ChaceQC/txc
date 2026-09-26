#pragma once

#include "common/error_kind.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace tx_generated::detail
{

extern thread_local char last_error[256];
extern thread_local char last_error_code[64];
extern thread_local tx::error_kind last_error_kind;
extern thread_local bool propagate_errors;

struct source_frame
{
    std::string function;
    std::string file;
    std::size_t line = 0;
    std::size_t column = 0;
};

extern thread_local std::vector<source_frame> active_stack;
extern thread_local std::vector<source_frame> last_error_stack;

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
void txrt_stack_push(const char* function, const char* file,
                     std::size_t line, std::size_t column) noexcept;
void txrt_stack_pop() noexcept;
void txrt_stack_location(const char* file, std::size_t line,
                          std::size_t column) noexcept;

}
