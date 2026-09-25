#include "stdlib/json.hpp"
#include "stdlib/json_utf8.hpp"

#include <algorithm>
#include <any>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tx_generated
{
namespace
{

class json_writer
{
public:
    explicit json_writer(std::int64_t indent) : indent_(indent)
    {
        if (indent < -1 || indent > 16)
        {
            fail("invalid_indent", "JSON 缩进宽度必须在 0 到 16 之间");
        }
    }

    [[nodiscard]] std::string write(const std::any& value)
    {
        write_value(value, 0);
        return std::move(output_);
    }

private:
    [[noreturn]] static void fail(const char* code, const char* message)
    {
        throw runtime_failure({tx::error_kind::runtime, code, message});
    }

    void newline(std::size_t depth)
    {
        if (indent_ >= 0)
        {
            output_.push_back('\n');
            output_.append(depth * static_cast<std::size_t>(indent_), ' ');
        }
    }

    void write_string(std::string_view text)
    {
        constexpr char hex[] = "0123456789abcdef";
        output_.push_back('"');
        for (std::size_t offset = 0; offset < text.size();)
        {
            const auto byte = static_cast<unsigned char>(text[offset]);
            if (byte >= 0x80)
            {
                const auto width = json_detail::utf8_width(text, offset);
                if (width == 0)
                {
                    fail("invalid_utf8", "JSON 字符串包含无效 UTF-8");
                }
                output_.append(text.substr(offset, width));
                offset += width;
                continue;
            }
            ++offset;
            switch (byte)
            {
            case '"': output_ += "\\\""; break;
            case '\\': output_ += "\\\\"; break;
            case '\b': output_ += "\\b"; break;
            case '\f': output_ += "\\f"; break;
            case '\n': output_ += "\\n"; break;
            case '\r': output_ += "\\r"; break;
            case '\t': output_ += "\\t"; break;
            default:
                if (byte < 0x20)
                {
                    output_ += "\\u00";
                    output_.push_back(hex[byte >> 4]);
                    output_.push_back(hex[byte & 0x0f]);
                }
                else
                {
                    output_.push_back(static_cast<char>(byte));
                }
            }
        }
        output_.push_back('"');
    }

    void write_float(double value)
    {
        if (!std::isfinite(value))
        {
            fail("non_finite", "JSON 不能序列化 NaN 或无穷大");
        }
        char buffer[128];
        const auto [end, error] = std::to_chars(
            buffer, buffer + sizeof(buffer), value, std::chars_format::general);
        if (error != std::errc{})
        {
            fail("operation_failed", "JSON 浮点数序列化失败");
        }
        const std::string_view text(buffer, end);
        output_.append(text);
        if (text.find_first_of(".eE") == std::string_view::npos)
        {
            output_ += ".0";
        }
    }

    void write_array(const tx_array& values, std::size_t depth)
    {
        if (!active_.insert(values.identity()).second)
        {
            fail("cyclic_value", "JSON 不能序列化循环引用的数组");
        }
        output_.push_back('[');
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            if (index != 0)
            {
                output_.push_back(',');
            }
            newline(depth + 1);
            write_value(values[index], depth + 1);
        }
        if (values.size() != 0)
        {
            newline(depth);
        }
        output_.push_back(']');
        active_.erase(values.identity());
    }

    void write_object(const tx_dict& values, std::size_t depth)
    {
        if (!active_.insert(values.identity()).second)
        {
            fail("cyclic_value", "JSON 不能序列化循环引用的对象");
        }
        using field = std::pair<std::string_view, const std::any*>;
        std::vector<field> fields;
        fields.reserve(values.size());
        values.for_each([&](const std::any& key, const std::any& value)
        {
            const auto* name = std::any_cast<std::string>(&key);
            if (!name)
            {
                fail("invalid_key", "JSON 对象的字段名必须是字符串");
            }
            fields.emplace_back(*name, &value);
        });
        // 字典内部使用哈希索引，输出按 UTF-8 字节序固定排列。
        std::sort(fields.begin(), fields.end(),
            [](const field& left, const field& right)
            {
                return left.first < right.first;
            });
        output_.push_back('{');
        for (std::size_t index = 0; index < fields.size(); ++index)
        {
            if (index != 0)
            {
                output_.push_back(',');
            }
            newline(depth + 1);
            write_string(fields[index].first);
            output_ += indent_ >= 0 ? ": " : ":";
            write_value(*fields[index].second, depth + 1);
        }
        if (!fields.empty())
        {
            newline(depth);
        }
        output_.push_back('}');
        active_.erase(values.identity());
    }

    void write_value(const std::any& value, std::size_t depth)
    {
        if (depth > 128)
        {
            fail("depth_limit", "JSON 嵌套超过 128 层");
        }
        if (!value.has_value())
        {
            output_ += "null";
        }
        else if (const auto* item = std::any_cast<bool>(&value))
        {
            output_ += *item ? "true" : "false";
        }
        else if (const auto* item = std::any_cast<std::int64_t>(&value))
        {
            output_ += std::to_string(*item);
        }
        else if (const auto* item = std::any_cast<double>(&value))
        {
            write_float(*item);
        }
        else if (const auto* item = std::any_cast<std::string>(&value))
        {
            write_string(*item);
        }
        else if (const auto* item = std::any_cast<tx_array>(&value))
        {
            write_array(*item, depth);
        }
        else if (const auto* item = std::any_cast<tx_dict>(&value))
        {
            write_object(*item, depth);
        }
        else
        {
            fail("unsupported_value", "JSON 只支持 none、bool、int、float、str、array 和 dict");
        }
    }

    std::int64_t indent_;
    std::string output_;
    std::unordered_set<const void*> active_;
};

} // namespace

std::string json_stringify(const std::any& value)
{
    return json_writer(-1).write(value);
}

std::string json_stringify_pretty(const std::any& value, std::int64_t indent)
{
    if (indent < 0)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_indent",
                               "JSON 缩进宽度必须在 0 到 16 之间"});
    }
    return json_writer(indent).write(value);
}

} // namespace tx_generated
