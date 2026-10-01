#include "stdlib/filesystem_watch.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/filesystem_extended.hpp"
#include "stdlib/error.hpp"
#include "stdlib/process_internal.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <map>
#include <mutex>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>

namespace tx_generated
{

struct fs_watcher_state
{
    process_detail::native_handle descriptor;
    process_detail::native_handle wake;
    std::filesystem::path root;
    std::map<int, std::filesystem::path> directories;
    std::deque<watch_event_value> queued;
    std::mutex mutex;
    std::condition_variable no_waiters;
    bool recursive = false;
    bool waiting = false;
    bool closed = false;
};

namespace
{

[[noreturn]] void watch_error(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::io, code, message});
}

void add_directory(fs_watcher_state& state, const std::filesystem::path& path)
{
    constexpr std::uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_ATTRIB |
        IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF | IN_ONLYDIR | IN_DONT_FOLLOW;
    const int watch = ::inotify_add_watch(state.descriptor.get(), path.c_str(), mask);
    if (watch < 0)
    {
        watch_error("operation_failed", "添加目录监视失败");
    }
    state.directories.insert_or_assign(watch, path.lexically_relative(state.root));
}

void add_tree(fs_watcher_state& state, const std::filesystem::path& path)
{
    add_directory(state, path);
    if (!state.recursive)
    {
        return;
    }
    std::error_code error;
    std::filesystem::recursive_directory_iterator iterator(path, error), end;
    while (!error && iterator != end)
    {
        if (iterator->is_directory(error) && !iterator->is_symlink(error))
        {
            add_directory(state, iterator->path());
        }
        iterator.increment(error);
    }
    if (error)
    {
        watch_error("operation_failed", "扫描递归监视目录失败");
    }
}

void remove_tree(fs_watcher_state& state, const std::filesystem::path& relative)
{
    const auto prefix = relative.generic_string() + '/';
    for (auto iterator = state.directories.begin(); iterator != state.directories.end();)
    {
        const auto text = iterator->second.generic_string();
        if (iterator->second == relative || text.starts_with(prefix))
        {
            ::inotify_rm_watch(state.descriptor.get(), iterator->first);
            iterator = state.directories.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }
}

void queue_event(fs_watcher_state& state, const inotify_event& item)
{
    if (item.mask & IN_Q_OVERFLOW)
    {
        state.queued.push_back({"overflow", ""});
        return;
    }
    const auto found = state.directories.find(item.wd);
    if (found == state.directories.end())
    {
        return;
    }
    const auto relative = item.len ? found->second / item.name : found->second;
    if (item.mask & IN_IGNORED)
    {
        state.directories.erase(found);
        return;
    }
    if (item.mask & (IN_DELETE_SELF | IN_MOVE_SELF))
    {
        state.queued.push_back({"overflow", ""});
    }
    const char* kind = item.mask & IN_MOVED_FROM ? "renamed_old"
        : item.mask & IN_MOVED_TO ? "renamed_new"
        : item.mask & IN_CREATE ? "created"
        : item.mask & IN_DELETE ? "deleted"
        : item.mask & (IN_MODIFY | IN_ATTRIB) ? "modified" : nullptr;
    if (kind && item.len)
    {
        state.queued.push_back({kind, relative.lexically_normal().generic_string()});
    }
    if (state.recursive && (item.mask & IN_ISDIR))
    {
        if (item.mask & (IN_MOVED_FROM | IN_DELETE))
        {
            remove_tree(state, relative);
        }
        if (item.mask & (IN_CREATE | IN_MOVED_TO))
        {
            try
            {
                add_tree(state, state.root / relative);
            }
            catch (const runtime_failure&)
            {
                state.queued.push_back({"overflow", ""});
            }
            // 新子树安装监视之前存在窗口，明确要求重新扫描而不伪造完整事件流。
            state.queued.push_back({"overflow", ""});
        }
    }
}

void read_events(fs_watcher_state& state)
{
    alignas(inotify_event) std::array<char, 65536> buffer;
    const auto size = ::read(state.descriptor.get(), buffer.data(), buffer.size());
    if (size < 0)
    {
        if (errno == EAGAIN || errno == EINTR)
        {
            return;
        }
        watch_error("operation_failed", "读取目录监视失败");
    }
    for (std::size_t offset = 0; offset + sizeof(inotify_event) <= static_cast<std::size_t>(size);)
    {
        const auto& item = *reinterpret_cast<const inotify_event*>(buffer.data() + offset);
        if (offset + sizeof(item) + item.len > static_cast<std::size_t>(size))
        {
            state.queued.push_back({"overflow", ""});
            break;
        }
        queue_event(state, item);
        offset += sizeof(item) + item.len;
    }
}

} // namespace

