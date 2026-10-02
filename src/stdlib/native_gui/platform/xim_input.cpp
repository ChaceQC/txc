#include "stdlib/native_gui/platform/xim.hpp"
#include "stdlib/native_gui/core/unicode.hpp"

#include <algorithm>
#include <stdexcept>

namespace tx::ui
{
bool x11_ime::composing() const noexcept
{
    return preediting_;
}

void x11_ime::composition(bool active)
{
    preediting_ = active;
    if (!active)
    {
        preedit_.clear();
        feedback_.clear();
        caret_ = 0;
    }
    if (output_.size() >= 4096)
    {
        throw std::length_error("输入法事件队列已满");
    }
    window_event event{event_kind::composition};
    event.composing = active;
    event.text = preedit_;
    event.caret = caret_;
    event.feedback = feedback_;
    output_.push_back(std::move(event));
    delivery_boundary_ = true;
}

void x11_ime::cancel()
{
    if (ready())
    {
        cancelling_ = true;
        flush_keys();
    }
    composition(false);
}

void x11_ime::sync_reply()
{
    x11_packet reply;
    reply.u16(im_);
    reply.u16(ic_);
    send(62, reply);
}

bool x11_ime::process_key(const pending_key& key)
{
    const auto type = key.packet.data[0] & 0x7f;
    const auto& triggers = forward_mask_ ? off_triggers_ : triggers_;
    for (std::size_t index = 0; type == 2 && !key.repeat && index < triggers.size(); ++index)
    {
        const auto& trigger = triggers[index];
        if (key.symbol == trigger.symbol && (key.packet.get16(28) & trigger.mask) == trigger.modifier)
        {
            x11_packet request;
            request.u16(im_);
            request.u16(ic_);
            request.u32(forward_mask_ ? 1 : 0);
            request.u32(index);
            request.u32(selected_mask_);
            waiting_trigger_ = true;
            send(35, request);
            return true;
        }
    }
    const unsigned mask = type == 2 ? 1 : type == 3 ? 2 : type == 4 ? 4 : type == 5 ? 8 : 64;
    if (!(forward_mask_ & mask))
    {
        unfiltered_(key.packet, key.repeat);
        // Tab、快捷键及鼠标都可能改变焦点；先让应用消费事件，再处理后续输入。
        delivery_boundary_ = true;
        return true;
    }
    x11_packet request;
    request.u16(im_);
    request.u16(ic_);
    request.u16(3);
    request.u16(0);
    request.bytes(key.packet.data);
    waiting_sync_ = true;
    sent_repeat_ = key.repeat;
    send(60, request);
    return true;
}

void x11_ime::flush_keys()
{
    if (!ready() || waiting_sync_ || waiting_trigger_ || updating_position_ || reset_sent_ || delivery_boundary_)
    {
        return;
    }
    if (cancelling_)
    {
        x11_packet reset;
        reset.u16(im_);
        reset.u16(ic_);
        reset_sent_ = true;
        send(64, reset);
        return;
    }
    if (!active_)
    {
        keys_.clear();
        return;
    }
    if (!enabled_)
    {
        if (!keys_.empty())
        {
            const auto key = std::move(keys_.front());
            keys_.pop_front();
            unfiltered_(key.packet, key.repeat);
            delivery_boundary_ = true;
        }
        return;
    }
    if (position_dirty_)
    {
        update_position();
        if (updating_position_)
        {
            return;
        }
    }
    while (!keys_.empty())
    {
        auto key = std::move(keys_.front());
        keys_.pop_front();
        if (process_key(key))
        {
            break;
        }
    }
}

bool x11_ime::forward(const x11_packet& packet, std::uint32_t symbol, bool repeat)
{
    if (packet.data.size() != 32 || (packet.data[0] & 0x7f) < 2 || (packet.data[0] & 0x7f) > 6)
    {
        return false;
    }
    if (!enabled_ || !active_)
    {
        return false;
    }
    if ((phase_ == phase::idle || phase_ == phase::failed) &&
        std::chrono::steady_clock::now() - attempted_ > std::chrono::seconds(2))
    {
        start();
    }
    if (phase_ == phase::idle || phase_ == phase::failed)
    {
        return false;
    }
    if (!keys_.empty() && (packet.data[0] & 0x7f) == 6 && (keys_.back().packet.data[0] & 0x7f) == 6)
    {
        keys_.back() = {packet, symbol, repeat};
    }
    else
    {
        if (keys_.size() >= 256)
        {
            report("输入法按键队列已满");
            return false;
        }
        keys_.push_back({packet, symbol, repeat});
    }
    flush_keys();
    return true;
}

void x11_ime::preedit_draw(const x11_packet& payload)
{
    const auto first = payload.get32(8), removed = payload.get32(12), status = payload.get32(16);
    const auto length = payload.get16(20);
    if (payload.data.size() < 22 + length || first > preedit_.size() || removed > preedit_.size() - first)
    {
        throw std::runtime_error("输入法预编辑替换区间越界");
    }
    const auto inserted = (status & 1) ? std::u32string{} : decode_utf8(
        std::string(payload.data.begin() + 22, payload.data.begin() + 22 + length)).scalars;
    const auto feedback_start = (22 + length + 3) & ~3u;
    const auto feedback_bytes = payload.get16(feedback_start);
    if (feedback_bytes % 4 || payload.data.size() < feedback_start + 4 + feedback_bytes ||
        (!(status & 2) && feedback_bytes / 4 != inserted.size()))
    {
        throw std::runtime_error("输入法预编辑反馈长度非法");
    }
    std::vector<std::uint32_t> feedback(inserted.size());
    for (std::size_t index = 0; !(status & 2) && index < feedback.size(); ++index)
    {
        feedback[index] = payload.get32(feedback_start + 4 + index * 4);
    }
    if (preedit_.size() - removed + inserted.size() > 16384)
    {
        throw std::length_error("输入法预编辑超过 16384 个字符");
    }
    preedit_.replace(first, removed, inserted);
    feedback_.resize(feedback_.size() < first + removed ? first + removed : feedback_.size());
    feedback_.erase(feedback_.begin() + first, feedback_.begin() + first + removed);
    feedback_.insert(feedback_.begin() + first, feedback.begin(), feedback.end());
    caret_ = std::min<std::size_t>(payload.get32(4), preedit_.size());
    composition(true);
}

void x11_ime::preedit_caret(const x11_packet& payload)
{
    const auto direction = payload.get32(8);
    const auto boundaries = direction == 2 || direction == 3 ? word_boundaries(preedit_) : grapheme_boundaries(preedit_);
    if (direction == 10)
    {
        caret_ = std::min<std::size_t>(payload.get32(4), preedit_.size());
    }
    else if (direction == 0 || direction == 2)
    {
        const auto next = std::upper_bound(boundaries.begin(), boundaries.end(), caret_);
        caret_ = next == boundaries.end() ? preedit_.size() : *next;
    }
    else if (direction == 1 || direction == 3)
    {
        const auto previous = std::lower_bound(boundaries.begin(), boundaries.end(), caret_);
        caret_ = previous == boundaries.begin() ? 0 : *std::prev(previous);
    }
    else if (direction == 8 || direction == 9)
    {
        caret_ = direction == 8 ? 0 : preedit_.size();
    }
    x11_packet reply;
    reply.u16(im_);
    reply.u16(ic_);
    reply.u32(caret_);
    send(77, reply);
    composition(true);
}
}
