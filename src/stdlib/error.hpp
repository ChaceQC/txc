#pragma once

#include "common/error_kind.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace tx_generated
{

struct error_info
{
    tx::error_kind kind = tx::error_kind::none;
    std::string code;
    std::string message;
};

template<class value_type>
struct operation_result
{
    bool ok = false;
    value_type value{};
    error_info error;
};

class runtime_failure : public std::runtime_error
{
public:
    explicit runtime_failure(error_info error)
        : std::runtime_error(error.message), error_(std::move(error))
    {
    }

    [[nodiscard]] const error_info& error() const noexcept
    {
        return error_;
    }

private:
    error_info error_;
};

[[nodiscard]] inline const char* error_kind_name(tx::error_kind kind) noexcept
{
    switch (kind)
    {
    case tx::error_kind::none: return "";
    case tx::error_kind::parse: return "parse_error";
    case tx::error_kind::io: return "io_error";
    default: return "runtime_error";
    }
}

} // namespace tx_generated
