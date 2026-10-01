#include "backend/cpp/graphics_result.hpp"
#include "backend/cpp/vector_abi_internal.hpp"
#include "stdlib/graphics/windows/assets.hpp"

using namespace tx_generated;
using namespace tx_generated::graphics;

namespace
{

std::vector<D2D1_GRADIENT_STOP> read_stops(const void* value)
{
    const auto& source = detail::vector_value<std::any>(value).data().values;
    if (source.size() < 2 || source.size() > 256)
    {
        fail("invalid_argument", "渐变需要 2–256 个有序色标");
    }
    std::vector<D2D1_GRADIENT_STOP> result;
    double previous = -1;
    for (const auto& item : source)
    {
        const auto& record = std::any_cast<const dynamic_struct&>(item);
        const auto position = std::any_cast<double>(record->read_field(0));
        if (position < previous || position < 0 || position > 1)
        {
            fail("invalid_argument", "渐变位置必须有序且位于 [0,1]");
        }
        const auto color = checked_color({std::any_cast<double>(record->read_field(1)),
            std::any_cast<double>(record->read_field(2)), std::any_cast<double>(record->read_field(3)),
            std::any_cast<double>(record->read_field(4))});
        result.push_back({checked_number(position), color});
        previous = position;
    }
    return result;
}

} // namespace

extern "C" int txrt_graphics_solid_brush(resource* value, double r, double g, double b, double a, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        const auto color = checked_color({r, g, b, a});
        auto state = own<brush>(require_app(value));
        state->color = color;
        return make_handle(state);
    });
}

extern "C" int txrt_graphics_linear_gradient(resource* value, double x1, double y1, double x2, double y2, const void* stops, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        auto colors = read_stops(stops);
        const auto start = D2D1::Point2F(checked_number(x1), checked_number(y1));
        const auto end = D2D1::Point2F(checked_number(x2), checked_number(y2));
        auto state = own<brush>(require_app(value));
        state->kind = brush::mode::linear;
        state->first = start;
        state->second = end;
        state->stops = std::move(colors);
        return make_handle(state);
    });
}

extern "C" int txrt_graphics_radial_gradient(resource* value, double x, double y, double ox, double oy, double rx, double ry, const void* stops, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        auto colors = read_stops(stops);
        const auto center = D2D1::Point2F(checked_number(x), checked_number(y));
        const auto offset = D2D1::Point2F(checked_number(ox), checked_number(oy));
        const auto radius_x = checked_width(rx), radius_y = checked_width(ry);
        auto state = own<brush>(require_app(value));
        state->kind = brush::mode::radial;
        state->first = center;
        state->second = offset;
        state->radius_x = radius_x;
        state->radius_y = radius_y;
        state->stops = std::move(colors);
        return make_handle(state);
    });
}
