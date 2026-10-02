#include "stdlib/native_gui/platform/xim.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

namespace tx::ui
{
namespace
{
void attribute_value(x11_packet& list, unsigned id, const x11_packet& value)
{
    list.u16(id);
    list.u16(value.data.size());
    list.bytes(value.data);
    list.pad();
}
}

void x11_ime::connected_reply(const x11_packet& payload)
{
    if (payload.get16(0) != 1)
    {
        throw std::runtime_error("输入法协议版本不是 XIM 1.x");
    }
    std::string locale = "en_US.UTF-8";
    for (const auto* variable : {"LC_ALL", "LC_CTYPE", "LANG"})
    {
        if (const auto* value = std::getenv(variable); value && *value)
        {
            locale = value;
            break;
        }
    }
    if (locale.size() > 255 || (locale.find("UTF-8") == std::string::npos && locale.find("utf8") == std::string::npos))
    {
        locale = "en_US.UTF-8";
    }
    x11_packet request;
    request.u8(locale.size());
    request.text(locale);
    phase_ = phase::open;
    send(30, request);
}

void x11_ime::open_reply(const x11_packet& payload)
{
    im_ = payload.get16(0);
    if (!im_)
    {
        throw std::runtime_error("输入法返回了无效 IM 编号");
    }
    const auto attributes = [&](std::size_t start, std::size_t count, auto& target)
    {
        if (start > payload.data.size() || count > payload.data.size() - start)
        {
            throw std::runtime_error("输入法属性目录越界");
        }
        const auto end = start + count;
        while (start < end)
        {
            if (end - start < 6)
            {
                throw std::runtime_error("输入法属性目录截断");
            }
            const auto id = payload.get16(start), type = payload.get16(start + 2), length = payload.get16(start + 4);
            if (length > end - start - 6 || length > 255)
            {
                throw std::runtime_error("输入法属性名称长度非法");
            }
            const std::string name(payload.data.begin() + start + 6, payload.data.begin() + start + 6 + length);
            target[name] = attribute{static_cast<std::uint16_t>(id), static_cast<std::uint16_t>(type)};
            start = (start + 6 + length + 3) & ~std::size_t(3);
        }
    };
    const auto im_length = payload.get16(2);
    attributes(4, im_length, im_attributes_);
    const auto ic_start = 4 + im_length;
    attributes(ic_start + 4, payload.get16(ic_start), ic_attributes_);
    const auto styles = im_attributes_.find("queryInputStyle");
    if (styles == im_attributes_.end() || styles->second.type != 10)
    {
        throw std::runtime_error("输入法没有提供输入样式查询");
    }
    x11_packet request;
    request.u16(im_);
    request.u16(2);
    request.u16(styles->second.id);
    request.u16(0);
    phase_ = phase::styles;
    send(44, request);
}

void x11_ime::style_reply(const x11_packet& payload)
{
    bool supported = false;
    const auto end = 4 + payload.get16(2);
    if (end > payload.data.size())
    {
        throw std::runtime_error("输入法样式属性长度越界");
    }
    for (std::size_t cursor = 4; cursor < end;)
    {
        const auto id = payload.get16(cursor), length = payload.get16(cursor + 2);
        if (end - cursor < 4 || length > end - cursor - 4)
        {
            throw std::runtime_error("输入法样式值长度越界");
        }
        if (id == im_attributes_.at("queryInputStyle").id)
        {
            const auto count = payload.get16(cursor + 4);
            if (length < 4 || count > (length - 4) / 4)
            {
                throw std::runtime_error("输入法样式列表长度非法");
            }
            for (unsigned index = 0; index < count; ++index)
            {
                const auto style = payload.get32(cursor + 8 + index * 4);
                if (style == 0x402 || style == 0x802)
                {
                    style_ = style;
                    supported = true;
                    break;
                }
            }
        }
        cursor += 4 + ((length + 3) & ~3u);
    }
    if (!supported)
    {
        throw std::runtime_error("输入法不支持由应用自行绘制预编辑文本");
    }
    negotiate_encoding();
}

void x11_ime::negotiate_encoding()
{
    x11_packet names;
    for (const std::string name : {"UTF8_STRING", "UTF-8"})
    {
        names.u8(name.size());
        names.text(name);
    }
    x11_packet request;
    request.u16(im_);
    request.u16(names.data.size());
    request.bytes(names.data);
    request.pad();
    request.u16(0);
    request.u16(0);
    phase_ = phase::encoding;
    send(38, request);
}

void x11_ime::encoding_reply(const x11_packet& payload)
{
    if (payload.get16(0) != im_ || payload.get16(2) != 0 || payload.get16(4) > 1)
    {
        throw std::runtime_error("输入法没有提供 UTF-8 传输编码");
    }
    create_context();
}

void x11_ime::create_context()
{
    x11_packet attributes;
    for (const auto& name : {"inputStyle", "clientWindow", "focusWindow"})
    {
        const auto found = ic_attributes_.find(name);
        if (found == ic_attributes_.end() || found->second.type != (std::string_view(name) == "inputStyle" ? 3 : 5))
        {
            throw std::runtime_error("输入法缺少必要的输入上下文属性");
        }
        x11_packet value;
        value.u32(std::string_view(name) == "inputStyle" ? style_ : window_);
        attribute_value(attributes, found->second.id, value);
    }
    x11_packet request;
    request.u16(im_);
    request.u16(attributes.data.size());
    request.bytes(attributes.data);
    phase_ = phase::create;
    send(50, request);
}

void x11_ime::created_reply(const x11_packet& payload)
{
    if (payload.get16(0) != im_ || !(ic_ = payload.get16(2)))
    {
        throw std::runtime_error("输入法返回了无效 IC 编号");
    }
    phase_ = phase::ready;
    focus();
    flush_keys();
}

void x11_ime::focus()
{
    if (ready())
    {
        x11_packet value;
        value.u16(im_);
        value.u16(ic_);
        send(enabled_ && active_ ? 58 : 59, value);
    }
}

void x11_ime::enable(bool value)
{
    if (enabled_ && !value)
    {
        cancel();
    }
    enabled_ = value;
    if (value && (phase_ == phase::idle || phase_ == phase::failed) &&
        std::chrono::steady_clock::now() - attempted_ > std::chrono::seconds(2))
    {
        start();
    }
    if (value && phase_ == phase::idle)
    {
        report("未找到可用的 XIM 输入法服务；当前使用直接键盘输入");
    }
    focus();
    flush_keys();
}

void x11_ime::active(bool value)
{
    if (!value)
    {
        cancel();
    }
    active_ = value;
    focus();
}

void x11_ime::position(rect bounds)
{
    if (!std::isfinite(bounds.x) || !std::isfinite(bounds.y + bounds.height))
    {
        throw std::invalid_argument("输入法候选位置必须有限");
    }
    if (bounds.x != position_.x || bounds.y != position_.y || bounds.height != position_.height)
    {
        position_ = bounds;
        position_dirty_ = true;
        flush_keys();
    }
}

void x11_ime::update_position()
{
    position_dirty_ = false;
    const auto preedit = ic_attributes_.find("preeditAttributes"), spot = ic_attributes_.find("spotLocation");
    if (preedit == ic_attributes_.end() || spot == ic_attributes_.end() ||
        preedit->second.type != 0x7fff || spot->second.type != 12)
    {
        return;
    }
    x11_packet point;
    point.u16(static_cast<std::uint16_t>(std::lround(std::clamp(position_.x, -32768.0, 32767.0))));
    point.u16(static_cast<std::uint16_t>(std::lround(std::clamp(position_.y + position_.height, -32768.0, 32767.0))));
    x11_packet nested;
    attribute_value(nested, spot->second.id, point);
    x11_packet attributes;
    attribute_value(attributes, preedit->second.id, nested);
    x11_packet request;
    request.u16(im_);
    request.u16(ic_);
    request.u16(attributes.data.size());
    request.u16(0);
    request.bytes(attributes.data);
    updating_position_ = true;
    send(54, request);
}

bool x11_ime::session_message(unsigned opcode, const x11_packet& payload)
{
    if ((opcode == 55 || opcode == 36 || opcode == 65) &&
        (!ready() || payload.get16(0) != im_ || payload.get16(2) != ic_))
    {
        return true;
    }
    if (opcode == 2 && phase_ == phase::connect)
    {
        connected_reply(payload);
    }
    else if (opcode == 31 && phase_ == phase::open)
    {
        open_reply(payload);
    }
    else if (opcode == 45 && phase_ == phase::styles)
    {
        style_reply(payload);
    }
    else if (opcode == 39 && phase_ == phase::encoding)
    {
        encoding_reply(payload);
    }
    else if (opcode == 51 && phase_ == phase::create)
    {
        created_reply(payload);
    }
    else if (opcode == 55)
    {
        updating_position_ = false;
        flush_keys();
    }
    else if (opcode == 36)
    {
        waiting_trigger_ = false;
        flush_keys();
    }
    else if (opcode == 65)
    {
        cancelling_ = reset_sent_ = false;
        preedit_.clear();
        composition(false);
        focus();
        flush_keys();
    }
    else
    {
        return false;
    }
    return true;
}
}
