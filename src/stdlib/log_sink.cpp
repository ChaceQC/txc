#include "stdlib/log.hpp"

#include "stdlib/error.hpp"
#include "stdlib/stdlib.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace tx_generated
{
namespace
{

struct log_sink
{
    std::mutex mutex;
    std::ofstream file;
    std::filesystem::path path;
    std::uint64_t size = 0;
    std::uint64_t maximum = 0;
    int backups = 0;
    std::vector<std::string> secret_keys;
    std::atomic_int minimum{0};
};

log_sink& sink()
{
    // 退出期间 detached 线程仍可能记录日志，避免静态析构与写入竞态。
    static auto* state = new log_sink();
    return *state;
}

int level_number(const std::string& level)
{
    const char* levels[] = {"trace", "debug", "info", "warn", "error"};
    for (int index = 0; index < 5; ++index)
    {
        if (level == levels[index])
        {
            return index;
        }
    }
    throw runtime_failure({tx::error_kind::runtime, "invalid_level",
        "日志级别必须是 trace/debug/info/warn/error"});
}

[[noreturn]] void io_failure()
{
    throw runtime_failure({tx::error_kind::io, "log_io_error",
        "日志写入、刷新或轮转失败；已完成的文件操作不会回滚"});
}

void rotate(log_sink& state)
{
    state.file.close();
    if (state.file.fail())
    {
        io_failure();
    }
    for (int index = state.backups; index >= 1; --index)
    {
        auto destination = state.path;
        destination += "." + std::to_string(index);
        auto source = state.path;
        if (index > 1)
        {
            source += "." + std::to_string(index - 1);
        }
        std::error_code error;
        std::filesystem::remove(destination, error);
        if (error)
        {
            io_failure();
        }
        if (std::filesystem::exists(source))
        {
            std::filesystem::rename(source, destination, error);
            if (error)
            {
                io_failure();
            }
        }
    }
    state.file.open(state.path, std::ios::binary | std::ios::trunc);
    if (!state.file)
    {
        io_failure();
    }
    state.size = 0;
}

} // namespace

bool tx_log_enabled(const std::string& level)
{
    return level_number(level) >= sink().minimum.load(std::memory_order_relaxed);
}

void tx_log_set_level(const std::string& level)
{
    sink().minimum.store(level_number(level), std::memory_order_relaxed);
}

void tx_log_set_file(const std::string& path, std::int64_t maximum, std::int64_t backups)
{
    if (path.empty() || path.find('\0') != std::string::npos || maximum < 256 ||
        maximum > 1073741824 || backups < 1 || backups > 32)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_log_options",
            "日志路径或轮转限额无效"});
    }
    auto& state = sink();
    std::lock_guard lock(state.mutex);
    auto target = std::filesystem::absolute(std::filesystem::path(
        std::u8string(reinterpret_cast<const char8_t*>(path.data()), path.size())));
    std::ofstream pending(target, std::ios::binary | std::ios::app);
    if (!pending)
    {
        io_failure();
    }
    std::error_code error;
    const auto size = std::filesystem::file_size(target, error);
    if (error)
    {
        io_failure();
    }
    if (state.file.is_open())
    {
        state.file.flush();
        if (!state.file)
        {
            io_failure();
        }
    }
    state.file = std::move(pending);
    state.path = std::move(target);
    state.size = size;
    state.maximum = maximum;
    state.backups = static_cast<int>(backups);
}

void tx_log_set_stderr()
{
    auto& state = sink();
    std::lock_guard lock(state.mutex);
    if (state.file.is_open())
    {
        state.file.close();
        if (state.file.fail())
        {
            io_failure();
        }
    }
    state.path.clear();
}

void tx_log_flush()
{
    auto& state = sink();
    std::lock_guard lock(state.mutex);
    if (!state.path.empty())
    {
        state.file.flush();
        if (!state.file)
        {
            io_failure();
        }
    }
}

void tx_log_write(const std::string& record)
{
    auto& state = sink();
    std::lock_guard lock(state.mutex);
    if (state.path.empty())
    {
        tx_fn_write_error(record);
        return;
    }
    if (record.size() > state.maximum)
    {
        throw runtime_failure({tx::error_kind::runtime, "log_size_limit",
            "单条日志超过文件大小上限"});
    }
    if (state.size > state.maximum - record.size())
    {
        rotate(state);
    }
    state.file.write(record.data(), static_cast<std::streamsize>(record.size()));
    state.file.flush();
    if (!state.file)
    {
        io_failure();
    }
    state.size += record.size();
}

void tx_log_set_secret_keys(std::vector<std::string> keys)
{
    auto& state = sink();
    std::lock_guard lock(state.mutex);
    state.secret_keys = std::move(keys);
}

std::vector<std::string> tx_log_secret_keys()
{
    auto& state = sink();
    std::lock_guard lock(state.mutex);
    return state.secret_keys;
}

} // namespace tx_generated
