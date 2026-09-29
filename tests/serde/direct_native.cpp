#include "stdlib/serde_direct.hpp"
#include "stdlib/vector.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace tx_generated;

namespace
{

const serde_type integer{serde_kind::integer, "int", nullptr, nullptr, &tx_serde_integer};
const serde_type text{serde_kind::text, "str", nullptr, nullptr, &tx_serde_text};
const serde_default required{-1, 0, nullptr, 0};
const serde_field payload_fields[] = {{"id", 1, 0, integer, required}, {"name", 2, 1, text, required}};
constexpr std::uint64_t order[] = {0, 1};
const serde_schema payload{"payload", "payload", 1, serde_unknown::reject,
    2, -1, "", payload_fields, 2, order, order};

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template<class operation>
error_info failure(operation run)
{
    try
    {
        run();
    }
    catch (const runtime_failure& error)
    {
        return error.error();
    }
    throw std::runtime_error("expected failure");
}

std::any direct_decode(const serde_schema& schema, std::string_view wire, bool json)
{
    serde_reader reader(json ? serde_format::json : serde_format::cbor, wire);
    auto result = serde_read_struct(reader, schema, {});
    reader.finish();
    return result;
}

std::string direct_encode(const serde_schema& schema, const std::any& value, bool json)
{
    serde_writer writer(json ? serde_format::json : serde_format::cbor);
    serde_write_struct(writer, value, schema, {});
    return writer.finish();
}

std::any make_payload(std::string name = "x")
{
    struct_fields fields(2);
    fields[0] = {"id", std::int64_t{7}};
    fields[1] = {"name", std::move(name)};
    return dynamic_struct(dynamic_struct_data{"payload", "payload", std::move(fields)});
}

void check_allocations()
{
    auto value = make_payload("中\n\t\"\\");
    auto& context = detail::current_runtime_context();
    for (const bool json : {true, false})
    {
        auto before = context.allocations_since_collection;
        auto wire = direct_encode(payload, value, json);
        require(context.allocations_since_collection == before, "encoding created a TX container");
        before = context.allocations_since_collection;
        auto decoded = direct_decode(payload, wire, json);
        require(context.allocations_since_collection - before == 1, "decode needs only the final struct");
        serde_active active;
        auto dynamic = serde_encode_struct(value, &payload,
            json ? serde_format::json : serde_format::cbor, active, 0);
        const auto legacy = json ? json_stringify(dynamic) : [&]
        {
            auto data = cbor_encode(dynamic, serde_cbor_limits);
            return std::string(data->begin(), data->end());
        }();
        require(wire == legacy, "wire bytes differ from legacy");
        require(direct_encode(payload, decoded, json) == wire, "roundtrip bytes differ");
        wire.assign(4096, 'x');
        require(std::any_cast<const std::string&>(std::any_cast<const dynamic_struct&>(decoded)
            ->fields[1].value) == "中\n\t\"\\", "decoded text borrowed input storage");
    }
}

void check_json_errors()
{
    const std::string inputs[] = {
        "", "null", "[]", "{}", "{\"$schema\":2,\"id\":7,\"name\":\"x\"}",
        "{\"$schema\":1,\"id\":7,\"id\":8,\"name\":\"x\"}",
        "{\"$schema\":1,\"id\":7,\"name\":\"x\",\"future\":1}",
        "{\"$schema\":1,\"id\":\"bad\",\"name\":\"x\"}",
        "{\"$schema\":1,\"id\":7}", "{\"$schema\":1,\"id\":7,\"name\":\"x\",}",
        "{\"$schema\":1,\"id\":7,\"name\":\"x\"} true",
        "{\"id\":false,\"$schema\":2}", "{\"$schema\":2,\"future\":[}",
        "{\"$schema\":1,\"id\":9223372036854775808,\"name\":\"x\"}",
        "{\"$schema\":1,\"id\":7.0,\"name\":\"x\"}",
        "{\"$schema\":1,\"id\":7,\"name\":\"\\ud800\"}",
        "{\"$schema\":1,\"id\":7,\"name\":\"\xff\"}",
        "{\"$schema\":1,\"id\":7,\"name\":\"x\",\"\\u0069d\":8}",
        "{\"$schema\":1,\"$schema\":1,\"id\":7,\"name\":\"x\"}"
    };
    for (const auto& input : inputs)
    {
        (void)failure([&]
        {
            (void)direct_decode(payload, input, true);
        });
        const auto legacy = failure([&]
        {
            (void)serde_decode_struct(json_parse_unique(input), &payload, serde_format::json, 0);
        });
        const auto current = failure([&]
        {
            (void)serde_deserialize_json(&payload, input);
        });
        require(current.kind == legacy.kind && current.code == legacy.code &&
            current.message == legacy.message, "JSON diagnostic priority changed");
    }
}

void check_unknown()
{
    auto ignored = payload;
    ignored.unknown = serde_unknown::ignore;
    const auto base = std::string("{\"$schema\":1,\"id\":7,\"name\":\"x\",\"future\":");
    auto& context = detail::current_runtime_context();
    const auto before = context.allocations_since_collection;
    auto value = direct_decode(ignored, base + "{\"a\":[1,true,null,{\"b\":\"ok\"}]}}", true);
    require(context.allocations_since_collection - before == 1, "ignored JSON built a dynamic tree");
    for (const auto& suffix : {"{\"a\":1,\"a\":2}}", "[1,]}", "true,\"future\":false}"})
    {
        (void)failure([&]
        {
            (void)direct_decode(ignored, base + suffix, true);
        });
    }
    tx_dict extras;
    (void)extras.emplace_back(std::string("a"), std::int64_t{1});
    tx_dict wire;
    (void)wire.emplace_back(std::int64_t{0}, std::int64_t{1});
    (void)wire.emplace_back(std::int64_t{1}, std::int64_t{7});
    (void)wire.emplace_back(std::int64_t{2}, std::string("x"));
    (void)wire.emplace_back(std::int64_t{9}, extras);
    auto binary = cbor_encode(wire, serde_cbor_limits);
    const std::string bytes(binary->begin(), binary->end());
    const auto start = context.allocations_since_collection;
    (void)direct_decode(ignored, bytes, false);
    require(context.allocations_since_collection - start == 1, "ignored CBOR built a dynamic tree");
    // 规范排序下 +0/-0 浮点键仍是 TX dict 的重复键。
    const std::string duplicate_zero = std::string("\xa4\x00\x01\x01\x07\x02\x61x\x09\xa2", 10) +
        std::string("\xf9\x00\x00\x01\xf9\x80\x00\x02", 8);
    require(failure([&]
    {
        (void)direct_decode(ignored, duplicate_zero, false);
    }).code == "duplicate_key", "ignored CBOR missed signed-zero duplicate");
}

std::weak_ptr<dynamic_struct_data> partial;

void check_preserve()
{
    auto preserved = payload;
    preserved.unknown = serde_unknown::preserve;
    preserved.field_count = 3;
    preserved.unknown_index = 2;
    preserved.unknown_name = "extras";
    for (const bool json : {true, false})
    {
        tx_dict unknown;
        // JSON 的未知键可排在版本字段之前；CBOR 跨 23/24 边界仍按规范顺序。
        const std::any key = json ? std::any(std::string("!first")) : std::any(std::int64_t{24});
        (void)unknown.emplace_back(key, std::string("未来"));
        struct_fields fields(3);
        fields[0] = {"id", std::int64_t{7}};
        fields[1] = {"name", std::string("x")};
        fields[2] = {"extras", unknown};
        const std::any value = dynamic_struct(dynamic_struct_data{"payload", "payload", std::move(fields)});
        auto wire = direct_encode(preserved, value, json);
        auto decoded = direct_decode(preserved, wire, json);
        require(direct_encode(preserved, decoded, json) == wire, "preserved roundtrip changed bytes");
        serde_active active;
        auto dynamic = serde_encode_struct(value, &preserved,
            json ? serde_format::json : serde_format::cbor, active, 0);
        if (json)
        {
            require(json_stringify(dynamic) == wire, "preserved JSON order changed");
        }
        else
        {
            auto old = cbor_encode(dynamic, serde_cbor_limits);
            require(std::string(old->begin(), old->end()) == wire, "preserved CBOR order changed");
        }
        const std::any collision = json ? std::any(std::string("id")) : std::any(std::int64_t{1});
        (void)unknown.emplace_back(collision, false);
        require(failure([&]
        {
            (void)direct_encode(preserved, value, json);
        }).code == "unknown_collision", "unknown collision skipped");
        (void)unknown.erase(collision);
        (void)unknown.emplace_back(key, unknown);
        require(failure([&]
        {
            (void)direct_encode(preserved, value, json);
        }).code == "cyclic_value", "unknown cycle skipped");
        unknown.clear();
    }
}

std::any observe_decode(serde_reader& reader, const serde_type& type, serde_depth depth)
{
    auto value = tx_serde_structure.decode(reader, type, depth);
    partial = std::any_cast<const dynamic_struct&>(value).data;
    return value;
}

void check_cleanup_and_limits()
{
    const serde_codec observed{tx_serde_structure.encode, observe_decode};
    const serde_field fields[] = {
        {"child", 1, 0, {serde_kind::structure, "payload", nullptr, &payload, &observed}, required},
        {"last", 2, 1, integer, required}
    };
    const serde_schema parent{"parent", "parent", 1, serde_unknown::reject,
        2, -1, "", fields, 2, order, order};
    const auto error = failure([&]
    {
        (void)direct_decode(parent,
            "{\"$schema\":1,\"child\":{\"$schema\":1,\"id\":7,\"name\":\"owned\"},\"last\":false}", true);
    });
    require(error.code == "type_mismatch" && partial.expired(), "partial nested struct leaked");
    auto ignored = payload;
    ignored.unknown = serde_unknown::ignore;
    const auto nested = std::string("{\"$schema\":1,\"id\":7,\"name\":\"x\",\"future\":") +
        std::string(129, '[') + "0" + std::string(129, ']') + "}";
    require(failure([&]
    {
        (void)direct_decode(ignored, nested, true);
    }).code == "depth_limit", "ignored depth limit skipped");
    require(failure([&]
    {
        (void)direct_encode(payload, make_payload(std::string(serde_byte_limit, 'x')), true);
    }).code == "size_limit", "output size limit skipped");
    require(failure([&]
    {
        (void)direct_decode(payload, std::string(serde_byte_limit + 1, 'x'), true);
    }).code == "size_limit", "input size limit skipped");
}

void check_cbor_errors()
{
    const std::string valid = direct_encode(payload, make_payload(), false);
    std::vector<std::string> inputs{valid + "x", valid.substr(0, valid.size() - 1),
        std::string("\xbf\xff", 2), std::string("\xb8\x03", 2),
        std::string("\xa3\x00\x01\x01\x07\x01\x07", 7),
        std::string("\xa3\x01\x07\x00\x01\x02\x61x", 8),
        std::string("\xa1\x00\x02", 3), std::string("\xba\x00\x0f\x42\x41", 5)};
    for (const auto& input : inputs)
    {
        (void)failure([&]
        {
            (void)direct_decode(payload, input, false);
        });
        const auto bytes = make_bytes({input.begin(), input.end()});
        const auto legacy = failure([&]
        {
            (void)serde_decode_struct(cbor_decode(bytes, serde_cbor_limits), &payload, serde_format::cbor, 0);
        });
        const auto current = failure([&]
        {
            (void)serde_deserialize_cbor(&payload, bytes);
        });
        require(current.kind == legacy.kind && current.code == legacy.code &&
            current.message == legacy.message, "CBOR diagnostic priority changed");
    }
}

} // namespace

int main()
{
    check_allocations();
    check_json_errors();
    check_unknown();
    check_preserve();
    check_cleanup_and_limits();
    check_cbor_errors();
    require(detail::current_runtime_context().newest_handle == nullptr, "temporary text roots leaked");
    collect_cycles();
    std::cout << "{\"status\":\"SERDE_DIRECT_NATIVE_OK\",\"fixed_encode_tx_allocations\":0,"
                 "\"fixed_decode_tx_allocations\":1,\"ignored_decode_tx_allocations\":1,"
                 "\"json_error_cases\":19,\"cbor_error_cases\":8,\"partial_struct_released\":true}\n";
}
