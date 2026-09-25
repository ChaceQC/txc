#include "stdlib/file_stream.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace tx_generated
{
namespace
{

[[noreturn]] void io_failure(const char* code, const std::string& message)
{
    throw runtime_failure({tx::error_kind::io, code, message});
}

const char* system_error_code(int value)
{
    if (value == ENOENT)
    {
        return "not_found";
    }
    if (value == EACCES || value == EPERM)
    {
        return "permission_denied";
    }
    return "operation_failed";
}

stream_mode parse_mode(std::string_view value, bool text)
{
    if (value == "read")
    {
        return stream_mode::read;
    }
    if (value == "write")
    {
        return stream_mode::write;
    }
    if (value == "append")
    {
        return stream_mode::append;
    }
    if (!text && value == "update")
    {
        return stream_mode::update;
    }
    io_failure("invalid_mode", "文件流模式无效");
}

std::filesystem::path stream_path(std::string_view value)
{
    try
    {
        return detail::path_from_utf8(value);
    }
    catch (const std::runtime_error& error)
    {
        io_failure("invalid_path", error.what());
    }
}

} // namespace

stream_file::stream_file(std::string_view path, stream_mode mode)
    : mode_(mode)
{
    const auto native_path = stream_path(path);
    const wchar_t* native_mode = mode == stream_mode::read ? L"rb"
        : mode == stream_mode::write ? L"wb"
        : mode == stream_mode::append ? L"ab" : L"r+b";
    const int status = _wfopen_s(&file_, native_path.c_str(), native_mode);
    if (status != 0 || !file_)
    {
        io_failure(system_error_code(status), "无法打开文件流：" +
            std::string(path));
    }
    if (mode == stream_mode::append && _fseeki64(file_, 0, SEEK_END) != 0)
    {
        const int error = errno;
        std::fclose(file_);
        file_ = nullptr;
        io_failure(system_error_code(error), "无法定位到文件末尾");
    }
}

stream_file::~stream_file() noexcept
{
    if (file_)
    {
        std::fclose(file_);
    }
}

stream_mode stream_file::mode() const noexcept
{
    return mode_;
}

void stream_file::require_open() const
{
    if (!file_)
    {
        io_failure("closed_stream", "文件流已关闭");
    }
}

std::string stream_file::read(std::size_t size)
{
    require_open();
    if (mode_ != stream_mode::read && mode_ != stream_mode::update)
    {
        io_failure("invalid_mode", "当前文件流不可读取");
    }
    if (mode_ == stream_mode::update && last_ == direction::write)
    {
        io_failure("invalid_seek", "切换读写方向前需要 seek 或 flush");
    }
    std::string result(size, '\0');
    const auto count = std::fread(result.data(), 1, size, file_);
    if (std::ferror(file_))
    {
        io_failure("operation_failed", "读取文件流失败");
    }
    result.resize(count);
    last_ = direction::read;
    return result;
}

void stream_file::write(std::string_view data)
{
    require_open();
    if (mode_ == stream_mode::read)
    {
        io_failure("invalid_mode", "当前文件流不可写入");
    }
    if (mode_ == stream_mode::update && last_ == direction::read)
    {
        io_failure("invalid_seek", "切换读写方向前需要 seek 或 flush");
    }
    std::size_t offset = 0;
    while (offset < data.size())
    {
        const auto count = std::fwrite(data.data() + offset, 1,
                                       data.size() - offset, file_);
        if (count == 0)
        {
            io_failure("operation_failed", "写入文件流失败，文件可能已部分写入");
        }
        offset += count;
    }
    last_ = direction::write;
}

std::int64_t stream_file::tell()
{
    require_open();
    const auto position = _ftelli64(file_);
    if (position < 0)
    {
        io_failure("invalid_seek", "无法取得文件流位置");
    }
    return position;
}

std::int64_t stream_file::seek(std::int64_t offset, std::string_view origin)
{
    require_open();
    if (mode_ == stream_mode::append)
    {
        io_failure("invalid_seek", "追加文件流不可定位");
    }
    std::int64_t base = 0;
    if (origin == "current")
    {
        base = tell();
    }
    else if (origin == "end")
    {
        if (_fseeki64(file_, 0, SEEK_END) != 0)
        {
            io_failure("invalid_seek", "无法定位到文件末尾");
        }
        base = tell();
    }
    else if (origin != "start")
    {
        io_failure("invalid_seek", "定位基准只能是 start、current 或 end");
    }
    if (offset < -base || (offset > 0 &&
        base > std::numeric_limits<std::int64_t>::max() - offset))
    {
        io_failure("invalid_seek", "文件流位置超出 int 范围");
    }
    const auto position = base + offset;
    if (_fseeki64(file_, position, SEEK_SET) != 0)
    {
        io_failure("invalid_seek", "文件流定位失败");
    }
    last_ = direction::none;
    return position;
}

void stream_file::flush()
{
    require_open();
    const int status = last_ == direction::write ? std::fflush(file_)
        : _fseeki64(file_, 0, SEEK_CUR);
    if (status != 0)
    {
        io_failure("operation_failed", "刷新文件流失败");
    }
    last_ = direction::none;
}

void stream_file::close()
{
    if (!file_)
    {
        return;
    }
    auto* closing = file_;
    file_ = nullptr;
    if (std::fclose(closing) != 0)
    {
        io_failure("operation_failed", "关闭文件流失败，写入可能未完成");
    }
}

binary_stream_state::binary_stream_state(std::string_view path, stream_mode mode)
    : file(path, mode)
{
}

binary_stream open_binary_stream(std::string_view path, std::string_view mode)
{
    return std::make_shared<binary_stream_state>(path, parse_mode(mode, false));
}

byte_chunk_value stream_read_bytes(const binary_stream& source, std::int64_t size)
{
    source->file.require_open();
    if (size < 1 || size > 8 * 1024 * 1024)
    {
        io_failure("size_limit", "单次读取大小必须在 1 到 8 MiB 之间");
    }
    const auto data = source->file.read(static_cast<std::size_t>(size));
    return {make_bytes({data.begin(), data.end()}), data.empty()};
}

byte_value stream_read_all_bytes(const binary_stream& source)
{
    std::vector<std::uint8_t> result;
    constexpr std::size_t block_size = 8 * 1024 * 1024;
    while (true)
    {
        const auto block = source->file.read(block_size);
        if (block.empty())
        {
            break;
        }
        if (block.size() > static_cast<std::size_t>(
                std::numeric_limits<std::int64_t>::max()) - result.size())
        {
            io_failure("size_limit", "文件剩余内容超出 int 范围");
        }
        result.insert(result.end(), block.begin(), block.end());
    }
    return make_bytes(std::move(result));
}

void stream_write_bytes(const binary_stream& target, const byte_value& data)
{
    target->file.write({reinterpret_cast<const char*>(data->data()), data->size()});
}

std::int64_t stream_tell(const binary_stream& source)
{
    return source->file.tell();
}

std::int64_t stream_seek(const binary_stream& source, std::int64_t offset,
                         std::string_view origin)
{
    return source->file.seek(offset, origin);
}

void stream_flush(const binary_stream& target)
{
    target->file.flush();
}

void stream_close(const binary_stream& target)
{
    target->file.close();
}

} // namespace tx_generated
