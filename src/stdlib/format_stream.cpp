#include "stdlib/format_stream.hpp"
#include "stdlib/error.hpp"

#include <utility>
#include <algorithm>

namespace tx_generated
{

namespace
{

template<class stream_type>
std::function<void()> stream_guard(const stream_type& stream)
{
    if (!stream)
    {
        throw runtime_failure({tx::error_kind::io, "closed_stream", "文件流未打开或已关闭"});
    }
    stream->file.require_open();
    return [stream]
    {
        stream->file.require_open();
    };
}

} // namespace

std::function<void()> format_stream_guard(const binary_stream& stream)
{
    return stream_guard(stream);
}

std::function<void()> format_stream_guard(const text_stream& stream)
{
    return stream_guard(stream);
}

format_source format_read_source(const binary_stream& source)
{
    (void)format_stream_guard(source);
    return [source]
    {
        return source->file.read(4096);
    };
}

format_source format_read_source(const text_stream& source)
{
    (void)format_stream_guard(source);
    return [source]
    {
        return stream_read_chars(source, 1024).text;
    };
}

format_sink format_write_sink(const binary_stream& target)
{
    (void)format_stream_guard(target);
    return [target](std::string_view text)
    {
        target->file.write(text);
    };
}

format_sink format_write_sink(const text_stream& target)
{
    (void)format_stream_guard(target);
    return [target](std::string_view text)
    {
        stream_write_text(target, text);
    };
}

format_input::format_input(std::string_view text, std::string format)
    : memory_(text), format_(std::move(format)), eof_(true)
{
}

format_input::format_input(format_source source, std::string format,
                           std::uint64_t limit)
    : source_(std::move(source)), format_(std::move(format)), limit_(limit)
{
}

int format_input::peek(std::size_t ahead)
{
    if (!source_)
    {
        return cursor_ + ahead < memory_.size()
            ? static_cast<unsigned char>(memory_[cursor_ + ahead]) : -1;
    }
    while (cursor_ + ahead >= buffer_.size() && !eof_)
    {
        buffer_.erase(0, cursor_);
        cursor_ = 0;
        auto chunk = source_();
        eof_ = chunk.empty();
        buffer_ += chunk;
    }
    return cursor_ + ahead < buffer_.size()
        ? static_cast<unsigned char>(buffer_[cursor_ + ahead]) : -1;
}

char format_input::get()
{
    const auto byte = peek();
    if (byte < 0)
    {
        fail("invalid_syntax", "输入意外结束");
    }
    if (offset_ >= limit_ || offset_ - value_start_ >= value_limit_)
    {
        fail("size_limit", "输入超过配置的字节上限");
    }
    ++cursor_;
    ++offset_;
    if (byte == '\n')
    {
        ++line_;
        column_ = 1;
    }
    else if ((byte & 0xc0) != 0x80)
    {
        ++column_;
    }
    return static_cast<char>(byte);
}

bool format_input::take(char expected)
{
    if (peek() != static_cast<unsigned char>(expected))
    {
        return false;
    }
    (void)get();
    return true;
}

std::string_view format_input::take_json_ascii()
{
    if (peek() < 0)
    {
        return {};
    }
    const std::string_view buffer = source_ ? std::string_view(buffer_) : memory_;
    const auto available = std::min<std::uint64_t>(buffer.size() - cursor_,
        std::min(limit_ - offset_, value_limit_ - (offset_ - value_start_)));
    const auto start = cursor_;
    const auto end = start + static_cast<std::size_t>(available);
    while (cursor_ < end)
    {
        const auto byte = static_cast<unsigned char>(buffer[cursor_]);
        if (byte < 0x20 || byte >= 0x80 || byte == '"' || byte == '\\')
        {
            break;
        }
        ++cursor_;
    }
    const auto count = cursor_ - start;
    // 仅消费无换行的 ASCII；位置与逐字节 get 完全一致，限额处仍由 get 报错。
    offset_ += count;
    column_ += count;
    return buffer.substr(start, count);
}

void format_input::begin_value(std::uint64_t limit)
{
    value_start_ = offset_;
    value_limit_ = limit;
}

void format_input::end_value()
{
    value_limit_ = UINT64_MAX;
}

std::uint64_t format_input::offset() const
{
    return offset_;
}

void format_input::fail(const char* code, std::string_view reason) const
{
    throw runtime_failure({tx::error_kind::parse, code,
        format_ + " 第 " + std::to_string(line_) + " 行第 " +
        std::to_string(column_) + " 列（字节偏移 " +
        std::to_string(offset_) + "）：" + std::string(reason)});
}

} // namespace tx_generated
