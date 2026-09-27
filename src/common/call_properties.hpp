#pragma once

#include <vector>

namespace tx
{

enum class argument_ownership
{
    held,
    borrowed
};

struct call_effects
{
    // 未登记的入口一律保守，不能把未知回调或用户析构当成叶子操作。
    bool allocates = true;
    bool runs_user_code = true;
    bool releases_user_objects = true;
    bool mutates_arguments = true;
    bool saves_arguments = true;

    [[nodiscard]] bool allows_borrow() const
    {
        return !runs_user_code && !releases_user_objects;
    }
};

struct call_properties
{
    call_effects effects;
    argument_ownership receiver = argument_ownership::held;
    std::vector<argument_ownership> arguments;
};

} // namespace tx