fs_watcher fs_watch(const std::string& path, bool recursive)
{
    auto state = std::make_shared<fs_watcher_state>();
    const auto checked = detail::checked_path(path);
    std::error_code error;
    state->root = std::filesystem::canonical(checked, error);
    if (error || !std::filesystem::is_directory(state->root, error))
    {
        watch_error("invalid_argument", "只能监视存在的目录");
    }
    state->descriptor.reset(::inotify_init1(IN_NONBLOCK | IN_CLOEXEC));
    state->wake.reset(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));
    if (!state->descriptor.valid() || !state->wake.valid())
    {
        watch_error("operation_failed", "创建目录监视失败");
    }
    state->recursive = recursive;
    add_tree(*state, state->root);
    return state;
}

watch_event_value fs_watch_next(const fs_watcher& watcher, std::int64_t timeout_millis)
{
    if (!watcher)
    {
        watch_error("closed_handle", "文件监视器已关闭");
    }
    if (timeout_millis < -1 || timeout_millis > 0xfffffffeLL)
    {
        watch_error("invalid_argument", "监视超时必须为 -1 或 0～4294967294 毫秒");
    }
    const auto limit = process_detail::deadline(timeout_millis);
    std::unique_lock lock(watcher->mutex);
    if (watcher->closed)
    {
        watch_error("closed_handle", "文件监视器已关闭");
    }
    if (watcher->waiting)
    {
        watch_error("invalid_argument", "同一监视器不能同时等待多个事件");
    }
    for (;;)
    {
        if (watcher->closed)
        {
            watch_error("closed_handle", "文件监视器已关闭");
        }
        if (!watcher->queued.empty())
        {
            auto result = std::move(watcher->queued.front());
            watcher->queued.pop_front();
            return result;
        }
        std::array<pollfd, 2> items{{{watcher->descriptor.get(), POLLIN, 0}, {watcher->wake.get(), POLLIN, 0}}};
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(limit - process_detail::steady_clock::now()).count();
        const int timeout = timeout_millis < 0 ? -1 : static_cast<int>(std::clamp<std::int64_t>(remaining, 0, 1000));
        watcher->waiting = true;
        lock.unlock();
        const int count = ::poll(items.data(), items.size(), timeout);
        const int error = errno;
        lock.lock();
        watcher->waiting = false;
        watcher->no_waiters.notify_all();
        if (watcher->closed)
        {
            watch_error("closed_handle", "文件监视器已关闭");
        }
        if (count > 0)
        {
            read_events(*watcher);
        }
        else if (count < 0 && error != EINTR)
        {
            watch_error("operation_failed", "等待目录监视失败");
        }
        if (watcher->queued.empty() && process_detail::steady_clock::now() >= limit)
        {
            return {"timeout", ""};
        }
    }
}

void fs_close_watch(const fs_watcher& watcher)
{
    if (watcher)
    {
        std::unique_lock lock(watcher->mutex);
        if (watcher->closed)
        {
            return;
        }
        watcher->closed = true;
        const std::uint64_t signal = 1;
        (void)::write(watcher->wake.get(), &signal, sizeof(signal));
        // 先唤醒并等待 poll 退出，再关闭 fd，避免描述符复用到其他等待请求。
        watcher->no_waiters.wait(lock, [&]
        {
            return !watcher->waiting;
        });
        watcher->descriptor.reset();
        watcher->wake.reset();
    }
}

} // namespace tx_generated
