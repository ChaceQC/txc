#include "backend/cpp/concurrency_value.hpp"
#include "stdlib/channel.hpp"
#include "stdlib/synchronization.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <typeinfo>

namespace tx_generated
{
namespace
{

bool name_matches(std::string_view type, std::string_view prefix,
                  std::string_view suffix) noexcept
{
    return type.size() == prefix.size() + suffix.size() + 1 &&
           type.starts_with(prefix) && type.back() == '>' &&
           type.substr(prefix.size(), suffix.size()) == suffix;
}

template<class value_type>
bool scalar_match(const std::any& value, std::string_view type,
                  std::string_view suffix) noexcept
{
    return (name_matches(type, "mutex<", suffix) &&
            value.type() == typeid(std::shared_ptr<mutex_state<value_type>>)) ||
           (name_matches(type, "mutex_guard<", suffix) &&
            value.type() == typeid(std::shared_ptr<mutex_guard_state<value_type>>)) ||
           (name_matches(type, "rw_lock<", suffix) &&
            value.type() == typeid(std::shared_ptr<rw_lock_state<value_type>>)) ||
           (name_matches(type, "rw_read_guard<", suffix) &&
            value.type() == typeid(std::shared_ptr<rw_guard_state<value_type, false>>)) ||
           (name_matches(type, "rw_write_guard<", suffix) &&
            value.type() == typeid(std::shared_ptr<rw_guard_state<value_type, true>>)) ||
           (name_matches(type, "channel<", suffix) &&
            value.type() == typeid(std::shared_ptr<channel_state<value_type>>));
}

template<class value_type>
bool scalar_type(const std::any& value) noexcept
{
    return value.type() == typeid(std::shared_ptr<mutex_state<value_type>>) ||
           value.type() == typeid(std::shared_ptr<mutex_guard_state<value_type>>) ||
           value.type() == typeid(std::shared_ptr<rw_lock_state<value_type>>) ||
           value.type() == typeid(std::shared_ptr<rw_guard_state<value_type, false>>) ||
           value.type() == typeid(std::shared_ptr<rw_guard_state<value_type, true>>) ||
           value.type() == typeid(std::shared_ptr<channel_state<value_type>>);
}

template<class value_type>
bool generic_channel_match(const std::any& value,
                           std::string_view type) noexcept
{
    if (!type.starts_with("channel<") || !type.ends_with('>'))
    {
        return false;
    }
    const auto* state = std::any_cast<
        std::shared_ptr<channel_state<value_type>>>(&value);
    return state && *state && (*state)->type_name == type;
}

bool generic_name_match(std::string_view actual, std::string_view requested,
    std::string_view requested_prefix, std::string_view owner_prefix) noexcept
{
    return requested.starts_with(requested_prefix) &&
        requested.ends_with('>') && actual.starts_with(owner_prefix) &&
        actual.ends_with('>') &&
        requested.substr(requested_prefix.size(),
            requested.size() - requested_prefix.size() - 1) ==
        actual.substr(owner_prefix.size(),
            actual.size() - owner_prefix.size() - 1);
}

template<class value_type>
bool generic_sync_match(const std::any& value,
                        std::string_view type) noexcept
{
    if (const auto* owner = std::any_cast<
            std::shared_ptr<mutex_state<value_type>>>(&value))
    {
        return *owner && (*owner)->type_name == type;
    }
    if (const auto* guard = std::any_cast<
            std::shared_ptr<mutex_guard_state<value_type>>>(&value))
    {
        return *guard && generic_name_match((*guard)->owner->type_name,
            type, "mutex_guard<", "mutex<");
    }
    if (const auto* owner = std::any_cast<
            std::shared_ptr<rw_lock_state<value_type>>>(&value))
    {
        return *owner && (*owner)->type_name == type;
    }
    if (const auto* guard = std::any_cast<
            std::shared_ptr<rw_guard_state<value_type, false>>>(&value))
    {
        return *guard && generic_name_match((*guard)->owner->type_name,
            type, "rw_read_guard<", "rw_lock<");
    }
    if (const auto* guard = std::any_cast<
            std::shared_ptr<rw_guard_state<value_type, true>>>(&value))
    {
        return *guard && generic_name_match((*guard)->owner->type_name,
            type, "rw_write_guard<", "rw_lock<");
    }
    return false;
}

} // namespace

bool concurrency_matches(const std::any& value, std::string_view type) noexcept
{
    return scalar_match<std::int64_t>(value, type, "int") ||
           scalar_match<double>(value, type, "float") ||
           scalar_match<bool>(value, type, "bool") ||
           generic_channel_match<std::string>(value, type) ||
           generic_channel_match<std::any>(value, type) ||
           generic_sync_match<std::string>(value, type) ||
           generic_sync_match<std::any>(value, type) ||
           (type == "atomic<int>" &&
            value.type() == typeid(std::shared_ptr<atomic_state<std::int64_t>>)) ||
           (type == "atomic<bool>" &&
            value.type() == typeid(std::shared_ptr<atomic_state<bool>>)) ||
           (type == "condition" &&
            value.type() == typeid(std::shared_ptr<condition_state>)) ||
           (type == "semaphore" &&
            value.type() == typeid(std::shared_ptr<semaphore_state>)) ||
           (type == "once" &&
            value.type() == typeid(std::shared_ptr<once_state>));
}

bool is_concurrency_value(const std::any& value) noexcept
{
    return scalar_type<std::int64_t>(value) || scalar_type<double>(value) ||
           scalar_type<bool>(value) || scalar_type<std::string>(value) ||
           scalar_type<std::any>(value) ||
           value.type() == typeid(std::shared_ptr<channel_state<std::string>>) ||
           value.type() == typeid(std::shared_ptr<channel_state<std::any>>) ||
           value.type() == typeid(std::shared_ptr<atomic_state<std::int64_t>>) ||
           value.type() == typeid(std::shared_ptr<atomic_state<bool>>) ||
           value.type() == typeid(std::shared_ptr<condition_state>) ||
           value.type() == typeid(std::shared_ptr<semaphore_state>) ||
           value.type() == typeid(std::shared_ptr<once_state>);
}

bool is_shareable_concurrency_value(const std::any& value) noexcept
{
    return value.type() == typeid(std::shared_ptr<mutex_state<std::int64_t>>) ||
           value.type() == typeid(std::shared_ptr<mutex_state<double>>) ||
           value.type() == typeid(std::shared_ptr<mutex_state<bool>>) ||
           value.type() == typeid(std::shared_ptr<mutex_state<std::string>>) ||
           value.type() == typeid(std::shared_ptr<mutex_state<std::any>>) ||
           value.type() == typeid(std::shared_ptr<rw_lock_state<std::int64_t>>) ||
           value.type() == typeid(std::shared_ptr<rw_lock_state<double>>) ||
           value.type() == typeid(std::shared_ptr<rw_lock_state<bool>>) ||
           value.type() == typeid(std::shared_ptr<rw_lock_state<std::string>>) ||
           value.type() == typeid(std::shared_ptr<rw_lock_state<std::any>>) ||
           value.type() == typeid(std::shared_ptr<channel_state<std::int64_t>>) ||
           value.type() == typeid(std::shared_ptr<channel_state<double>>) ||
           value.type() == typeid(std::shared_ptr<channel_state<bool>>) ||
           value.type() == typeid(std::shared_ptr<channel_state<std::string>>) ||
           value.type() == typeid(std::shared_ptr<channel_state<std::any>>) ||
           value.type() == typeid(std::shared_ptr<atomic_state<std::int64_t>>) ||
           value.type() == typeid(std::shared_ptr<atomic_state<bool>>) ||
           value.type() == typeid(std::shared_ptr<condition_state>) ||
           value.type() == typeid(std::shared_ptr<semaphore_state>) ||
           value.type() == typeid(std::shared_ptr<once_state>);
}

} // namespace tx_generated
