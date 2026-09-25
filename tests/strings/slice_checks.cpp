#include "stdlib/stdlib.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

bool rejects(const std::string& text, std::int64_t start, std::int64_t end,
             const std::string& expected)
{
    try
    {
        (void)tx_generated::tx_fn_slice(text, start, end);
    }
    catch (const std::exception& error)
    {
        return error.what() == expected;
    }
    return false;
}

} // namespace

int main()
{
    const auto invalid = std::string("ok") + static_cast<char>(0xff);
    const bool valid = tx_generated::tx_fn_slice("你好🙂", 1, 3) == "好🙂" &&
        tx_generated::tx_fn_slice("你好🙂", 3, 3).empty() &&
        tx_generated::tx_fn_slice("", 0, 0).empty() &&
        rejects("你好🙂", -1, 1, "字符串切片范围无效") &&
        rejects("你好🙂", 0, 4, "字符串切片范围无效") &&
        rejects(invalid, 0, 1, "字符串包含无效 UTF-8") &&
        rejects(invalid, -1, 1, "字符串包含无效 UTF-8");
    if (!valid)
    {
        return 1;
    }
    std::cout << "UTF-8 slice boundaries and error priority: 7/7\n";
}
