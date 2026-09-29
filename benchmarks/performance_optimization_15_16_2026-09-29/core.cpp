#include "stdlib/bytes.hpp"
#include "stdlib/stdlib.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{

using clock_type = std::chrono::steady_clock;

void report(const std::string& name, clock_type::time_point start, std::int64_t checksum)
{
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        clock_type::now() - start).count();
    std::cout << name << '\n' << elapsed << '\n' << checksum << '\n';
}

// 独立完整长度/字符契约。用于核对算法尺度，不代表 TX 句柄/诊断外围。
std::string reference_hex(const tx_generated::byte_value& data)
{
    if (data->size() > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()) / 2 ||
        data->size() > std::string{}.max_size() / 2)
    {
        throw std::length_error("hex length");
    }
    std::string result(data->size() * 2, '\0');
    const std::string_view digits = "0123456789abcdef";
    for (std::size_t index = 0; index < data->size(); ++index)
    {
        result[index * 2] = digits[(*data)[index] / 16];
        result[index * 2 + 1] = digits[(*data)[index] % 16];
    }
    return result;
}

int reference_digit(unsigned char value)
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f')
    {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F')
    {
        return value - 'A' + 10;
    }
    return -1;
}

tx_generated::byte_value reference_unhex(std::string_view text)
{
    if (text.size() % 2 != 0)
    {
        throw std::invalid_argument("odd length");
    }
    for (const unsigned char character : text)
    {
        if (reference_digit(character) < 0)
        {
            throw std::invalid_argument("invalid hex");
        }
    }
    std::vector<std::uint8_t> values(text.size() / 2);
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        values[index] = static_cast<std::uint8_t>(
            reference_digit(text[index * 2]) * 16 + reference_digit(text[index * 2 + 1]));
    }
    return std::make_shared<const std::vector<std::uint8_t>>(std::move(values));
}

void hex_paths(std::size_t size, int rounds, const std::string& label, bool reference)
{
    std::vector<std::uint8_t> values(size);
    for (std::size_t index = 0; index < size; ++index)
    {
        values[index] = static_cast<std::uint8_t>(index % 256);
    }
    const auto data = tx_generated::make_bytes(std::move(values));
    const auto text = reference_hex(data);
    if (tx_generated::bytes_to_hex(data) != text ||
        *tx_generated::bytes_from_hex(text) != *data || *reference_unhex(text) != *data)
    {
        throw std::runtime_error("incorrect output");
    }
    std::int64_t checksum = 0;
    auto start = clock_type::now();
    for (int index = 0; index < rounds; ++index)
    {
        const auto result = reference ? reference_hex(data) : tx_generated::bytes_to_hex(data);
        checksum += static_cast<std::int64_t>(result.size());
    }
    report("hex_encode_" + label, start, checksum);
    checksum = 0;
    start = clock_type::now();
    for (int index = 0; index < rounds; ++index)
    {
        const auto result = reference ? reference_unhex(text) : tx_generated::bytes_from_hex(text);
        checksum += static_cast<std::int64_t>(result->size());
    }
    report("hex_decode_" + label, start, checksum);
}

double reference_mean(const std::vector<double>& values)
{
    // 与 TX 相同的扩展精度、顺序与 Neumaier 补偿；不以普通 double 求和对照。
    static_assert(std::numeric_limits<long double>::max_exponent > 1088);
    long double total = 0;
    long double correction = 0;
    std::int64_t count = 0;
    for (const auto value : values)
    {
        if (!std::isfinite(value) || count == std::numeric_limits<std::int64_t>::max())
        {
            throw std::runtime_error("invalid sample");
        }
        ++count;
        const auto sample = static_cast<long double>(value);
        const auto next = total + sample;
        correction += std::fabs(total) >= std::fabs(sample)
            ? (total - next) + sample : (sample - next) + total;
        total = next;
    }
    if (count == 0)
    {
        throw std::runtime_error("empty sample");
    }
    const auto result = (total + correction) / count;
    if (!std::isfinite(result) || std::fabs(result) > std::numeric_limits<double>::max())
    {
        throw std::runtime_error("out of range");
    }
    return static_cast<double>(result);
}

void mean_path(std::size_t size, int rounds, const std::string& label)
{
    std::vector<double> values(size);
    for (std::size_t index = 0; index < size; ++index)
    {
        values[index] = static_cast<double>(index % 128);
    }
    std::int64_t checksum = 0;
    const auto start = clock_type::now();
    for (int index = 0; index < rounds; ++index)
    {
        checksum += static_cast<std::int64_t>(reference_mean(values));
    }
    report("mean_" + label, start, checksum);
}

} // namespace

int main(int argc, char** argv)
{
    const bool reference = argc > 1 && std::string_view(argv[1]) == "reference";
    hex_paths(16, 100000, "short", reference);
    hex_paths(65536, 100, "large", reference);
    if (reference)
    {
        mean_path(128, 10000, "short");
        mean_path(65536, 100, "large");
    }
}
