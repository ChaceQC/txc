#pragma once

#include "backend/cpp/value_format.hpp"
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

enum class serde_kind { integer, floating, boolean, text, bytes, vector,
                        option, structure };
enum class serde_format { json, cbor };
enum class serde_unknown { reject, ignore, preserve };

struct serde_schema;

struct serde_type
{
    serde_kind kind = serde_kind::integer;
    std::string name;
    std::shared_ptr<serde_type> element;
    std::shared_ptr<serde_schema> structure;
};

struct serde_field
{
    std::string name;
    std::int64_t number = 0;
    std::size_t index = 0;
    serde_type type;
    std::optional<std::any> default_value;
};

struct serde_schema
{
    std::string type_name;
    std::string display_name;
    std::int64_t version = 0;
    serde_unknown unknown = serde_unknown::reject;
    std::size_t field_count = 0;
    std::optional<std::size_t> unknown_index;
    std::string unknown_name;
    std::vector<serde_field> fields;
};

using serde_active = std::unordered_set<const void*>;

std::shared_ptr<serde_schema> serde_parse_schema(std::string_view source);
std::any serde_encode_value(const std::any& value, const serde_type& type,
    serde_format format, serde_active& active, std::size_t depth);
std::any serde_decode_value(const std::any& value, const serde_type& type,
    serde_format format, std::size_t depth);
std::any serde_encode_struct(const std::any& value,
    const std::shared_ptr<serde_schema>& schema, serde_format format,
    serde_active& active, std::size_t depth);
std::any serde_decode_struct(const std::any& value,
    const std::shared_ptr<serde_schema>& schema, serde_format format,
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

std::string serde_serialize_json(std::string_view schema, const std::any& value);
std::any serde_deserialize_json(std::string_view schema, std::string_view text);
byte_value serde_serialize_cbor(std::string_view schema, const std::any& value);
std::any serde_deserialize_cbor(std::string_view schema, const byte_value& data);

} // namespace tx_generated
