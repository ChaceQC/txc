#include "backend/cpp/gui_abi.hpp"
#include "backend/cpp/graphics_result.hpp"
#include "stdlib/gui/windows/state.hpp"

using namespace tx_generated;
using namespace tx_generated::gui;

namespace
{

void length_value(const record_type* type, const char* mode, double value, void** result)
{
    checked_length(mode, value);
    dynamic_struct item{dynamic_struct_data(type)};
    item->write_field(0, std::string(mode));
    item->write_field(1, value);
    *result = detail::make_handle<std::any>(std::move(item));
}

} // namespace

extern "C" int txrt_gui_fixed(double value, const record_type* result_type, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        length_value(result_type, "fixed", value, result);
    });
}

extern "C" int txrt_gui_auto_length(const record_type* result_type, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        length_value(result_type, "auto", 0, result);
    });
}

extern "C" int txrt_gui_stretch(double weight, const record_type* result_type, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        length_value(result_type, "stretch", weight, result);
    });
}
