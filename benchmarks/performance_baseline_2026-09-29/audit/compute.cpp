#include <mbedtls/sha256.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using clock_type = std::chrono::steady_clock;

template<class Operation>
void measure(const char* name, Operation operation)
{
    const auto start = clock_type::now();
    const auto checksum = operation();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        clock_type::now() - start).count();
    std::cout << name << '\n' << elapsed << '\n' << checksum << '\n';
}

std::int64_t algorithm_sort()
{
    std::vector<int> values;
    for (int i = 0; i < 256; ++i)
    {
        values.push_back(255 - i);
    }
    std::int64_t checksum = 0;
    for (int i = 0; i < 5000; ++i)
    {
        auto ordered = values;
        std::sort(ordered.begin(), ordered.end());
        checksum += ordered[0] + ordered[255];
    }
    return checksum;
}

std::string to_hex(const std::string& data)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(data.size() * 2);
    for (unsigned char value : data)
    {
        result.push_back(digits[value >> 4]);
        result.push_back(digits[value & 15]);
    }
    return result;
}

std::string from_hex(const std::string& text)
{
    auto digit = [](char value) -> unsigned char
    {
        if (value >= '0' && value <= '9')
        {
            return static_cast<unsigned char>(value - '0');
        }
        return static_cast<unsigned char>(value - 'a' + 10);
    };
    std::string result;
    result.reserve(text.size() / 2);
    for (std::size_t index = 0; index < text.size(); index += 2)
    {
        result.push_back(static_cast<char>(digit(text[index]) * 16 +
            digit(text[index + 1])));
    }
    return result;
}

std::int64_t bytes_hex()
{
    const std::string data = "Hello, 世界!";
    std::int64_t checksum = 0;
    for (int i = 0; i < 20000; ++i)
    {
        checksum += from_hex(to_hex(data)).size();
    }
    return checksum;
}

std::int64_t cancel_status()
{
    static bool cancelled = false;
    static std::mutex lock;
    std::int64_t checksum = 0;
    for (int i = 0; i < 100000; ++i)
    {
        std::lock_guard guard(lock);
        checksum += cancelled ? 1 : 0;
    }
    return checksum;
}

std::int64_t crypto_sha256()
{
    const std::string data = "0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef";
    std::array<unsigned char, 32> output{};
    std::int64_t checksum = 0;
    for (int i = 0; i < 10000; ++i)
    {
        if (mbedtls_sha256(reinterpret_cast<const unsigned char*>(data.data()),
            data.size(), output.data(), 0) != 0)
        {
            throw std::runtime_error("SHA-256 failed");
        }
        checksum += output.size();
    }
    return checksum;
}

std::int64_t dictionary_contains()
{
    std::unordered_map<int, int> values;
    for (int i = 0; i < 128; ++i)
    {
        values.emplace(i, i);
    }
    std::int64_t checksum = 0;
    for (int i = 0; i < 100000; ++i)
    {
        checksum += values.contains(i % 128) ? 1 : 0;
    }
    return checksum;
}

std::int64_t env_get()
{
    _putenv_s("TX_PERF_AUDIT_VALUE", "sample");
    std::int64_t checksum = 0;
    for (int i = 0; i < 20000; ++i)
    {
        const char* value = std::getenv("TX_PERF_AUDIT_VALUE");
        checksum += value ? std::string(value).size() : 0;
    }
    _putenv_s("TX_PERF_AUDIT_VALUE", "");
    return checksum;
}

std::int64_t format_text()
{
    std::int64_t checksum = 0;
    for (int i = 0; i < 10000; ++i)
    {
        std::ostringstream stream;
        stream << "alpha" << ':' << 42;
        checksum += stream.str().size();
    }
    return checksum;
}

std::int64_t math_sqrt()
{
    double checksum = 0.0;
    for (int i = 1; i <= 1000000; ++i)
    {
        checksum += std::sqrt(static_cast<double>(i));
    }
    return static_cast<std::int64_t>(checksum);
}

std::int64_t parse_int()
{
    const std::string number = "12345";
    std::int64_t checksum = 0;
    for (int i = 0; i < 100000; ++i)
    {
        checksum += std::stoll(number);
    }
    return checksum;
}

std::int64_t random_int()
{
    std::mt19937_64 source(12345);
    std::uniform_int_distribution<std::int64_t> distribution(0, 1000);
    std::int64_t checksum = 0;
    for (int i = 0; i < 200000; ++i)
    {
        checksum += distribution(source);
    }
    return checksum;
}

std::int64_t regex_search()
{
    const std::regex pattern("[a-z]+[0-9]+");
    const std::string text = "prefix abc123 suffix";
    std::int64_t checksum = 0;
    for (int i = 0; i < 20000; ++i)
    {
        std::smatch result;
        if (std::regex_search(text, result, pattern))
        {
            checksum += result.str().size();
        }
    }
    return checksum;
}

std::int64_t statistics_mean()
{
    std::vector<double> values;
    for (int i = 0; i < 128; ++i)
    {
        values.push_back(i);
    }
    std::int64_t checksum = 0;
    for (int i = 0; i < 10000; ++i)
    {
        const auto sum = std::accumulate(values.begin(), values.end(), 0.0);
        checksum += static_cast<std::int64_t>(sum / values.size());
    }
    return checksum;
}

std::int64_t test_assert()
{
    std::int64_t checksum = 0;
    for (int i = 0; i < 100000; ++i)
    {
        if (i < 0)
        {
            throw std::runtime_error("negative index");
        }
        ++checksum;
    }
    return checksum;
}

int main()
{
    measure("algorithm_sort", algorithm_sort);
    measure("bytes_hex", bytes_hex);
    measure("cancel_status", cancel_status);
    measure("crypto_sha256", crypto_sha256);
    measure("dictionary_contains", dictionary_contains);
    measure("env_get", env_get);
    measure("format_text", format_text);
    measure("math_sqrt", math_sqrt);
    measure("parse_int", parse_int);
    measure("random_int", random_int);
    measure("regex_search", regex_search);
    measure("statistics_mean", statistics_mean);
    measure("test_assert", test_assert);
    return 0;
}
