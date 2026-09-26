#include "stdlib/file_stream.hpp"

#include "stdlib/error.hpp"

#include <filesystem>
#include <limits>
#include <stdexcept>
#include <utility>

namespace tx_generated
{
namespace
{

[[noreturn]] void text_error(const char* code, const std::string& message)
{
    throw runtime_failure({tx::error_kind::io, code, message});
}

std::string read_exact(stream_file& file, std::size_t count)
{
    std::string result;
    while (result.size() < count)
    {
        const auto part = file.read(count - result.size());
        if (part.empty())
        {
            text_error("invalid_encoding", "文本流在字符中间结束");
        }
        result += part;
    }
    return result;
}

std::string decode_scalar(text_stream_state& source)
{
    const auto first = source.file.read(1);
    if (first.empty())
    {
        return {};
    }
    const auto value = static_cast<unsigned char>(first[0]);
    const auto encoding = source.encoding;
    if (encoding == detail::text_encoding::utf8 ||
        encoding == detail::text_encoding::utf8_sig)
    {
        const std::size_t count = value < 0x80 ? 1 :
            value >= 0xc2 && value <= 0xdf ? 2 :
            value >= 0xe0 && value <= 0xef ? 3 :
            value >= 0xf0 && value <= 0xf4 ? 4 : 0;
        if (count == 0)
        {
            text_error("invalid_encoding", "文本流包含无效 UTF-8 字节");
        }
        const auto bytes = first + (count > 1 ? read_exact(source.file, count - 1) : "");
        return detail::decode_text(bytes, detail::text_encoding::utf8);
    }
    if (encoding == detail::text_encoding::utf16le ||
        encoding == detail::text_encoding::utf16be)
    {
        const auto bytes = first + read_exact(source.file, 1);
        const auto first_byte = static_cast<unsigned char>(bytes[0]);
        const auto second_byte = static_cast<unsigned char>(bytes[1]);
        const unsigned unit = encoding == detail::text_encoding::utf16le
            ? first_byte | (second_byte << 8) : second_byte | (first_byte << 8);
        std::wstring wide(1, static_cast<wchar_t>(unit));
        if (unit >= 0xd800 && unit <= 0xdbff)
        {
            const auto tail = read_exact(source.file, 2);
            const auto low = static_cast<unsigned char>(tail[0]);
            const auto high = static_cast<unsigned char>(tail[1]);
            const unsigned follower = encoding == detail::text_encoding::utf16le
                ? low | (high << 8) : high | (low << 8);
            if (follower < 0xdc00 || follower > 0xdfff)
            {
                text_error("invalid_encoding", "UTF-16 高代理项后缺少低代理项");
            }
            wide.push_back(static_cast<wchar_t>(follower));
        }
        else if (unit >= 0xdc00 && unit <= 0xdfff)
        {
            text_error("invalid_encoding", "UTF-16 存在孤立低代理项");
        }
        return detail::wide_to_utf8(wide);
    }
    if (value < 0x80)
    {
        return first;
    }
    if (value < 0x81 || value > 0xfe)
    {
        text_error("invalid_encoding", "文本流包含无效字符集字节");
    }
    auto bytes = first + read_exact(source.file, 1);
    const auto second = static_cast<unsigned char>(bytes[1]);
    if (encoding == detail::text_encoding::gb18030 &&
        second >= 0x30 && second <= 0x39)
    {
        bytes += read_exact(source.file, 2);
        const auto third = static_cast<unsigned char>(bytes[2]);
        const auto fourth = static_cast<unsigned char>(bytes[3]);
        if (third < 0x81 || third > 0xfe || fourth < 0x30 || fourth > 0x39)
        {
            text_error("invalid_encoding", "GB18030 四字节序列无效");
        }
    }
    else if (second < 0x40 || second == 0x7f || second > 0xfe)
    {
        text_error("invalid_encoding", "GBK/GB18030 双字节序列无效");
    }
    return detail::decode_text(bytes, encoding);
}

std::optional<std::string> next_scalar(text_stream_state& source)
{
    source.file.require_open();
    if (source.pending)
    {
        auto value = std::move(*source.pending);
        source.pending.reset();
        return value;
    }
    try
    {
        auto value = decode_scalar(source);
        if (value.empty())
        {
            return std::nullopt;
        }
        return value;
    }
    catch (const runtime_failure&)
    {
        throw;
    }
    catch (const std::runtime_error& error)
    {
        text_error("invalid_encoding", error.what());
    }
}

void configure_reader(text_stream_state& source)
{
    const auto selected = source.encoding;
    if (selected == detail::text_encoding::utf8_sig)
    {
        const auto prefix = source.file.read(3);
        (void)source.file.seek(prefix == "\xef\xbb\xbf" ? 3 : 0, "start");
        source.encoding = detail::text_encoding::utf8;
    }
    else if (selected == detail::text_encoding::utf16 ||
             selected == detail::text_encoding::utf16le ||
             selected == detail::text_encoding::utf16be)
    {
        const auto prefix = source.file.read(2);
        const bool little = prefix == "\xff\xfe";
        const bool big = prefix == "\xfe\xff";
        if (selected == detail::text_encoding::utf16 && !little && !big)
        {
            text_error("invalid_encoding", "UTF-16 文本流缺少 BOM");
        }
        if ((selected == detail::text_encoding::utf16le && big) ||
            (selected == detail::text_encoding::utf16be && little))
        {
            text_error("invalid_encoding", "UTF-16 BOM 与指定字节序不一致");
        }
        source.encoding = selected == detail::text_encoding::utf16
            ? little ? detail::text_encoding::utf16le
                     : detail::text_encoding::utf16be : selected;
        (void)source.file.seek(little || big ? 2 : 0, "start");
    }
}

void configure_writer(text_stream_state& source, std::string_view path)
{
    bool empty = source.file.mode() == stream_mode::write;
    if (source.file.mode() == stream_mode::append)
    {
        const auto file_path = detail::path_from_utf8(path);
        std::error_code error;
        const auto size = std::filesystem::file_size(file_path, error);
        if (error)
        {
            text_error("operation_failed", "无法检查追加文件大小");
        }
        empty = size == 0;
        if (!empty && (source.encoding == detail::text_encoding::utf16 ||
                       source.encoding == detail::text_encoding::utf16le ||
                       source.encoding == detail::text_encoding::utf16be ||
                       source.encoding == detail::text_encoding::utf8_sig))
        {
            stream_file header(path, stream_mode::read);
            const auto prefix = header.read(source.encoding ==
                detail::text_encoding::utf8_sig ? 3 : 2);
            const bool little = prefix.starts_with("\xff\xfe");
            const bool big = prefix.starts_with("\xfe\xff");
            if (source.encoding == detail::text_encoding::utf8_sig &&
                prefix != "\xef\xbb\xbf")
            {
                text_error("invalid_encoding", "追加 UTF-8-SIG 文件需要已有 BOM");
            }
            if ((source.encoding == detail::text_encoding::utf16 &&
                 !little && !big) ||
                (source.encoding == detail::text_encoding::utf16le && big) ||
                (source.encoding == detail::text_encoding::utf16be && little))
            {
                text_error("invalid_encoding", "追加文件的 UTF-16 BOM 不匹配");
            }
            if (source.encoding == detail::text_encoding::utf16)
            {
                source.encoding = little ? detail::text_encoding::utf16le
                                         : detail::text_encoding::utf16be;
            }
        }
    }
    if (empty && (source.encoding == detail::text_encoding::utf8_sig ||
                  source.encoding == detail::text_encoding::utf16))
    {
        source.file.write(detail::encode_text("", source.encoding));
    }
}

} // namespace

text_stream_state::text_stream_state(std::string_view path, stream_mode mode,
                                     detail::text_encoding selected)
    : file(path, mode), encoding(selected)
{
    if (mode == stream_mode::read)
    {
        configure_reader(*this);
    }
    else
    {
        configure_writer(*this, path);
    }
}

text_stream open_text_stream(std::string_view path, std::string_view mode,
                             std::string_view encoding)
{
    detail::text_encoding selected;
    try
    {
        selected = detail::parse_encoding(encoding);
    }
    catch (const std::runtime_error& error)
    {
        text_error("invalid_encoding", error.what());
    }
    if (selected == detail::text_encoding::utf32 ||
        selected == detail::text_encoding::utf32le ||
        selected == detail::text_encoding::utf32be)
    {
        text_error("invalid_encoding", "UTF-32 文本流请使用二进制流和 encoding 增量解码器");
    }
    const auto selected_mode = mode == "read" ? stream_mode::read
        : mode == "write" ? stream_mode::write
        : mode == "append" ? stream_mode::append
        : stream_mode::update;
    if (selected_mode == stream_mode::update)
    {
        text_error("invalid_mode", "文本流模式只能是 read、write 或 append");
    }
    return std::make_shared<text_stream_state>(path, selected_mode, selected);
}

text_chunk_value stream_read_chars(const text_stream& source, std::int64_t count)
{
    source->file.require_open();
    if (count < 1 || count > 1048576)
    {
        text_error("size_limit", "单次读取字符数必须在 1 到 1048576 之间");
    }
    text_chunk_value result;
    for (std::int64_t index = 0; index < count; ++index)
    {
        const auto scalar = next_scalar(*source);
        if (!scalar)
        {
            result.eof = result.text.empty();
            break;
        }
        result.text += *scalar;
    }
    return result;
}

line_result_value stream_read_line(const text_stream& source,
                                    std::int64_t max_bytes)
{
    source->file.require_open();
    if (max_bytes < 1 || max_bytes > 8 * 1024 * 1024)
    {
        text_error("size_limit", "单行上限必须在 1 到 8 MiB 之间");
    }
    line_result_value result;
    while (const auto scalar = next_scalar(*source))
    {
        result.has_line = true;
        if (*scalar == "\n")
        {
            return result;
        }
        if (*scalar == "\r")
        {
            auto following = next_scalar(*source);
            if (following && *following != "\n")
            {
                source->pending = std::move(*following);
            }
            return result;
        }
        if (scalar->size() > static_cast<std::size_t>(max_bytes) -
                             result.line.size())
        {
            try
            {
                source->file.close();
            }
            catch (const runtime_failure&)
            {
                // 行长错误优先报告；关闭时仍已把句柄标记为失效。
            }
            text_error("size_limit", "文本行超过允许的 UTF-8 字节数");
        }
        result.line += *scalar;
    }
    return result;
}

void stream_write_text(const text_stream& target, std::string_view text)
{
    target->file.require_open();
    try
    {
        target->file.write(detail::encode_text(text, target->encoding, false));
    }
    catch (const runtime_failure&)
    {
        throw;
    }
    catch (const std::runtime_error& error)
    {
        text_error("invalid_encoding", error.what());
    }
}

void stream_write_line(const text_stream& target, std::string_view text)
{
    std::string line(text);
    line.push_back('\n');
    stream_write_text(target, line);
}

void stream_flush(const text_stream& target)
{
    target->file.flush();
}

void stream_close(const text_stream& target)
{
    target->file.close();
}

} // namespace tx_generated
