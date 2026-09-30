#include "stdlib/crypto_file_io.hpp"
#include "stdlib/crypto_internal.hpp"
#include "stdlib/filesystem_extended.hpp"

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

namespace tx_generated::crypto
{
namespace
{

[[noreturn]] void io_error(const char* operation)
{
    detail::fail_filesystem(operation, "", {errno, std::generic_category()});
}

void sync_file(int file)
{
    int status;
    do
    {
        status = fsync(file);
    } while (status < 0 && errno == EINTR);
    if (status < 0)
    {
        io_error("持久化文件");
    }
}

} // namespace

file_input::file_input(const std::filesystem::path& path)
{
    handle_ = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (handle_ < 0)
    {
        io_error("打开加密输入文件");
    }
}

file_input::~file_input() noexcept
{
    if (handle_ >= 0)
    {
        close(handle_);
    }
}

std::size_t file_input::read(std::span<std::uint8_t> output)
{
    ssize_t count;
    do
    {
        count = ::read(handle_, output.data(), std::min<std::size_t>(output.size(), 8 * 1024 * 1024));
    } while (count < 0 && errno == EINTR);
    if (count < 0)
    {
        io_error("读取加密输入文件");
    }
    return static_cast<std::size_t>(count);
}

void file_input::read_exact(std::span<std::uint8_t> output)
{
    while (!output.empty())
    {
        const auto count = read(output);
        if (!count)
        {
            fail("invalid_format", "加密文件被截断");
        }
        output = output.subspan(count);
    }
}

file_output::file_output(const std::filesystem::path& destination)
{
    if (destination.filename().empty())
    {
        fail("invalid_argument", "目标路径缺少文件名");
    }
    auto pattern = destination.string() + ".tx-tmp-XXXXXX";
    handle_ = mkostemp(pattern.data(), O_CLOEXEC);
    if (handle_ < 0)
    {
        io_error("创建同目录临时文件");
    }
    path_ = pattern;
}

file_output::~file_output() noexcept
{
    if (handle_ >= 0)
    {
        close(handle_);
    }
    if (remove_on_exit_)
    {
        unlink(path_.c_str());
    }
}

void file_output::write(std::span<const std::uint8_t> data)
{
    while (!data.empty())
    {
        const auto count = ::write(handle_, data.data(), std::min<std::size_t>(data.size(), 8 * 1024 * 1024));
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count <= 0)
        {
            io_error("写入临时文件");
        }
        data = data.subspan(static_cast<std::size_t>(count));
    }
}

void file_output::commit(const std::filesystem::path& destination)
{
    sync_file(handle_);
    const auto closing = handle_;
    handle_ = -1;
    if (close(closing) != 0)
    {
        io_error("关闭临时文件");
    }
    if (rename(path_.c_str(), destination.c_str()) != 0)
    {
        io_error("原子替换目标文件");
    }
    remove_on_exit_ = false;
    const auto parent = destination.parent_path().empty() ? std::filesystem::path(".") : destination.parent_path();
    const int directory = open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory < 0)
    {
        io_error("打开目标目录");
    }
    struct directory_owner
    {
        int value;
        ~directory_owner()
        {
            close(value);
        }
    } owner{directory};
    sync_file(directory);
}

void require_distinct_paths(const std::filesystem::path& source,
    const std::filesystem::path& destination)
{
    std::error_code error;
    if (source.lexically_normal() == destination.lexically_normal() ||
        std::filesystem::equivalent(source, destination, error))
    {
        fail("invalid_argument", "加密文件的源路径和目标路径必须不同");
    }
}

} // namespace tx_generated::crypto
