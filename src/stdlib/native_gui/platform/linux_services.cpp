#include "stdlib/native_gui/platform/linux_window.hpp"
#include "stdlib/native_gui/dialogs.hpp"

namespace tx::ui
{
std::uint32_t linux_window::native_id() const noexcept
{
    return window_;
}

void linux_window::set_modal_owner(linux_window& owner, bool enabled)
{
    x11_packet value;
    value.u32(enabled ? owner.window_ : 0);
    property(connection_.atom("WM_TRANSIENT_FOR"), 33, 32, value.data);
    x11_packet modal;
    if (enabled)
    {
        modal.u32(connection_.atom("_NET_WM_STATE_MODAL"));
    }
    property(connection_.atom("_NET_WM_STATE"), 4, 32, modal.data);
}

point linux_window::screen_origin()
{
    x11_packet request;
    request.u8(40);
    request.u8(0);
    request.u16(0);
    request.u32(window_);
    request.u32(connection_.root);
    request.u16(0);
    request.u16(0);
    const auto reply = connection_.query(std::move(request));
    return {static_cast<double>(static_cast<std::int16_t>(reply.get16(12))),
        static_cast<double>(static_cast<std::int16_t>(reply.get16(14)))};
}
}

namespace tx_generated::native_gui
{
void platform_modal(window& dialog, window& owner, bool enabled)
{
    static_cast<tx::ui::linux_window&>(*dialog.host).set_modal_owner(
        static_cast<tx::ui::linux_window&>(*owner.host), enabled);
}

tx::ui::point screen_origin(window& owner)
{
    return static_cast<tx::ui::linux_window&>(*owner.host).screen_origin();
}
}
