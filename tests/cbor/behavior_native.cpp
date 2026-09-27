#include "stdlib/cbor.hpp"
#include "stdlib/array.hpp"
#include "stdlib/dictionary.hpp"
#include "stdlib/error.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

using namespace tx_generated;

namespace
{

void expect(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("CBOR assertion failed");
    }
}

template<class operation>
void expect_error(std::string_view code, operation apply)
{
    try
    {
        apply();
    }
    catch (const runtime_failure& error)
    {
        expect(error.error().code == code);
        return;
    }
    throw std::runtime_error("expected CBOR error");
}

byte_value from_hex(std::string_view hex)
{
    return bytes_from_hex(hex);
}

format_source chunks(std::string data, std::size_t width)
{
    return [data = std::move(data), width, offset = std::size_t{0}]() mutable
    {
        const auto part = data.substr(offset, width);
        offset += part.size();
        return part;
    };
}

void check_vectors()
{
    const cbor_limits limits;
    expect(bytes_to_hex(cbor_encode(std::any{}, limits)) == "f6");
    expect(bytes_to_hex(cbor_encode(std::int64_t{-1}, limits)) == "20");
    expect(bytes_to_hex(cbor_encode(std::int64_t{INT64_MIN}, limits)) ==
           "3b7fffffffffffffff");
    expect(bytes_to_hex(cbor_encode(true, limits)) == "f5");
    expect(bytes_to_hex(cbor_encode(std::string("IETF"), limits)) ==
           "6449455446");
    expect(bytes_to_hex(cbor_encode(from_hex("01020304"), limits)) ==
           "4401020304");
    expect(bytes_to_hex(cbor_encode(1.5, limits)) == "f93e00");
    expect(bytes_to_hex(cbor_encode(2.0, limits)) == "f94000");
    expect(bytes_to_hex(cbor_encode(-0.0, limits)) == "f98000");
    expect(bytes_to_hex(cbor_encode(std::ldexp(1.0, -24), limits)) ==
           "f90001");
    expect(bytes_to_hex(cbor_encode(std::ldexp(1.0, -14), limits)) ==
           "f90400");
    expect(bytes_to_hex(cbor_encode(65504.0, limits)) == "f97bff");
    expect(bytes_to_hex(cbor_encode(65536.0, limits)) == "fa47800000");
    expect(bytes_to_hex(cbor_encode(std::numeric_limits<double>::quiet_NaN(),
                                    limits)) == "f97e00");

    tx_array array;
    array.push_back(std::int64_t{1});
    array.push_back(std::int64_t{2});
    array.push_back(std::int64_t{3});
    expect(bytes_to_hex(cbor_encode(array, limits)) == "83010203");
    tx_dict map;
    (void)map.emplace_back(std::string("b"), std::int64_t{2});
    (void)map.emplace_back(std::string("a"), std::int64_t{1});
    expect(bytes_to_hex(cbor_encode(map, limits)) == "a2616101616202");
    auto parsed = std::any_cast<tx_dict>(cbor_decode(from_hex("a2616101616202"), limits));
    expect(std::any_cast<std::int64_t>(*parsed.find_value(std::string_view("a"))) == 1);
    expect(std::any_cast<std::int64_t>(cbor_decode(
        from_hex("1b7fffffffffffffff"), limits)) == INT64_MAX);
    expect(std::any_cast<std::int64_t>(cbor_decode(
        from_hex("3b7fffffffffffffff"), limits)) == INT64_MIN);
    expect(std::signbit(std::any_cast<double>(cbor_decode(from_hex("f98000"), limits))));
}

void check_rejections()
{
    const cbor_limits limits;
    for (const auto hex : {"1817", "fa3fc00000", "a2616202616101"})
    {
        expect_error("non_canonical", [&]
        {
            (void)cbor_decode(from_hex(hex), limits);
        });
    }
    for (const auto hex : {"a2616101616102", "a2f600f600"})
    {
        expect_error("duplicate_key", [&]
        {
            (void)cbor_decode(from_hex(hex), limits);
        });
    }
    for (const auto hex : {"9f", "8201", "0102"})
    {
        expect_error("invalid_syntax", [&]
        {
            (void)cbor_decode(from_hex(hex), limits);
        });
    }
    expect_error("invalid_utf8", [&]
    {
        (void)cbor_decode(from_hex("62c0af"), limits);
    });
    expect_error("out_of_range", [&]
    {
        (void)cbor_decode(from_hex("1b8000000000000000"), limits);
    });
    expect_error("type_mismatch", [&]
    {
        (void)cbor_decode(from_hex("c100"), limits);
    });
    expect_error("depth_limit", [&]
    {
        (void)cbor_decode(from_hex("818180"), {100, 100, 1, 100});
    });
    expect_error("size_limit", [&]
    {
        (void)cbor_decode(from_hex("820102"), {100, 100, 128, 1});
    });
    expect_error("size_limit", [&]
    {
        (void)cbor_decode(from_hex("420102"), {100, 2, 128, 100});
    });
    expect_error("invalid_argument", [&]
    {
        (void)cbor_encode(std::int64_t{1}, {0, 100, 128, 100});
    });
    tx_array cyclic;
    cyclic.push_back(cyclic);
    expect_error("cyclic_value", [&]
    {
        (void)cbor_encode(cyclic, limits);
    });
    cyclic.clear();
}

void check_stream()
{
    const cbor_limits limits{100000, 4, 128, 20000};
    std::string data;
    auto writer = cbor_new_writer([&](std::string_view part)
    {
        expect(part.size() <= 4096);
        data.append(part);
    }, []
    {
    }, 20000, limits);
    for (std::int64_t index = 0; index < 20000; ++index)
    {
        cbor_write_value(writer, index);
    }
    cbor_finish(writer);
    cbor_finish(writer);
    expect_error("invalid_state", [&]
    {
        cbor_write_value(writer, std::int64_t{1});
    });
    auto reader = cbor_new_reader(chunks(data, 1), limits, {});
    for (std::int64_t index = 0; index < 20000; ++index)
    {
        auto item = cbor_next(reader);
        expect(item && std::any_cast<std::int64_t>(*item) == index);
    }
    expect(!cbor_next(reader));
    expect(!cbor_next(reader));
    cbor_close(reader);
    expect_error("invalid_state", [&]
    {
        (void)cbor_next(reader);
    });

    auto broken = cbor_new_reader(chunks(std::string("\x82\x01", 2), 1),
                                  limits, {});
    expect(cbor_next(broken).has_value());
    expect_error("invalid_syntax", [&]
    {
        (void)cbor_next(broken);
    });
    expect_error("invalid_state", [&]
    {
        (void)cbor_next(broken);
    });
    auto short_writer = cbor_new_writer([](std::string_view)
    {
    }, []
    {
    }, 2, limits);
    cbor_write_value(short_writer, std::int64_t{1});
    expect_error("invalid_state", [&]
    {
        cbor_finish(short_writer);
    });
    expect_error("invalid_state", [&]
    {
        cbor_write_value(short_writer, std::int64_t{2});
    });
}

} // namespace

int main()
{
    check_vectors();
    check_rejections();
    check_stream();
    std::cout << "CBOR_NATIVE_OK\n";
}
