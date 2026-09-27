#include "stdlib/encoding.hpp"
#include "stdlib/filesystem_extended.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/filesystem_watch.hpp"
#include "stdlib/error.hpp"

#include <windows.h>

#include <array>
#include <deque>
#include <mutex>
#include <string>

namespace tx_generated
{

struct watcher_mutex
{
    SRWLOCK native = SRWLOCK_INIT;

    void lock() noexcept
    {
        AcquireSRWLockExclusive(&native);
    }
    void unlock() noexcept
    {
        ReleaseSRWLockExclusive(&native);
    }
};

struct fs_watcher_state
{
    HANDLE directory = INVALID_HANDLE_VALUE;
    HANDLE event = nullptr;
    OVERLAPPED operation{};
    alignas(FILE_NOTIFY_INFORMATION) std::array<unsigned char, 64 * 1024> buffer{};
    std::deque<watch_event_value> queued;
    watcher_mutex mutex;
    CONDITION_VARIABLE no_waiters = CONDITION_VARIABLE_INIT;
    std::size_t waiters = 0;
    bool pending = false;
    bool closed = false;
    bool recursive = false;

    ~fs_watcher_state()
    {
        close();
    }

    void close() noexcept
    {
        std::unique_lock lock(mutex);
        if (closed)
        {
            return;
        }
        closed = true;
        if (pending)
        {
            CancelIoEx(directory, &operation);
        }
        // 锁和条件等待都经 Win32，避免静态/动态 pthread 的内部布局混用。
        while (waiters != 0)
        {
            SleepConditionVariableSRW(&no_waiters, &mutex.native, INFINITE, 0);
        }
        if (pending)
        {
            DWORD transferred = 0;
            GetOverlappedResult(directory, &operation, &transferred, TRUE);
            pending = false;
        }
        CloseHandle(event);
        CloseHandle(directory);
    }
};

namespace
{

[[noreturn]] void closed_error()
{
    throw runtime_failure({tx::error_kind::io, "closed_handle",
        "文件监视器已关闭"});
}

const char* action_kind(DWORD action)
{
    switch (action)
    {
    case FILE_ACTION_ADDED: return "created";
    case FILE_ACTION_REMOVED: return "deleted";
    case FILE_ACTION_MODIFIED: return "modified";
    case FILE_ACTION_RENAMED_OLD_NAME: return "renamed_old";
    case FILE_ACTION_RENAMED_NEW_NAME: return "renamed_new";
    default: return nullptr;
    }
}

void queue_events(fs_watcher_state& state, DWORD transferred)
{
    if (transferred == 0)
    {
        state.queued.push_back({"overflow", ""});
        return;
    }
    std::size_t offset = 0;
    while (offset < transferred)
    {
        if (transferred - offset < offsetof(FILE_NOTIFY_INFORMATION, FileName))
        {
            state.queued.push_back({"overflow", ""});
            return;
        }
        const auto* item = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(
            state.buffer.data() + offset);
        const auto name_bytes = static_cast<std::size_t>(item->FileNameLength);
        if ((name_bytes & 1) != 0 ||
            name_bytes > transferred - offset - offsetof(FILE_NOTIFY_INFORMATION, FileName))
        {
            state.queued.push_back({"overflow", ""});
            return;
        }
        if (const auto* kind = action_kind(item->Action))
        {
            const std::wstring name(item->FileName, name_bytes / sizeof(wchar_t));
            state.queued.push_back({kind,
                detail::path_text(std::filesystem::path(name))});
        }
        if (item->NextEntryOffset == 0)
        {
            break;
        }
        if (item->NextEntryOffset < offsetof(FILE_NOTIFY_INFORMATION, FileName) +
                name_bytes || item->NextEntryOffset > transferred - offset)
        {
            state.queued.push_back({"overflow", ""});
            return;
        }
        offset += item->NextEntryOffset;
    }
}

void start_read(fs_watcher_state& state, bool recursive)
{
    ResetEvent(state.event);
    state.operation = {};
    state.operation.hEvent = state.event;
    constexpr DWORD changes = FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |
        FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_CREATION;
    if (!ReadDirectoryChangesW(state.directory, state.buffer.data(),
            static_cast<DWORD>(state.buffer.size()), recursive, changes,
            nullptr, &state.operation, nullptr))
    {
        const auto error = GetLastError();
        if (error != ERROR_IO_PENDING)
        {
            detail::fail_windows("启动目录监视", "", error);
        }
    }
    state.pending = true;
}

watch_event_value finish_read(fs_watcher_state& state, bool recursive)
{
    DWORD transferred = 0;
    if (!GetOverlappedResult(state.directory, &state.operation, &transferred, FALSE))
    {
        const auto error = GetLastError();
        state.pending = false;
        if (error == ERROR_NOTIFY_ENUM_DIR || error == ERROR_MORE_DATA)
        {
            start_read(state, recursive);
            return {"overflow", ""};
        }
        detail::fail_windows("读取目录监视", "", error);
    }
    state.pending = false;
    queue_events(state, transferred);
    // 下一次系统读取必须先于向用户交付事件，以覆盖两次 TX 调用之间的变化。
    start_read(state, recursive);
    if (state.queued.empty())
    {
        return {"overflow", ""};
    }
    auto result = std::move(state.queued.front());
    state.queued.pop_front();
    return result;
}

} // namespace

fs_watcher fs_watch(const std::string& path, bool recursive)
{
    if (fs_stat(path, true).kind != "directory")
    {
        throw runtime_failure({tx::error_kind::io, "invalid_argument",
            "只能监视目录：" + path});
    }
    const auto native_path = detail::checked_path(path);
    const HANDLE directory = CreateFileW(native_path.c_str(),
        FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);
    if (directory == INVALID_HANDLE_VALUE)
    {
        detail::fail_windows("打开目录监视", path, GetLastError());
    }
    const HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!event)
    {
        const auto error = GetLastError();
        CloseHandle(directory);
        detail::fail_windows("创建目录监视事件", path, error);
    }
    fs_watcher state;
    try
    {
        state = std::make_shared<fs_watcher_state>();
    }
    catch (...)
    {
        CloseHandle(event);
        CloseHandle(directory);
        throw;
    }
    state->directory = directory;
    state->event = event;
    state->operation.hEvent = event;
    state->recursive = recursive;
    start_read(*state, recursive);
    return state;
}

