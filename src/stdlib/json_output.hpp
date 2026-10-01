#pragma once

#include "stdlib/format_stream.hpp"
#include "stdlib/error.hpp"

#include <utility>

namespace tx_generated::json_detail
{

class output_buffer
{
public:
    output_buffer(format_sink sink, std::uint64_t limit)
        : sink_(std::move(sink)), limit_(limit)
    {
    }

    void append(std::string_view text)
    {
        if (text.size() > limit_ - written_)
        {
            throw runtime_failure({tx::error_kind::runtime, "size_limit",
                "JSON 输出超过配置的字节上限"});
        }
        written_ += text.size();
        // 调用方始终在 UTF-8 标量边界 append，文本流不会收到半个字符。
        if (sink_ && buffer_.size() + text.size() > 4096)
        {
            flush();
        }
        // 连续 ASCII 快路径可能一次传来很长的文本。流式输出仍必须有界，
        // 分块位置退到 UTF-8 标量边界；内存输出不增加分块或复制。
        if (sink_)
        {
            while (text.size() > 4096)
            {
                std::size_t count = 4096;
                while (count > 0 && (static_cast<unsigned char>(text[count]) & 0xc0) == 0x80)
                {
                    --count;
                }
                if (count == 0)
                {
                    throw runtime_failure({tx::error_kind::runtime, "invalid_utf8",
                        "JSON 输出包含无效 UTF-8"});
                }
                sink_(text.substr(0, count));
                text.remove_prefix(count);
            }
        }
        buffer_.append(text);
    }

    void append(std::size_t count, char value)
    {
        append(std::string(count, value));
    }

    void push_back(char value)
    {
        append(std::string_view(&value, 1));
    }

    output_buffer& operator+=(std::string_view text)
    {
        append(text);
        return *this;
    }

    std::string finish()
    {
        if (sink_)
        {
            flush();
        }
        return std::move(buffer_);
    }

private:
    void flush()
    {
        if (!buffer_.empty())
        {
            sink_(buffer_);
            buffer_.clear();
        }
    }

    format_sink sink_;
    std::string buffer_;
    std::uint64_t limit_;
    std::uint64_t written_ = 0;
};

} // namespace tx_generated::json_detail
