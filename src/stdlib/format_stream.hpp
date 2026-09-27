#pragma once

#include "stdlib/file_stream.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace tx_generated
{

using format_source = std::function<std::string()>;
using format_sink = std::function<void(std::string_view)>;

format_source format_read_source(const binary_stream& source);
format_source format_read_source(const text_stream& source);
format_sink format_write_sink(const binary_stream& target);
format_sink format_write_sink(const text_stream& target);
std::function<void()> format_stream_guard(const binary_stream& stream);
std::function<void()> format_stream_guard(const text_stream& stream);

// 有界预读与位置追踪供格式解析器共用；不持有已经消费的文件前缀。
class format_input
{
public:
    format_input(std::string_view text, std::string format);
    format_input(format_source source, std::string format, std::uint64_t limit);
    int peek(std::size_t ahead = 0);
    char get();
    bool take(char expected);
    void begin_value(std::uint64_t limit);
    void end_value();
    [[noreturn]] void fail(const char* code, std::string_view reason) const;
    std::uint64_t offset() const;

private:
    format_source source_;
    std::string buffer_;
    std::string_view memory_;
    std::string format_;
    std::size_t cursor_ = 0;
    std::uint64_t offset_ = 0;
    std::uint64_t line_ = 1;
    std::uint64_t column_ = 1;
    std::uint64_t limit_ = UINT64_MAX;
    std::uint64_t value_start_ = 0;
    std::uint64_t value_limit_ = UINT64_MAX;
    bool eof_ = false;
};

} // namespace tx_generated
