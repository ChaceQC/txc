#include "performance_equivalence/parse_contract.hpp"
#include "performance_equivalence/serde_payload.hpp"

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
    const std::int64_t seed = epoch > 0 ? 3 : 4;
    const std::vector<std::int64_t> small(1000, seed);
    const std::vector<std::int64_t> large(100000, seed);
    report("vector_scan_1k", [&]()
    {
        std::int64_t checksum = 0;
        for (int repetition = 0; repetition < 1000; ++repetition)
        {
            for (const std::int64_t value : small)
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
            for (const std::int64_t value : large)
            {
                checksum += value;
            }
        }
        return checksum;
    });
}

void bench_vector_access()
{
    std::vector<std::int64_t> values;
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
    std::unordered_map<std::int64_t, std::int64_t> small;
    std::unordered_map<std::int64_t, std::int64_t> large;
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
            std::int64_t key = 100000 + (i % 8192);
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
    using key_type = std::variant<std::int64_t, std::string, bool>;
    const std::unordered_map<key_type, std::int64_t> values = {
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
            checksum += values.contains(key_type{static_cast<std::int64_t>(i)});
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

void bench_serde_size()
{
    const performance_equivalence::payload short_value{7, "x"};
    const performance_equivalence::payload long_value{7,
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"};
    report("serde_short_text", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 5000; ++i)
        {
            const auto decoded = performance_equivalence::deserialize_payload(
                performance_equivalence::serialize_payload(short_value));
            checksum += decoded.id + decoded.name.size();
        }
        return checksum;
    });
    report("serde_long_text", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 5000; ++i)
        {
            const auto decoded = performance_equivalence::deserialize_payload(
                performance_equivalence::serialize_payload(long_value));
            checksum += decoded.id + decoded.name.size();
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
            const std::string_view text = valid_inputs[i % 1000];
            const auto result = performance_equivalence::try_parse_int(text, 10);
            checksum += result.ok;
        }
        return checksum;
    });
    report("parse_invalid", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            const std::string_view text = invalid_inputs[i % 1000];
            const auto result = performance_equivalence::try_parse_int(text, 10);
            checksum += !result.ok;
        }
        return checksum;
    });
}

void bench_parse_core()
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
    report("parse_core_valid", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += performance_equivalence::parse_int_core(valid_inputs[i % 1000]);
        }
        return checksum;
    });
    report("parse_core_invalid", [&]()
    {
        std::int64_t checksum = 0;
        for (int i = 0; i < 100000; ++i)
        {
            checksum += !performance_equivalence::parse_int_core(invalid_inputs[i % 1000]);
        }
        return checksum;
    });
}

void check_contracts()
{
    using performance_equivalence::try_parse_int;
    const auto minimum = try_parse_int(" -9223372036854775808 ", 10);
    const auto signed_value = try_parse_int(" +7 ", 10);
    const auto invalid = try_parse_int("7x", 10);
    const auto overflow = try_parse_int("9223372036854775808", 10);
    if (!minimum.ok || minimum.value != INT64_MIN ||
        !signed_value.ok || signed_value.value != 7 ||
        invalid.ok || invalid.error.code != "invalid_syntax" ||
        overflow.ok || overflow.error.code != "out_of_range")
    {
        throw std::runtime_error("解析公开结果契约校验失败");
    }
    performance_equivalence::check_serde_contract();
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::string_view{argv[1]} == "--parse-core")
    {
        bench_parse_core();
        return 0;
    }
    if (argc == 2 && std::string_view{argv[1]} == "--contract-check")
    {
        check_contracts();
        return 0;
    }
    if (argc != 1)
    {
        throw std::runtime_error("未知的基准参数");
    }
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
