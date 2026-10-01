#include "backend/cpp/graphics_result.hpp"
#include "stdlib/graphics/windows/assets.hpp"

using namespace tx_generated;
using namespace tx_generated::graphics;

extern "C" int txrt_graphics_save(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        save_state(require_canvas(value));
    });
}

extern "C" int txrt_graphics_restore(resource* value) noexcept
{
    return detail::invoke_leaf([&]
    {
        restore_state(require_canvas(value));
    });
}

namespace
{

void change_transform(resource* value, double a, double b, double c, double d, double x, double y, bool multiply)
{
    auto& state = require_canvas(value);
    auto matrix = D2D1::Matrix3x2F(checked_number(a), checked_number(b),
        checked_number(c), checked_number(d), checked_number(x), checked_number(y));
    if (multiply)
    {
        D2D1_MATRIX_3X2_F current{};
        state.target->GetTransform(&current);
        matrix = D2D1::Matrix3x2F::ReinterpretBaseType(&current)[0] * matrix;
        for (const auto number : {matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32})
        {
            checked_number(number);
        }
    }
    state.target->SetTransform(matrix);
}

} // namespace

extern "C" int txrt_graphics_set_transform(resource* value, double a, double b, double c, double d, double x, double y) noexcept
{
    return detail::invoke_leaf([&]
    {
        change_transform(value, a, b, c, d, x, y, false);
    });
}

extern "C" int txrt_graphics_transform(resource* value, double a, double b, double c, double d, double x, double y) noexcept
{
    return detail::invoke_leaf([&]
    {
        change_transform(value, a, b, c, d, x, y, true);
    });
}

extern "C" int txrt_graphics_clip_rect(resource* value, double x, double y, double width, double height) noexcept
{
    return detail::invoke_leaf([&]
    {
        clip_rectangle(require_canvas(value), {x, y, width, height});
    });
}

extern "C" int txrt_graphics_clip_path(resource* value, resource* path_value) noexcept
{
    return detail::invoke_leaf([&]
    {
        clip_geometry(require_canvas(value), asset<path>(path_value));
    });
}
