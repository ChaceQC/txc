#include "stdlib/native_gui/platform/linux_window.hpp"

#include <stdexcept>

namespace tx::ui
{
void linux_window::initialize_keyboard()
{
    // 只改变本连接的重复事件报告方式，不改变系统键盘设置。
    x11_packet extension;
    extension.u8(98);
    extension.u8(0);
    extension.u16(0);
    extension.u16(9);
    extension.u16(0);
    extension.text("XKEYBOARD");
    const auto registered = connection_.query(std::move(extension));
    if (!registered.data[8])
    {
        throw std::runtime_error("Linux GUI 需要 XKB 键盘扩展");
    }
    const auto opcode = registered.data[9];
    x11_packet version;
    version.u8(opcode);
    version.u8(0);
    version.u16(0);
    version.u16(1);
    version.u16(0);
    if (!connection_.query(std::move(version)).data[1])
    {
        throw std::runtime_error("XKB 1.0 不可用");
    }
    x11_packet flags;
    flags.u8(opcode);
    flags.u8(21);
    flags.u16(0);
    flags.u16(0x100);
    flags.u16(0);
    flags.u32(1);
    flags.u32(1);
    flags.u32(0);
    flags.u32(0);
    flags.u32(0);
    const auto repeat = connection_.query(std::move(flags));
    if (!(repeat.get32(8) & repeat.get32(12) & 1))
    {
        throw std::runtime_error("XKB 不支持可辨识自动重复");
    }
    x11_packet mapping;
    mapping.u8(101);
    mapping.u8(0);
    mapping.u16(0);
    // X11 GetKeyboardMapping：头部第二字节保留，first-keycode/count 位于偏移 4/5。
    mapping.u8(connection_.minimum_key);
    mapping.u8(connection_.maximum_key - connection_.minimum_key + 1);
    mapping.u16(0);
    const auto reply = connection_.query(std::move(mapping));
    keys_per_code_ = reply.data[1];
    if (!keys_per_code_ || reply.data.size() < 32 +
        (connection_.maximum_key - connection_.minimum_key + 1) * keys_per_code_ * 4)
    {
        throw std::runtime_error("系统键盘映射数据截断");
    }
    keys_.clear();
    keys_down_.fill(false);
    for (std::size_t offset = 32; offset + 4 <= reply.data.size(); offset += 4)
    {
        keys_.push_back(reply.get32(offset));
    }
}
}
