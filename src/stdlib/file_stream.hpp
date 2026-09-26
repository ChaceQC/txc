#pragma once

#include "stdlib/bytes.hpp"
#include "stdlib/encoding.hpp"

#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace tx_generated
{

enum class stream_mode
{
    read,
    write,
    append,
    update
};

class stream_file
{
public:
    stream_file(std::string_view path, stream_mode mode);
    ~stream_file() noexcept;
    stream_file(const stream_file&) = delete;
    stream_file& operator=(const stream_file&) = delete;

    [[nodiscard]] stream_mode mode() const noexcept;
    void require_open() const;
    [[nodiscard]] std::string read(std::size_t size);
    [[nodiscard]] std::size_t read_into(char* destination, std::size_t size);
    void write(std::string_view data);
    [[nodiscard]] std::int64_t tell();
    [[nodiscard]] std::int64_t seek(std::int64_t offset, std::string_view origin);
    void flush();
    void close();

private:
    enum class direction
    {
        none,
        read,
        write
    };
    std::FILE* file_ = nullptr;
    stream_mode mode_;
    direction last_ = direction::none;
};

struct binary_stream_state
{
    binary_stream_state(std::string_view path, stream_mode mode);
    stream_file file;
};

struct text_stream_state
{
    text_stream_state(std::string_view path, stream_mode mode,
                      detail::text_encoding encoding);
    stream_file file;
    detail::text_encoding encoding;
    std::optional<std::string> pending;
};

using binary_stream = std::shared_ptr<binary_stream_state>;
using text_stream = std::shared_ptr<text_stream_state>;

struct byte_chunk_value
{
    byte_value data;
    bool eof = false;
};

struct text_chunk_value
{
    std::string text;
    bool eof = false;
};

struct line_result_value
{
    bool has_line = false;
    std::string line;
};

binary_stream open_binary_stream(std::string_view path, std::string_view mode);
byte_chunk_value stream_read_bytes(const binary_stream& source, std::int64_t size);
byte_value stream_read_all_bytes(const binary_stream& source);
void stream_write_bytes(const binary_stream& target, const byte_value& data);
std::int64_t stream_tell(const binary_stream& source);
std::int64_t stream_seek(const binary_stream& source, std::int64_t offset,
                         std::string_view origin);
void stream_flush(const binary_stream& target);
void stream_close(const binary_stream& target);

text_stream open_text_stream(std::string_view path, std::string_view mode,
                             std::string_view encoding);
text_chunk_value stream_read_chars(const text_stream& source, std::int64_t count);
line_result_value stream_read_line(const text_stream& source,
                                    std::int64_t max_bytes);
void stream_write_text(const text_stream& target, std::string_view text);
void stream_write_line(const text_stream& target, std::string_view text);
void stream_flush(const text_stream& target);
void stream_close(const text_stream& target);

} // namespace tx_generated