watch_event_value fs_watch_next(const fs_watcher& watcher,
                                std::int64_t timeout_millis)
{
    if (!watcher)
    {
        closed_error();
    }
    if (timeout_millis < -1 || timeout_millis > 0xfffffffeLL)
    {
        throw runtime_failure({tx::error_kind::io, "invalid_argument",
            "监视超时必须为 -1 或 0～4294967294 毫秒"});
    }
    std::unique_lock lock(watcher->mutex);
    if (watcher->closed)
    {
        closed_error();
    }
    if (watcher->waiters != 0)
    {
        throw runtime_failure({tx::error_kind::io, "invalid_argument",
            "同一监视器不能同时等待多个事件"});
    }
    if (!watcher->queued.empty())
    {
        auto result = std::move(watcher->queued.front());
        watcher->queued.pop_front();
        return result;
    }
    if (!watcher->pending)
    {
        start_read(*watcher, watcher->recursive);
    }
    ++watcher->waiters;
    lock.unlock();
    const auto wait_result = WaitForSingleObject(watcher->event,
        timeout_millis < 0 ? INFINITE : static_cast<DWORD>(timeout_millis));
    const auto wait_error = wait_result == WAIT_FAILED ? GetLastError() : 0;
    lock.lock();
    --watcher->waiters;
    WakeAllConditionVariable(&watcher->no_waiters);
    if (watcher->closed)
    {
        closed_error();
    }
    if (wait_result == WAIT_TIMEOUT)
    {
        return {"timeout", ""};
    }
    if (wait_result != WAIT_OBJECT_0)
    {
        detail::fail_windows("等待目录监视", "", wait_error);
    }
    return finish_read(*watcher, watcher->recursive);
}

void fs_close_watch(const fs_watcher& watcher)
{
    if (watcher)
    {
        watcher->close();
    }
}

} // namespace tx_generated
