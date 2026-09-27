#include "stdlib/file_stream.hpp"

#include "stdlib/error.hpp"

#include <cstdint>
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace tx_generated
{
namespace
{

[[noreturn]] void lock_error(const char* code, const std::string& message)
{
    throw runtime_failure({tx::error_kind::io, code, message});
}

HANDLE native_handle(std::FILE* file)
{
    const auto descriptor = _fileno(file);
    if (descriptor < 0)
    {
        lock_error("operation_failed", "无法取得文件流描述符");
    }
    const auto value = _get_osfhandle(descriptor);
    if (value == -1)
    {
        lock_error("operation_failed", "无法取得文件流系统句柄");
    }
    return reinterpret_cast<HANDLE>(value);
}

} // namespace

void stream_file::sync()
{
    require_open();
    if (mode_ == stream_mode::read)
    {
        lock_error("invalid_mode", "只读文件流不能持久化写入");
    }
    flush();
    if (!FlushFileBuffers(native_handle(file_)))
    {
        lock_error("operation_failed", "持久化文件流失败（系统错误 " +
            std::to_string(GetLastError()) + "）");
    }
}

bool stream_file::lock(std::string_view mode, bool wait)
{
    require_open();
    if (locked_)
    {
        lock_error("invalid_state", "文件流已经持有一把锁");
    }
    if (mode != "shared" && mode != "exclusive")
    {
        lock_error("invalid_mode", "文件锁模式只能是 shared 或 exclusive");
    }
    if ((mode == "shared" && mode_ == stream_mode::write) ||
        (mode == "shared" && mode_ == stream_mode::append) ||
        (mode == "exclusive" && mode_ == stream_mode::read))
    {
        lock_error("invalid_mode", "文件流模式不允许请求的文件锁");
    }
    OVERLAPPED region{};
    DWORD flags = mode == "exclusive" ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    if (!wait)
    {
        flags |= LOCKFILE_FAIL_IMMEDIATELY;
    }
    if (!LockFileEx(native_handle(file_), flags, 0, MAXDWORD, MAXDWORD, &region))
    {
        const auto error = GetLastError();
        if (!wait && (error == ERROR_LOCK_VIOLATION ||
                      error == ERROR_IO_PENDING))
        {
            return false;
        }
        lock_error("operation_failed", "获取文件锁失败（系统错误 " +
            std::to_string(error) + "）");
    }
    locked_ = true;
    return true;
}

void stream_file::unlock()
{
    require_open();
    if (!locked_)
    {
        lock_error("invalid_state", "文件流没有持有锁");
    }
    OVERLAPPED region{};
    if (!UnlockFileEx(native_handle(file_), 0, MAXDWORD, MAXDWORD, &region))
    {
        lock_error("operation_failed", "释放文件锁失败（系统错误 " +
            std::to_string(GetLastError()) + "）");
    }
    locked_ = false;
}

} // namespace tx_generated
