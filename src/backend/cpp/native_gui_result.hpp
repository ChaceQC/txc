#pragma once

#include "backend/cpp/graphics_result.hpp"
#include "stdlib/native_gui/state.hpp"

namespace tx_generated::native_gui
{
template<class operation>
decltype(auto) guarded(operation&& run)
{
    try
    {
        return run();
    }
    catch (const runtime_failure&)
    {
        throw;
    }
    catch (const std::invalid_argument& error)
    {
        fail("invalid_argument", error.what());
    }
    catch (const std::out_of_range& error)
    {
        fail("invalid_argument", error.what());
    }
    catch (const std::length_error& error)
    {
        fail("resource_limit", error.what());
    }
    catch (const std::runtime_error& error)
    {
        fail("platform_error", error.what());
    }
}

template<class operation>
int result_call(const char* type, void** result, operation&& run) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return guarded(run);
    });
}

template<class operation>
int leaf_call(operation&& run) noexcept
{
    return detail::invoke_leaf([&]
    {
        guarded(run);
    });
}
}
