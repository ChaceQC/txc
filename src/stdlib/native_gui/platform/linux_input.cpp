#include "stdlib/native_gui/platform/linux_window.hpp"
#include "stdlib/native_gui/platform/xim.hpp"

namespace tx::ui
{
namespace
{
std::string key_name(std::uint32_t symbol)
{
    if (symbol >= 0xffbe && symbol <= 0xffc9)
    {
        return "f" + std::to_string(symbol - 0xffbe + 1);
    }
    if (symbol >= 'a' && symbol <= 'z')
    {
        return std::string(1, static_cast<char>(symbol));
    }
    if (symbol >= 'A' && symbol <= 'Z')
    {
        return std::string(1, static_cast<char>(symbol - 'A' + 'a'));
    }
    if (symbol >= '0' && symbol <= '9')
    {
        return std::string(1, static_cast<char>(symbol));
    }
    switch (symbol)
    {
    case 0xff08: return "backspace";
    case 0xff09:
    case 0xfe20: return "tab";
    case 0xff0d: return "enter";
    case 0xff1b: return "escape";
    case 0xffff: return "delete";
    case 0xff50: return "home";
    case 0xff51: return "left";
    case 0xff52: return "up";
    case 0xff53: return "right";
    case 0xff54: return "down";
    case 0xff55: return "page_up";
    case 0xff56: return "page_down";
    case 0xff57: return "end";
    case 0xffc1: return "f4";
    case 0x20: return "space";
    default: return "unknown";
    }
}
}

std::optional<window_event> linux_window::translate(const x11_packet& message, bool from_ime, bool repeat)
{
    const auto type = message.data[0] & 0x7f;
    window_event event;
    if (type == 12)
    {
        event.kind = event_kind::redraw;
    }
    else if (type == 8)
    {
        event.kind = event_kind::pointer_left;
    }
    else if (type == 22)
    {
        event.kind = event_kind::resized;
        event.width = message.get16(20);
        event.height = message.get16(22);
    }
    else if (type == 9 || type == 10)
    {
        event.kind = type == 9 ? event_kind::focus_gained : event_kind::focus_lost;
        if (type == 10)
        {
            keys_down_.fill(false);
        }
        if (ime_)
        {
            ime_->active(type == 9);
        }
    }
    else if (type == 33 && message.get32(8) == protocols_atom_ && message.get32(12) == delete_atom_)
    {
        event.kind = event_kind::close_requested;
    }
    else if (type >= 2 && type <= 6)
    {
        const auto modifiers = message.get16(28);
        event.shift = (modifiers & 1) != 0;
        event.ctrl = (modifiers & 4) != 0;
        event.alt = (modifiers & 8) != 0;
        event.meta = (modifiers & 64) != 0;
        const unsigned detail = message.data[1];
        if (type <= 3)
        {
            if (!from_ime)
            {
                repeat = type == 2 && keys_down_[detail];
                keys_down_[detail] = type == 2;
            }
            event.kind = type == 2 ? event_kind::key_down : event_kind::key_up;
            event.repeat = repeat;
            if (detail < connection_.minimum_key || detail > connection_.maximum_key || !keys_per_code_)
            {
                return {};
            }
            const std::size_t base = (detail - connection_.minimum_key) * keys_per_code_;
            if (base >= keys_.size())
            {
                return {};
            }
            auto symbol = keys_[base];
            const bool capital = (modifiers & 2) && ((symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z'));
            if ((event.shift != capital) && keys_per_code_ > 1 && base + 1 < keys_.size() && keys_[base + 1])
            {
                symbol = keys_[base + 1];
            }
            event.key = key_name(symbol);
            if (!from_ime && ime_ && ime_->forward(message, symbol, repeat))
            {
                return {};
            }
            event.composing = ime_ && ime_->composing();
            const char32_t scalar = (symbol & 0xff000000) == 0x01000000 ? symbol & 0xffffff :
                symbol >= 32 && symbol <= 255 ? symbol : 0;
            if (type == 2 && scalar && scalar != 127 && scalar <= 0x10ffff && !event.ctrl && !event.alt && !event.meta)
            {
                window_event text{event_kind::text_input};
                text.text.push_back(scalar);
                pending_events_.push_back(std::move(text));
            }
        }
        else
        {
            if (!from_ime && ime_ && ime_->forward(message, 0, false))
            {
                return {};
            }
            event.x = static_cast<std::int16_t>(message.get16(24));
            event.y = static_cast<std::int16_t>(message.get16(26));
            if (detail >= 4 && detail <= 7 && type != 6)
            {
                if (type == 5)
                {
                    return {};
                }
                event.kind = event_kind::wheel;
                event.wheel = detail == 4 || detail == 6 ? 1 : -1;
            }
            else
            {
                event.kind = type == 6 ? event_kind::pointer_moved :
                    type == 4 ? event_kind::pointer_down : event_kind::pointer_up;
                event.button = type == 6 ? 0 : detail == 1 ? 1 : detail == 3 ? 2 : 3;
            }
        }
    }
    else if (type == 34 && (message.data[1] == 0 || message.data[1] == 1))
    {
        initialize_keyboard();
        return {};
    }
    else
    {
        return {};
    }
    return event;
}
}
