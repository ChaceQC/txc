// 固定 ASCII 场景参考，不实现公开 format 的完整语法、错误和动态容器契约。
#include <charconv>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace
{

std::string format_item(std::int64_t value, char separator)
{
    std::string result = "item";
    result += separator;
    char buffer[20];
    const auto converted = std::to_chars(buffer, buffer + sizeof(buffer), value);
    result.append(buffer, converted.ptr);
    return result;
}

void measure(std::string_view name, bool dynamic)
{
    std::int64_t checksum = 0;
    const auto started = std::chrono::steady_clock::now();
    for (std::int64_t value = 0; value < 50000; ++value)
    {
        checksum += format_item(value, dynamic && value % 2 != 0 ? '-' : ':').size();
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - started).count();
    std::cout << name << '\n' << elapsed << '\n' << checksum << '\n';
}

} // namespace

int main()
{
    measure("format_literal", false);
    measure("format_local", false);
    measure("format_dynamic", true);
}
