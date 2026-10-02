#include "stdlib/native_gui/platform/xim.hpp"
#include "stdlib/native_gui/core/unicode.hpp"

#include <stdexcept>

namespace tx::ui
{
void x11_ime::message(unsigned opcode, const x11_packet& payload)
{
    if (session_message(opcode, payload))
    {
        return;
    }
    if (opcode == 20)
    {
        report("输入法协议错误，代码 " + std::to_string(payload.get16(6)));
        return;
    }
    if (opcode == 37 && im_ && payload.get16(0) == im_)
    {
        const auto context = payload.get16(2);
        // IC=0 只更新默认掩码，不能覆盖已经具有独立掩码的输入上下文。
        if (!context)
        {
            default_mask_ = payload.get32(4);
            if (!individual_mask_)
            {
                forward_mask_ = default_mask_;
            }
        }
        else if (ic_ && context == ic_)
        {
            forward_mask_ = payload.get32(4);
            individual_mask_ = true;
        }
        return;
    }
    if (opcode == 34 && (!im_ || payload.get16(0) == im_))
    {
        const auto read = [&](std::size_t offset, std::vector<trigger>& output)
        {
            const auto bytes = payload.get32(offset);
            if (bytes % 12 || bytes / 12 > 256 || payload.data.size() < offset + 4 + bytes)
            {
                throw std::runtime_error("输入法触发键列表长度非法");
            }
            output.clear();
            for (std::size_t i = 0; i < bytes; i += 12)
            {
                output.push_back({payload.get32(offset + 4 + i), payload.get32(offset + 8 + i), payload.get32(offset + 12 + i)});
            }
            return offset + 4 + bytes;
        };
        read(read(4, triggers_), off_triggers_);
        return;
    }
    if (!ready() || payload.data.size() < 4 || payload.get16(0) != im_ || payload.get16(2) != ic_)
    {
        return;
    }
    if (opcode == 61)
    {
        sync_reply();
    }
    else if (opcode == 62)
    {
        waiting_sync_ = false;
        flush_keys();
    }
    else if (opcode == 60)
    {
        if (payload.data.size() < 40)
        {
            throw std::runtime_error("输入法返回的按键包截断");
        }
        if (!cancelling_ && enabled_ && active_)
        {
            const x11_packet forwarded{{payload.data.begin() + 8, payload.data.begin() + 40}};
            unfiltered_(forwarded, sent_repeat_);
            delivery_boundary_ = true;
        }
        if (payload.get16(4) & 1)
        {
            sync_reply();
        }
    }
    else if (opcode == 73)
    {
        x11_packet reply;
        reply.u16(im_);
        reply.u16(ic_);
        reply.u32(16384);
        send(74, reply);
        if (!cancelling_ && enabled_ && active_)
        {
            preedit_.clear();
            feedback_.clear();
            caret_ = 0;
            composition(true);
        }
    }
    else if (opcode == 78)
    {
        composition(false);
    }
    else if (opcode == 75 && !cancelling_ && enabled_ && active_)
    {
        preedit_draw(payload);
    }
    else if (opcode == 76)
    {
        if (!cancelling_ && enabled_ && active_)
        {
            preedit_caret(payload);
        }
        else
        {
            x11_packet reply;
            reply.u16(im_);
            reply.u16(ic_);
            reply.u32(0);
            send(77, reply);
        }
    }
    else if (opcode == 63)
    {
        const auto flags = payload.get16(4);
        std::size_t text_offset = 0, length = 0;
        std::uint32_t symbol = 0;
        if ((flags & 6) == 2)
        {
            length = payload.get16(6);
            text_offset = 8;
        }
        else if (flags & 4)
        {
            symbol = payload.get32(8);
            if (flags & 2)
            {
                length = payload.get16(12);
                text_offset = 14;
            }
        }
        if (text_offset + length > payload.data.size())
        {
            throw std::runtime_error("输入法提交文本长度越界");
        }
        if (!cancelling_ && enabled_ && active_)
        {
            window_event result{event_kind::text_input};
            if (length)
            {
                result.text = decode_utf8(std::string(payload.data.begin() + text_offset,
                    payload.data.begin() + text_offset + length)).scalars;
            }
            else if ((symbol >= 32 && symbol <= 255) || (symbol & 0xff000000) == 0x01000000)
            {
                const auto scalar = symbol & 0xffffff;
                if (scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))
                {
                    throw std::runtime_error("输入法提交了非法 Unicode 标量");
                }
                result.text.push_back(scalar);
            }
            else if (symbol == 0xff0d || symbol == 0xff08 || symbol == 0xffff)
            {
                result.kind = event_kind::key_down;
                result.key = symbol == 0xff0d ? "enter" : symbol == 0xff08 ? "backspace" : "delete";
                result.repeat = sent_repeat_;
            }
            if (!result.text.empty() || !result.key.empty())
            {
                output_.push_back(std::move(result));
                delivery_boundary_ = true;
            }
        }
        if (flags & 1)
        {
            sync_reply();
        }
    }
}
}
