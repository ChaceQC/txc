#include <charconv>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

using clock_type = std::chrono::steady_clock;

template <typename operation_type>
void report(std::string_view name, operation_type operation)
{
    const auto started = clock_type::now();
    const std::int64_t checksum = operation();
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
        clock_type::now() - started).count();
    std::cout << name << '\n' << micros << '\n' << checksum << '\n';
}

void bench_vector_scale()
{
    // 使用运行时种子，避免 Release 把固定值求和预先折叠。
    const auto epoch = std::chrono::system_clock::now().time_since_epoch().count();
    const int seed = epoch > 0 ? 3 : 4;
    const std::vector<int> small(1000, seed);
    const std::vector<int> large(100000, seed);
    report("vector_scan_1k", [&]()
    {
        std::int64_t checksum = 0;
        for (int repetition = 0; repetition < 1000; ++repetition)
        {
            for (const int value : small)
            {
                checksum += value;
            }
        }
        return checksum;
    });
    report("vector_scan_100k", [&]()
    {
        std::int64_t checksum = 0;
        for (int repetition = 0; repetition < 100; ++repetition)
        {
            for (const int value : large)
            {
                checksum += value;
            }
        }
        return checksum;
    });
}

void bench_vector_access()
{
    std::vector<int> values;
    values.reserve(8192);
    for (int i = 0; i < 8192; ++i)
    {
        values.push_back(i % 97);
    }
    report("vector_index_sequential", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 1000000; ++i)
        {
            checksum += values[i % 8192];
        }
        return checksum;
    });
    report("vector_index_strided", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 1000000; ++i)
        {
            checksum += values[(i * 127) % 8192];
        }
        return checksum;
    });
}

void bench_map_distribution()
{
    std::unordered_map<int, int> small;
    std::unordered_map<int, int> large;
    small.reserve(128);
    large.reserve(8192);
    for (int i = 0; i < 8192; ++i)
    {
        large.emplace(i, i);
        if (i < 128)
        {
            small.emplace(i, i);
        }
    }
    report("map_hit_128", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 500000; ++i)
        {
            checksum += small.at(i % 128);
        }
        return checksum;
    });
    report("map_hit_8192", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 500000; ++i)
        {
            checksum += large.at((i * 127) % 8192);
        }
        return checksum;
    });
    report("map_hit_10_percent", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 500000; ++i)
        {
            int key = 100000 + (i % 8192);
            if (i % 10 == 0)
            {
                key = i % 8192;
            }
            checksum += large.contains(key);
        }
        return checksum;
    });
}

void bench_dictionary_keys()
{
    using key_type = std::variant<int, std::string, bool>;
    const std::unordered_map<key_type, int> values = {
        {key_type{7}, 1}, {key_type{std::string{"seven"}}, 2}, {key_type{true}, 3}
    };
    report("dictionary_int_hit", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 200000; ++i)
        {
            checksum += values.contains(key_type{7});
        }
        return checksum;
    });
    report("dictionary_text_hit", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 200000; ++i)
        {
            checksum += values.contains(key_type{std::string{"seven"}});
        }
        return checksum;
    });
    report("dictionary_mostly_miss", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 200000; ++i)
        {
            checksum += values.contains(key_type{i});
        }
        return checksum;
    });
}

std::string format_dynamic(std::string_view pattern, int value)
{
    const auto first = pattern.find("{}");
    const auto second = pattern.find("{}", first + 2);
    if (first == std::string_view::npos || second == std::string_view::npos)
    {
        throw std::runtime_error("unexpected format pattern");
    }
    std::string result;
    result.append(pattern.substr(0, first));
    result.append("item");
    result.append(pattern.substr(first + 2, second - first - 2));
    result.append(std::to_string(value));
    result.append(pattern.substr(second + 2));
    return result;
}

void bench_format_paths()
{
    const std::string pattern = "{}:{}";
    report("format_literal", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 50000; ++i)
        {
            checksum += (std::string{"item:"} + std::to_string(i)).size();
        }
        return checksum;
    });
    report("format_dynamic", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 50000; ++i)
        {
            checksum += format_dynamic(pattern, i).size();
        }
        return checksum;
    });
}

