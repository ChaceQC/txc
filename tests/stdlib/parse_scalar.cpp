#include "stdlib/parse.hpp"

#include <array>
#include <atomic>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>

namespace
{

std::atomic<std::size_t> allocations = 0;

void expect(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("parse scalar contract failed");
    }
}

void integer_bases()
{
    using namespace tx_generated;
    constexpr std::array<std::int64_t, 7> values{0, 1, -1, 123456789, -123456789,
        std::numeric_limits<std::int64_t>::max(), std::numeric_limits<std::int64_t>::min()};
    for (int base = 2; base <= 36; ++base)
    {
        for (const auto value : values)
        {
            std::array<char, 100> buffer{};
            const auto formatted = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, base);
            const std::string text(buffer.data(), formatted.ptr);
            const auto parsed = parse_int_scalar(" \t" + text + "\r\n", base);
            expect(parsed.error == parse_error::none && parsed.value == value);
            const auto bad = parse_int_scalar(text + "!", base);
            expect(bad.error == parse_error::int_syntax && bad.value == 0);
        }
        std::array<char, 100> buffer{};
        const auto magnitude = std::uint64_t{1} << 63;
        const auto formatted = std::to_chars(buffer.data(), buffer.data() + buffer.size(), magnitude, base);
        const std::string text(buffer.data(), formatted.ptr);
        expect(parse_int_scalar(text, base).error == parse_error::int_range);
        expect(parse_int_scalar("-" + text, base).value == std::numeric_limits<std::int64_t>::min());
        expect(parse_int_scalar(text + "!", base).error == parse_error::int_syntax);
    }
}

void errors_and_allocations()
{
    using namespace tx_generated;
    const auto before = allocations.load();
    expect(parse_int_scalar(" \t", 1).error == parse_error::invalid_base);
    expect(parse_int_scalar(" \t").error == parse_error::empty_int);
    expect(parse_int_scalar("+").error == parse_error::int_sign);
    expect(parse_int_scalar("+-1").error == parse_error::int_syntax);
    expect(parse_int_scalar("+00042").value == 42);
    expect(parse_int_scalar("-9223372036854775809").error == parse_error::int_range);
    expect(parse_int_scalar("999999999999999999999999999999x").error == parse_error::int_syntax);
    expect(parse_float_scalar(" +1.25e2 ").value == 125.0);
    expect(parse_float_scalar("").error == parse_error::empty_float);
    expect(parse_float_scalar("+-1").error == parse_error::float_sign);
    expect(parse_float_scalar("1e9999x").error == parse_error::float_syntax);
    expect(parse_float_scalar("1e9999").error == parse_error::float_range);
    expect(parse_float_scalar("nan").error == parse_error::non_finite);
    expect(allocations.load() == before);
    const auto failure = try_parse_int("+");
    expect(!failure.ok && failure.value == 0 && failure.error.kind == tx::error_kind::parse &&
           failure.error.code == "invalid_syntax" && failure.error.message == "整数符号后需要数字");
    const auto success = try_parse_float("0");
    expect(success.ok && success.error.kind == tx::error_kind::none &&
           success.error.code.empty() && success.error.message.empty());
}

} // namespace

void* operator new(std::size_t size)
{
    allocations.fetch_add(1);
    if (void* value = std::malloc(size == 0 ? 1 : size))
    {
        return value;
    }
    throw std::bad_alloc();
}

void operator delete(void* value) noexcept
{
    std::free(value);
}

void operator delete(void* value, std::size_t) noexcept
{
    std::free(value);
}

int main()
{
    integer_bases();
    errors_and_allocations();
    std::cout << "PARSE_CORE_OK\n";
}
