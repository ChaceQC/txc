#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace performance_equivalence
{

struct payload
{
    std::int64_t id = 0;
    std::string name;
};

[[nodiscard]] std::string serialize_payload(const payload& value);
[[nodiscard]] payload deserialize_payload(std::string_view text);
void check_serde_contract();

}
