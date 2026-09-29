#include "stdlib/serde_direct.hpp"

#include <chrono>
#include <iostream>

using namespace tx_generated;

namespace
{

const serde_field fields[] = {
    {"id", 1, 0, {serde_kind::integer, "int", nullptr, nullptr, &tx_serde_integer}, {-1, 0, nullptr, 0}},
    {"name", 2, 1, {serde_kind::text, "str", nullptr, nullptr, &tx_serde_text}, {-1, 0, nullptr, 0}}
};
constexpr std::uint64_t order[] = {0, 1};
const serde_schema schema{"payload", "payload", 1, serde_unknown::reject,
    2, -1, "", fields, 2, order, order};

void bench(std::string name, const char* label)
{
    struct_fields values(2);
    values[0] = {"id", std::int64_t{7}};
    values[1] = {"name", std::move(name)};
    const std::any value = dynamic_struct(dynamic_struct_data{"payload", "payload", std::move(values)});
    for (const bool json : {true, false})
    {
        std::int64_t checksum = 0;
        const auto started = std::chrono::steady_clock::now();
        for (int index = 0; index < 5000; ++index)
        {
            std::any decoded;
            if (json)
            {
                auto wire = serde_serialize_json(&schema, value);
                gc_safepoint();
                decoded = serde_deserialize_json(&schema, wire);
            }
            else
            {
                auto wire = serde_serialize_cbor(&schema, value);
                gc_safepoint();
                decoded = serde_deserialize_cbor(&schema, wire);
            }
            gc_safepoint();
            const auto& result = std::any_cast<const dynamic_struct&>(decoded);
            checksum += std::any_cast<std::int64_t>(result->fields[0].value) +
                static_cast<std::int64_t>(std::any_cast<const std::string&>(result->fields[1].value).size());
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started).count();
        std::cout << (json ? "serde_json_" : "serde_cbor_") << label << '\n'
                  << elapsed << '\n' << checksum << '\n';
    }
}

} // namespace

int main()
{
    bench("x", "short");
    bench("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", "long");
    collect_cycles();
}
