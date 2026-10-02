#include "backend/cpp/native_gui_result.hpp"
#include "backend/cpp/native_gui_abi.hpp"

#include <cmath>

using namespace tx_generated;
using graphics::resource;

namespace
{
void check_range(double minimum, double maximum, double value)
{
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || !std::isfinite(value) ||
        !std::isfinite(maximum - minimum) || minimum > maximum || value < minimum || value > maximum)
    {
        native_gui::fail("invalid_argument", "范围及当前值必须有限，且 minimum ≤ value ≤ maximum");
    }
}
}

extern "C" int txrt_native_gui_create_progress_bar(resource* parent, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create(native_gui::require_node(parent),
            tx::graphics_kind::native_progress_bar, ""));
    });
}

extern "C" int txrt_native_gui_set_progress(resource* control, double minimum, double maximum, double value) noexcept
{
    return native_gui::leaf_call([&]
    {
        check_range(minimum, maximum, value);
        auto& state = native_gui::require_node(control);
        state.minimum = minimum;
        state.maximum = maximum;
        state.value = value;
        ++state.revision;
        native_gui::dirty(state);
    });
}

extern "C" int txrt_native_gui_progress_value(resource* control, double* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_node(control).value;
    });
}

extern "C" int txrt_native_gui_set_indeterminate(resource* control, bool enabled) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        state.indeterminate = enabled;
        native_gui::dirty(state);
    });
}

extern "C" int txrt_native_gui_create_slider(resource* parent, double minimum, double maximum,
    double value, double step, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        check_range(minimum, maximum, value);
        if (minimum == maximum || !std::isfinite(step) || step <= 0 || step > maximum - minimum ||
            !std::isfinite((maximum - minimum) / step))
        {
            native_gui::fail("invalid_argument", "滑块范围必须非空，步长必须为范围内的有限正数");
        }
        const auto state = native_gui::create(native_gui::require_node(parent), tx::graphics_kind::native_slider, "");
        state->minimum = minimum;
        state->maximum = maximum;
        state->value = value;
        state->step = step;
        return graphics::make_handle(state);
    });
}

extern "C" int txrt_native_gui_set_slider_value(resource* control, double value) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        check_range(state.minimum, state.maximum, value);
        state.value = value;
        ++state.revision;
        native_gui::dirty(state);
    });
}

extern "C" int txrt_native_gui_slider_value(resource* control, double* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_node(control).value;
    });
}

extern "C" int txrt_native_gui_create_radio_button(resource* parent, const void* text, std::int64_t group,
    const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        const auto state = native_gui::create(native_gui::require_node(parent),
            tx::graphics_kind::native_radio_button, detail::text_value(text));
        state->radio_group = group;
        return graphics::make_handle(state);
    });
}

extern "C" int txrt_native_gui_set_radio_selected(resource* control, bool selected) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        if (selected)
        {
            if (const auto parent = state.parent.lock())
            {
                for (const auto& child : parent->children)
                {
                    if (child->kind == state.kind && child->radio_group == state.radio_group && child->checked)
                    {
                        child->checked = false;
                        ++child->revision;
                    }
                }
            }
        }
        state.checked = selected;
        ++state.revision;
        native_gui::dirty(state);
    });
}

extern "C" int txrt_native_gui_radio_selected(resource* control, bool* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_node(control).checked;
    });
}
