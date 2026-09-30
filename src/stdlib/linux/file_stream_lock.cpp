#include "stdlib/file_stream.hpp"
#include "stdlib/error.hpp"

#include <cerrno>
#include <sys/file.h>
#include <unistd.h>

namespace tx_generated
{
namespace
{

[[noreturn]] void lock_error(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::io, code, message});
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
    int status;
    do
    {
        status = fsync(fileno(file_));
    } while (status < 0 && errno == EINTR);
    if (status < 0)
    {
        lock_error("operation_failed", "持久化文件流失败");
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
    if ((mode == "shared" && (mode_ == stream_mode::write || mode_ == stream_mode::append)) ||
        (mode == "exclusive" && mode_ == stream_mode::read))
    {
        lock_error("invalid_mode", "文件流模式不允许请求的文件锁");
    }
    const int flags = (mode == "shared" ? LOCK_SH : LOCK_EX) | (wait ? 0 : LOCK_NB);
    int status;
    do
    {
        status = flock(fileno(file_), flags);
    } while (status < 0 && errno == EINTR);
    if (status < 0)
    {
        if (!wait && (errno == EWOULDBLOCK || errno == EAGAIN))
        {
            return false;
        }
        lock_error("operation_failed", "获取文件锁失败");
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
    if (flock(fileno(file_), LOCK_UN) != 0)
    {
        lock_error("operation_failed", "释放文件锁失败");
    }
    locked_ = false;
}

} // namespace tx_generated
