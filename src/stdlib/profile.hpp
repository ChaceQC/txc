#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace tx_generated
{
namespace detail
{
struct runtime_context;
}

void profile_register_thread(detail::runtime_context** context) noexcept;
void profile_unregister_thread() noexcept;
void profile_start(std::int64_t interval_ms, std::int64_t max_allocations);
std::string profile_snapshot(bool stop);
std::int64_t profile_begin_span(const std::string& name);
std::int64_t profile_end_span(std::int64_t id);
void profile_allocated(void* pointer, std::size_t bytes) noexcept;
void profile_freed(void* pointer) noexcept;
void profile_link_allocator() noexcept;

} // namespace tx_generated
