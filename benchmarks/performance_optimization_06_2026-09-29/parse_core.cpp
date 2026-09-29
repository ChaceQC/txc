#include "stdlib/parse.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{

void measure(const char* name, const std::vector<std::string>& inputs)
{
    std::int64_t checksum = 0;
    const auto started = std::chrono::steady_clock::now();
    for (int index = 0; index < 1000000; ++index)
    {
#ifdef SCALAR_CORE
        const auto result = tx_generated::parse_int_scalar(inputs[index % 1000], 10);
        checksum += result.error == tx_generated::parse_error::none;
#else
        const auto result = tx_generated::try_parse_int(inputs[index % 1000], 10);
        checksum += result.ok;
#endif
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - started).count();
    std::cout << name << '\n' << elapsed << '\n' << checksum << '\n';
}

} // namespace

int main()
{
    std::vector<std::string> valid;
    std::vector<std::string> invalid;
    for (int index = 0; index < 1000; ++index)
    {
        valid.push_back(std::to_string(index));
        invalid.push_back(valid.back() + "x");
    }
    measure("valid", valid);
    measure("invalid", invalid);
}
