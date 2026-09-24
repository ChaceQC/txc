#include <algorithm>
#include <any>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <unordered_map>

using clock_type = std::chrono::steady_clock;
using time_point = clock_type::time_point;
using dynamic_array = std::vector<std::any>;
using dynamic_dict = std::vector<std::pair<std::any, std::any>>;

template<class value_type>
void report(const char* name, time_point started, value_type checksum)
{
    const auto elapsed = std::chrono::duration<double, std::milli>(
        clock_type::now() - started).count();
    std::cout << name << '\n' << elapsed << '\n' << checksum << '\n';
}

std::string replace_copy(std::string text, std::string old,
                         std::string replacement)
{
    std::size_t position = 0;
    while ((position = text.find(old, position)) != std::string::npos)
    {
        text.replace(position, old.size(), replacement);
        position += replacement.size();
    }
    return text;
}

std::vector<std::string> split_copy(std::string text, std::string separator)
{
    std::vector<std::string> result;
    std::size_t start = 0;
    while (true)
    {
        const auto position = text.find(separator, start);
        if (position == std::string::npos)
        {
            result.push_back(text.substr(start));
            return result;
        }
        result.push_back(text.substr(start, position - start));
        start = position + separator.size();
    }
}

dynamic_array array_concat(dynamic_array left, dynamic_array right)
{
    left.insert(left.end(), right.begin(), right.end());
    return left;
}

dynamic_array array_reverse(dynamic_array values)
{
    std::reverse(values.begin(), values.end());
    return values;
}

dynamic_array array_slice(dynamic_array values, std::size_t first,
                          std::size_t last)
{
    return dynamic_array(values.begin() + first, values.begin() + last);
}

void dict_set(dynamic_dict& values, std::int64_t key, std::int64_t value)
{
    for (auto& entry : values)
    {
        if (std::any_cast<std::int64_t>(entry.first) == key)
        {
            entry.second = value;
            return;
        }
    }
    values.emplace_back(key, value);
}

std::int64_t dict_get(dynamic_dict values, std::int64_t key)
{
    for (const auto& entry : values)
    {
        if (std::any_cast<std::int64_t>(entry.first) == key)
        {
            return std::any_cast<std::int64_t>(entry.second);
        }
    }
    return 0;
}

void bench_math()
{
    double checksum = 0.0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 1000000; ++i)
    {
        checksum += std::sqrt(static_cast<double>(i));
    }
    report("math", started, checksum);
}

void bench_random()
{
    std::mt19937_64 generator(12345);
    std::uniform_int_distribution<std::int64_t> distribution(0, 1000);
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 200000; ++i)
    {
        checksum += distribution(generator);
    }
    report("random", started, checksum);
}

void bench_random_long()
{
    std::mt19937_64 generator(12345);
    std::uniform_int_distribution<std::int64_t> distribution(0, 1000);
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 5000000; ++i)
    {
        checksum += distribution(generator);
    }
    report("random_long", started, checksum);
}

void bench_string()
{
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 10000; ++i)
    {
        auto replaced = replace_copy("alpha,beta,alpha", "alpha", "x");
        auto parts = split_copy(replaced, ",");
        checksum += static_cast<std::int64_t>(parts.size());
    }
    report("string", started, checksum);
}

void bench_array()
{
    dynamic_array values;
    for (std::int64_t i = 0; i < 64; ++i)
    {
        values.emplace_back(i);
    }
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 3000; ++i)
    {
        auto combined = array_concat(values, values);
        auto reversed = array_reverse(combined);
        auto middle = array_slice(reversed, 16, 112);
        checksum += std::any_cast<std::int64_t>(middle[0]) +
            static_cast<std::int64_t>(middle.size());
    }
    report("array", started, checksum);
}

void bench_dict_dynamic()
{
    dynamic_dict values;
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t repetition = 1; repetition <= 100; ++repetition)
    {
        for (std::int64_t i = 0; i < 128; ++i)
        {
            dict_set(values, i, i + repetition);
            checksum += dict_get(values, i);
        }
    }
    report("dict_dynamic", started, checksum);
}

void bench_dict_hash()
{
    std::unordered_map<std::int64_t, std::int64_t> values;
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t repetition = 1; repetition <= 100; ++repetition)
    {
        for (std::int64_t i = 0; i < 128; ++i)
        {
            values[i] = i + repetition;
            checksum += values.at(i);
        }
    }
    report("dict_hash", started, checksum);
}

void bench_path()
{
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 10000; ++i)
    {
        const auto joined = std::filesystem::path("tx_build") /
            "sub/data.txt";
        checksum += static_cast<std::int64_t>(
            joined.extension().string().size());
    }
    report("path", started, checksum);
}

void bench_fs()
{
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 200; ++i)
    {
        std::vector<std::string> entries;
        for (const auto& entry : std::filesystem::directory_iterator("tx/stdlib"))
        {
            entries.push_back(entry.path().filename().string());
        }
        std::sort(entries.begin(), entries.end());
        checksum += static_cast<std::int64_t>(entries.size());
        if (std::filesystem::is_regular_file("tx/stdlib/math.txh"))
        {
            checksum += 1;
        }
    }
    report("fs", started, checksum);
}

void bench_file()
{
    const std::string content =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    const std::filesystem::path path = "tx_build/library_compare_cpp.txt";
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 300; ++i)
    {
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output.write(content.data(), static_cast<std::streamsize>(content.size()));
        }
        std::ifstream input(path, std::ios::binary);
        const std::string loaded{std::istreambuf_iterator<char>(input),
                                 std::istreambuf_iterator<char>()};
        checksum += static_cast<std::int64_t>(loaded.size());
    }
    report("file", started, checksum);
}

void bench_io()
{
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 5000; ++i)
    {
        std::cerr.write("x", 1);
    }
    report("io", started, 5000);
}

void bench_time()
{
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 100000; ++i)
    {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now).count() > 0)
        {
            ++checksum;
        }
    }
    report("time", started, checksum);
}

int main(int argc, char** argv)
{
    std::cout << std::setprecision(17);
    if (argc > 1 && std::string_view(argv[1]) == "random-long")
    {
        bench_random_long();
        return 0;
    }
    bench_math();
    bench_random();
    bench_string();
    bench_array();
    bench_dict_dynamic();
    bench_dict_hash();
    bench_path();
    bench_fs();
    bench_file();
    bench_io();
    bench_time();
}