std::string encode_utf8(std::u32string_view value)
{
    std::string result;
    for (const char32_t scalar : value)
    {
        if (scalar <= 0x7f)
        {
            result.push_back(static_cast<char>(scalar));
        }
        else if (scalar <= 0x7ff)
        {
            result.push_back(static_cast<char>(0xc0 | (scalar >> 6)));
            result.push_back(static_cast<char>(0x80 | (scalar & 0x3f)));
        }
        else if (scalar <= 0xffff)
        {
            result.push_back(static_cast<char>(0xe0 | (scalar >> 12)));
            result.push_back(static_cast<char>(0x80 | ((scalar >> 6) & 0x3f)));
            result.push_back(static_cast<char>(0x80 | (scalar & 0x3f)));
        }
        else
        {
            result.push_back(static_cast<char>(0xf0 | (scalar >> 18)));
            result.push_back(static_cast<char>(0x80 | ((scalar >> 12) & 0x3f)));
            result.push_back(static_cast<char>(0x80 | ((scalar >> 6) & 0x3f)));
            result.push_back(static_cast<char>(0x80 | (scalar & 0x3f)));
        }
    }
    return result;
}

void bench_encoding_paths()
{
    constexpr std::u32string_view value = U"Hello, 世界 🌍";
    const std::string codec = "utf-8";
    report("encoding_literal", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 50000; ++i)
        {
            checksum += encode_utf8(value).size();
        }
        return checksum;
    });
    report("encoding_dynamic", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 50000; ++i)
        {
            if (codec != "utf-8")
            {
                throw std::runtime_error("unexpected encoding");
            }
            checksum += encode_utf8(value).size();
        }
        return checksum;
    });
}

std::int64_t fixed_json_roundtrip(std::string_view name)
{
    const std::string json = "{\"id\":7,\"name\":\"" + std::string{name} + "\"}";
    const auto name_start = json.find("\"name\":\"") + 8;
    const auto name_end = json.find('"', name_start);
    int id = 0;
    const auto start = json.data() + 6;
    const auto parsed = std::from_chars(start, json.data() + json.size(), id);
    if (parsed.ec != std::errc{} || name_end == std::string::npos)
    {
        throw std::runtime_error("invalid JSON result");
    }
    return id + static_cast<std::int64_t>(name_end - name_start);
}

void bench_serde_size()
{
    const std::string short_name = "x";
    const std::string long_name = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    report("serde_short_text", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 5000; ++i)
        {
            checksum += fixed_json_roundtrip(short_name);
        }
        return checksum;
    });
    report("serde_long_text", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 5000; ++i)
        {
            checksum += fixed_json_roundtrip(long_name);
        }
        return checksum;
    });
}

void bench_parse_paths()
{
    std::vector<std::string> valid_inputs;
    std::vector<std::string> invalid_inputs;
    valid_inputs.reserve(1000);
    invalid_inputs.reserve(1000);
    for (int i = 0; i < 1000; ++i)
    {
        valid_inputs.push_back(std::to_string(i));
        invalid_inputs.push_back(valid_inputs.back() + "x");
    }
    report("parse_valid", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            int parsed = 0;
            const std::string_view text = valid_inputs[i % 1000];
            const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
            checksum += result.ec == std::errc{} && result.ptr == text.data() + text.size();
        }
        return checksum;
    });
    report("parse_invalid", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            int parsed = 0;
            const std::string_view text = invalid_inputs[i % 1000];
            const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
            checksum += result.ec != std::errc{} || result.ptr != text.data() + text.size();
        }
        return checksum;
    });
}

int main()
{
    bench_vector_scale();
    bench_vector_access();
    bench_map_distribution();
    bench_dictionary_keys();
    bench_format_paths();
    bench_encoding_paths();
    bench_serde_size();
    bench_parse_paths();
    return 0;
}
