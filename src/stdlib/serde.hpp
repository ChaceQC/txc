#pragma once

#include "backend/cpp/value_format.hpp"
#include "backend/cpp/serde_abi.hpp"
#include "stdlib/bytes.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace tx_generated
{

enum class serde_format { json, cbor };

using serde_active = std::unordered_set<const void*>;

std::any serde_default_value(const serde_default& value);
std::any serde_encode_value(const std::any& value, const serde_type& type,
    serde_format format, serde_active& active, std::size_t depth);
std::any serde_decode_value(const std::any& value, const serde_type& type,
    serde_format format, std::size_t depth);
std::any serde_encode_struct(const std::any& value,
    const serde_schema* schema, serde_format format,
    serde_active& active, std::size_t depth);
std::any serde_decode_struct(const std::any& value,
    const serde_schema* schema, serde_format format,
    std::size_t depth);
[[noreturn]] void serde_decode_error(const char* code, std::string message);
[[noreturn]] void serde_encode_error(const char* code, std::string message);

class serde_cycle_guard
{
public:
    serde_cycle_guard(serde_active& active, const void* identity)
        : active_(active), identity_(identity)
    {
        if (!active_.insert(identity_).second)
        {
            serde_encode_error("cyclic_value", "serde 不能编码循环引用");
        }
    }
    ~serde_cycle_guard()
    {
        active_.erase(identity_);
    }
    serde_cycle_guard(const serde_cycle_guard&) = delete;
    serde_cycle_guard& operator=(const serde_cycle_guard&) = delete;

private:
    serde_active& active_;
    const void* identity_;
};

std::string serde_serialize_json(const serde_schema* schema, const std::any& value);
std::any serde_deserialize_json(const serde_schema* schema, std::string_view text);
byte_value serde_serialize_cbor(const serde_schema* schema, const std::any& value);
std::any serde_deserialize_cbor(const serde_schema* schema, const byte_value& data);

} // namespace tx_generated
