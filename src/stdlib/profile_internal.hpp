#pragma once

#include "stdlib/profile.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#endif

namespace tx_generated::profiling
{

using clock_type = std::chrono::steady_clock;

struct sampled_thread
{
#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    pthread_t handle{};
    clockid_t cpu_clock{};
#endif
    detail::runtime_context** context = nullptr;
    std::uint64_t cpu = 0;
};

struct allocation
{
    std::size_t bytes = 0;
    detail::source_frame source;
};

struct cpu_sample
{
    detail::source_frame source;
#ifndef _WIN32
    std::string source_file;
    std::string source_function;
#endif
    std::string abi;
    std::uint64_t instruction = 0;
    std::uint64_t cpu_100ns = 0;
    std::uint64_t count = 0;
};

struct span
{
    std::string name;
    clock_type::time_point start;
    std::int64_t duration_us = -1;
};

struct profile_state
{
    std::mutex mutex;
    std::mutex control;
    std::atomic_bool enabled{false};
    std::jthread sampler;
#ifdef _WIN32
    std::map<DWORD, sampled_thread> threads;
#else
    std::map<pthread_t, sampled_thread> threads;
#endif
    std::unordered_map<void*, allocation> allocations;
    std::map<std::string, cpu_sample> samples;
    std::map<std::int64_t, span> spans;
    clock_type::time_point start;
    std::int64_t elapsed_us = 0;
    std::uint64_t sampler_us = 0;
    std::uint64_t allocated_bytes = 0;
    std::uint64_t allocation_count = 0;
    std::uint64_t dropped_allocations = 0;
    std::uint64_t missed_samples = 0;
    std::size_t maximum = 0;
    std::int64_t interval_ms = 0;
    std::int64_t next_span = 1;
};

extern thread_local bool inside_profiler;
struct internal_guard
{
    bool previous = inside_profiler;
    internal_guard()
    {
        inside_profiler = true;
    }
    ~internal_guard()
    {
        inside_profiler = previous;
    }
};

profile_state& state();
void sample_loop(std::stop_token token);
std::string report_locked(profile_state& current);

} // namespace tx_generated::profiling
