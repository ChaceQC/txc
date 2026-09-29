#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <string>

std::string apply_template(const std::string& pattern, std::int64_t value)
{
    return std::vformat(pattern, std::make_format_args(value));
}

int main()
{
    for (const bool through_parameter : {false, true})
    {
        std::int64_t checksum = 0;
        const auto start = std::chrono::steady_clock::now();
        for (std::int64_t value = 0; value < 50000; ++value)
        {
            std::string pattern = value % 2 == 0 ? "item:{:06d}" : "item:{:04d}";
            checksum += (through_parameter ? apply_template(pattern, value) :
                std::vformat(pattern, std::make_format_args(value))).size();
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start).count();
        std::cout << (through_parameter ? "format_parameter" : "format_alternating")
                  << '\n' << elapsed << '\n' << checksum << '\n';
    }
}
