#include "common/utf8.hpp"
#include "backend/cpp/parse_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/parse.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{

void expect(bool value)
{
    if (!value)
    {
        throw std::runtime_error("remaining performance contract failed");
    }
}

void compare_scan(std::string_view text)
{
    std::size_t count = 0;
    bool valid = true;
    for (std::size_t offset = 0; offset < text.size();)
    {
        const auto width = tx::utf8_width(text, offset);
        if (width == 0)
        {
            valid = false;
            break;
        }
        offset += width;
        ++count;
    }
    const auto scanned = tx::scan_utf8(text);
    expect(scanned.valid == valid && (!valid || scanned.length == count));
}

void utf8_boundaries()
{
    const std::array<std::string, 18> sequences{
        "\xc0\x80", "\xc1\xbf", "\xc2", "\xe0\x80\x80", "\xed\xa0\x80",
        "\xf0\x80\x80\x80", "\xf4\x90\x80\x80", "\xf5\x80\x80\x80",
        "\x80", "\xff", "\xc2\xc2\x80", "\xe0\xa0", "\xf0\x90\x80",
        "\xc2\x80", "\xe0\xa0\x80", "\xed\x9f\xbf", "\xf0\x90\x80\x80", "\xf4\x8f\xbf\xbf"};
    for (std::size_t offset = 0; offset < 65; ++offset)
    {
        for (const auto& sequence : sequences)
        {
            compare_scan(std::string(offset, 'a') + sequence);
            compare_scan(std::string(offset, 'a') + sequence + std::string(65, '\0'));
        }
    }
    // 覆盖所有双字节组合，并把它们放进批量路径的首部、内部与末尾。
    for (unsigned first = 0; first < 256; ++first)
    {
        for (unsigned second = 0; second < 256; ++second)
        {
            std::string pair{static_cast<char>(first), static_cast<char>(second)};
            compare_scan(pair);
            compare_scan(pair + std::string(40, 'a'));
            compare_scan(std::string(31, 'a') + pair);
        }
    }
    std::uint32_t random = 1234567;
    for (unsigned trial = 0; trial < 30000; ++trial)
    {
        std::string text(trial % 129, '\0');
        for (auto& byte : text)
        {
            random = random * 1664525U + 1013904223U;
            byte = static_cast<char>(random >> 24);
        }
        compare_scan(text);
    }
}

void all_unicode_scalars()
{
    std::string text;
    std::size_t count = 0;
    for (std::uint32_t point = 0; point <= 0x10ffff; ++point)
    {
        if (point >= 0xd800 && point <= 0xdfff)
        {
            continue;
        }
        if (point <= 0x7f)
        {
            text.push_back(static_cast<char>(point));
        }
        else
        {
            const unsigned width = point <= 0x7ff ? 2 : point <= 0xffff ? 3 : 4;
            text.push_back(static_cast<char>((0xffU << (8 - width)) | (point >> (6 * (width - 1)))));
            for (unsigned index = width - 1; index > 0; --index)
            {
                text.push_back(static_cast<char>(0x80 | ((point >> (6 * (index - 1))) & 0x3f)));
            }
        }
        ++count;
    }
    const auto result = tx::scan_utf8(text);
    expect(result.valid && result.length == count);
}

void explicit_parse_context()
{
    using namespace tx_generated;
    using namespace tx_generated::detail;
    auto* text = make_handle<std::string>(" +42 ");
    runtime_context context;
    bool ok = false;
    std::int64_t value = 0;
    std::int64_t error = -1;
    auto& thread = current_runtime_context();
    thread.last_error_kind = tx::error_kind::runtime;
    expect(txrt_parse_int_scalar_context(&context, text, 10, &ok, &value, &error) == 0);
    expect(ok && value == 42 && error == 0);
    context.last_error_kind = tx::error_kind::parse;
    expect(txrt_parse_int_scalar_context(&context, text, 10, &ok, &value, &error) ==
           static_cast<int>(tx::error_kind::parse));
    expect(context.last_error_kind == tx::error_kind::parse && thread.last_error_kind == tx::error_kind::runtime);
    context.last_error_kind = tx::error_kind::none;
    expect(txrt_parse_int_scalar_context(&context, text, 1, &ok, &value, &error) == 0);
    expect(!ok && value == 0 && error == static_cast<std::int64_t>(parse_error::invalid_base));
    double floating = 0;
    expect(txrt_parse_float_scalar_context(&context, text, &ok, &floating, &error) == 0);
    expect(ok && floating == 42);
    expect(txrt_parse_int_scalar(text, 10, &ok, &value, &error) == static_cast<int>(tx::error_kind::runtime));
    thread.last_error_kind = tx::error_kind::none;
    destroy_handle(text);
}

} // namespace

int main()
{
    utf8_boundaries();
    all_unicode_scalars();
    explicit_parse_context();
    std::cout << "REMAINING_NATIVE_OK\n";
}
