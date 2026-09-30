#pragma once

#include "backend/cpp/cycle_gc.hpp"
#include "common/error_kind.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <type_traits>
#include <vector>

namespace tx_generated::detail
{

struct handle_link;

struct source_frame
{
    // 名称和文件名借用生成程序的静态字节，错误快照不延长函数栈的寿命。
    const char* function = "";
    const char* file = "";
    std::size_t line = 0;
    std::size_t column = 0;
};

struct diagnostic_frame
{
    source_frame source;
    diagnostic_frame* parent = nullptr;
};

struct runtime_context;

struct registered_gc_node
{
    std::weak_ptr<void> object;
    gc_trace trace;
    gc_clear clear;
    gc_finalize finalize;
    std::uint64_t owner = 0;
};

struct runtime_context
{
    // 前两个字段由 LLVM 直接访问；其余字段只由运行时管理。
    diagnostic_frame* active_frame = nullptr;
    tx::error_kind last_error_kind = tx::error_kind::none;
    char last_error_code[64]{};
    char last_error[256]{};
    bool propagate_errors = false;
    std::vector<source_frame> last_error_stack;
    handle_link* newest_handle = nullptr;
    bool cleaning_handles = false;
    std::size_t allocations_since_collection = 0;
    bool collecting = false;
    std::size_t concurrent_depth = 0;
    std::uint64_t gc_owner_id = 0;
    std::mt19937_64 random_engine;
    bool random_initialized = false;
    std::shared_ptr<void> gc_workspace;
    std::string log_task_id;
    std::string log_thread_id;
    std::string log_request_id;
    const char* profile_abi = "";
    diagnostic_frame* profile_abi_frame = nullptr;
};

// 对应 %tx_runtime_context = { ptr, i32 } 和 %tx_diagnostic_frame。
static_assert(std::is_standard_layout_v<runtime_context>);
static_assert(offsetof(runtime_context, active_frame) == 0);
static_assert(offsetof(runtime_context, last_error_kind) == 8);
static_assert(sizeof(tx::error_kind) == 4);
static_assert(std::is_standard_layout_v<diagnostic_frame>);
static_assert(offsetof(diagnostic_frame, source) == 0);
static_assert(offsetof(diagnostic_frame, parent) == 32);
static_assert(sizeof(diagnostic_frame) == 40);
static_assert(offsetof(source_frame, file) == 8);
static_assert(offsetof(source_frame, line) == 16);
static_assert(offsetof(source_frame, column) == 24);

extern constinit thread_local runtime_context* thread_context;
runtime_context& initialize_runtime_context() noexcept;

inline runtime_context& current_runtime_context() noexcept
{
    // constinit 指针没有动态初始化守卫，热路径只查一次 TLS。
    if (auto* context = thread_context)
    {
        return *context;
    }
    return initialize_runtime_context();
}

std::vector<source_frame> capture_stack(const runtime_context& context);

} // namespace tx_generated::detail

extern "C" void* txrt_runtime_context() noexcept;
