#include "stdlib/encoding.hpp"
#include "stdlib/stdlib.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <typeinfo>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace tx_generated
{
namespace
{

bool is_console(DWORD stream)
{
    const HANDLE handle = GetStdHandle(stream);
    DWORD mode = 0;
    return handle != nullptr && handle != INVALID_HANDLE_VALUE &&
           GetConsoleMode(handle, &mode) != 0;
}

struct console_utf8
{
    UINT input_code_page = 0;
    UINT output_code_page = 0;
    bool changed_input = false;
    bool changed_output = false;

    console_utf8()
    {
        if (is_console(STD_OUTPUT_HANDLE) || is_console(STD_ERROR_HANDLE))
        {
            output_code_page = GetConsoleOutputCP();
            changed_output = output_code_page != CP_UTF8;
            if (changed_output && !SetConsoleOutputCP(CP_UTF8))
            {
                throw std::runtime_error("无法把终端输出字符集设为 UTF-8");
            }
        }
        if (is_console(STD_INPUT_HANDLE))
        {
            input_code_page = GetConsoleCP();
            changed_input = input_code_page != CP_UTF8;
            if (changed_input && !SetConsoleCP(CP_UTF8))
            {
                if (changed_output)
                {
                    (void)SetConsoleOutputCP(output_code_page);
                }
                throw std::runtime_error("无法把终端输入字符集设为 UTF-8");
            }
        }
    }

    ~console_utf8()
    {
        // 先输出缓冲数据，再恢复原控制台代码页。
        std::cout.flush();
        std::cerr.flush();
        if (changed_input)
        {
            (void)SetConsoleCP(input_code_page);
        }
        if (changed_output)
        {
            (void)SetConsoleOutputCP(output_code_page);
        }
    }
};

void prepare_console()
{
    static const console_utf8 state;
    (void)state;
}

void write_checked(std::ostream& stream, std::string_view text,
                   const char* error_message)
{
    prepare_console();
    if (text.size() > static_cast<std::size_t>(
            std::numeric_limits<std::streamsize>::max()))
    {
        throw std::runtime_error("终端输出文本过长");
    }
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream)
    {
        throw std::runtime_error(error_message);
    }
}

} // namespace

void tx_prepare_console()
{
    prepare_console();
}

std::string tx_input()
{
    prepare_console();
    std::string result;
    if (!std::getline(std::cin, result))
    {
        throw std::runtime_error(std::cin.bad()
            ? "标准输入读取失败" : "标准输入已结束");
    }
    if (!result.empty() && result.back() == '\r')
    {
        result.pop_back();
    }
    (void)detail::utf8_to_wide(result);
    return result;
}

std::string tx_input(const std::string& prompt)
{
    tx_fn_write(prompt);
    tx_fn_flush();
    return tx_input();
}

void tx_fn_write(std::string text)
{
    write_checked(std::cout, text, "标准输出写入失败");
}

void tx_fn_write_line(std::string text)
{
    text.push_back('\n');
    write_checked(std::cout, text, "标准输出写入失败");
}

void tx_fn_write_error(std::string text)
{
    write_checked(std::cerr, text, "标准错误写入失败");
}

void tx_fn_flush()
{
    prepare_console();
    std::cout.flush();
    if (!std::cout)
    {
        throw std::runtime_error("标准输出刷新失败");
    }
}

void tx_print(tx_int value)
{
    tx_fn_write_line(tx_int_to_string(value));
}

void tx_print(double value)
{
    tx_fn_write_line(tx_float_to_string(value));
}

void tx_print(bool value)
{
    tx_fn_write_line(tx_bool_to_string(value));
}

void tx_print(const std::string& value)
{
    tx_fn_write_line(value);
}

void tx_print(const std::any& value)
{
    if (!value.has_value())
    {
        tx_fn_write_line("none");
    }
    else if (value.type() == typeid(tx_int) || value.type() == typeid(double) ||
             value.type() == typeid(bool) || value.type() == typeid(std::string))
    {
        tx_fn_write_line(tx_to_string(value));
    }
    else
    {
        throw std::runtime_error("此数组元素类型暂不可直接打印");
    }
}

} // namespace tx_generated
