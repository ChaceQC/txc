#include "xim_fixture.hpp"
#include "stdlib/native_gui/core/unicode.hpp"

#include <algorithm>
#include <iostream>

namespace xim_test
{
x11_packet key(unsigned type, unsigned detail)
{
    x11_packet packet;
    packet.u8(type);
    packet.u8(detail);
    packet.data.resize(32);
    return packet;
}

x11_packet commit(const std::string& text, unsigned flags)
{
    auto result = context();
    result.u16(flags);
    result.u16(text.size());
    result.text(text);
    return result;
}

x11_packet preedit(const std::string& text, unsigned first, unsigned removed, unsigned caret)
{
    auto result = context();
    result.u32(caret);
    result.u32(first);
    result.u32(removed);
    result.u32(0);
    result.u16(text.size());
    result.text(text);
    result.pad();
    const auto length = decode_utf8(text).scalars.size();
    result.u16(length * 4);
    result.u16(0);
    for (std::size_t i = 0; i < length; ++i)
    {
        result.u32(i == 0 ? 1 : 2);
    }
    return result;
}

void roundtrip(unsigned major, unsigned minor)
{
    fixture test(major, minor);
    const auto& spot = test.last_position;
    require(spot.get16(8) == 107 && spot.get16(12) == 109 && spot.get16(16) == 31 &&
        spot.get16(18) == 60, "nested candidate position");
    require(test.ime->forward(key(2), 'a', false), "key consumed by XIM");
    test.take(60);
    test.send(73, context());
    require(test.take(74).payload.get32(4) == 16384, "preedit start limit");
    test.consume();
    test.send(75, preedit("中文😀组合文本abcdef", 0, 0, 3));
    test.until([&]
    {
        return !test.output.empty();
    });
    require(test.output.back().text == U"中文😀组合文本abcdef" && test.output.back().caret == 3 &&
        test.output.back().feedback.front() == 1, "UTF-8 preedit and feedback");
    test.consume();
    test.send(75, preedit("字", 1, 2, 2));
    test.until([&]
    {
        return !test.output.empty();
    });
    require(test.output.back().text == U"中字组合文本abcdef", "scalar preedit replacement");
    test.consume();
    auto caret = context();
    caret.u32(4);
    caret.u32(10);
    caret.u32(1);
    test.send(76, caret);
    require(test.take(77).payload.get32(4) == 4, "preedit caret response");
    test.consume();
    test.send(63, commit("中文😀"));
    test.send(78, context());
    test.send(62, context());
    test.take(62);
    test.until([&]
    {
        return !test.ime->composing();
    });
    require(std::any_of(test.output.begin(), test.output.end(), [](const auto& value)
    {
        return value.kind == event_kind::text_input && value.text == U"中文😀";
    }), "commit preserved independently of preedit done");
    test.consume();
    test.ime->position({-5, 100, 1, 20});
    test.take(54);
    require(static_cast<std::int16_t>(test.last_position.get16(16)) == -5 &&
        test.last_position.get16(18) == 120, "updated candidate position");
    std::cout << "XIM transport " << major << '.' << minor << " / handshake / preedit / commit PASS\n";
}

void event_masks()
{
    fixture test;
    test.mask(0, 999);
    test.mask(0, 0);
    test.send(61, context());
    test.take(62);
    test.ime->forward(key(2), 'a', true);
    auto forwarded = test.take(60);
    test.send_pair(60, forwarded.payload, 62, context());
    test.until([&]
    {
        return !test.unfiltered.empty();
    });
    require(test.repeats.back(), "forwarded repeat preserved");
    std::cout << "XIM default / foreign IC masks / coalesced property / repeat PASS\n";
}

void focus_boundary()
{
    fixture test;
    test.ime->forward(key(2, 23), 0xff09, false);
    test.ime->forward(key(2), 'a', false);
    auto tab = test.take(60);
    test.send_pair(60, tab.payload, 62, context());
    test.until([&]
    {
        return !test.unfiltered.empty();
    });
    for (int i = 0; i < 8; ++i)
    {
        test.step();
    }
    require(test.forwarded == 1, "Tab must be delivered before next key");
    test.take(62);
    // 模拟应用消费 Tab 后切到新编辑器。旧响应只能被确认，不得写到新位置。
    test.ime->enable(false);
    test.ime->enable(true);
    test.consume();
    test.take(64);
    test.send(63, commit("旧输入"));
    test.take(62);
    require(test.output.empty(), "late commit discarded during reset");
    auto reset = context();
    reset.u16(9);
    reset.text("旧组合");
    test.send(65, reset);
    test.until([&]
    {
        return !test.output.empty();
    });
    require(test.forwarded == 1, "reset boundary delivered before new input");
    test.consume();
    test.take(60);
    test.send_pair(63, commit("新输入"), 62, context());
    test.take(62);
    require(test.output.size() == 1 && test.output.front().text == U"新输入", "new focus receives only new commit");
    std::cout << "XIM keyboard focus / reset / stale commit isolation PASS\n";
}

void malformed_preedit()
{
    fixture test;
    test.send(75, preedit("a", 100, 1, 0));
    test.until([&]
    {
        return !test.ime->ready();
    });
    require(std::any_of(test.output.begin(), test.output.end(), [](const auto& value)
    {
        return value.kind == event_kind::input_method_error;
    }), "bad preedit reports input error");
    require(!test.ime->forward(key(2), 'a', false), "keyboard fallback after IM failure");
    std::cout << "XIM malformed preedit / diagnostic / keyboard fallback PASS\n";
}

void trigger_keys()
{
    fixture test;
    test.mask(0);
    auto triggers = context(0);
    for (int list = 0; list < 2; ++list)
    {
        triggers.u32(12);
        triggers.u32(0x20);
        triggers.u32(4);
        triggers.u32(4);
    }
    test.send(34, triggers);
    test.send(61, context());
    test.take(62);
    auto trigger = key(2, 65);
    trigger.data[28] = 4;
    test.ime->forward(trigger, 0x20, false);
    const auto on = test.take(35);
    require(on.payload.get32(4) == 0 && on.payload.get32(8) == 0 &&
        on.payload.get32(12) == 3, "dynamic trigger on index and selected mask");
    test.mask(3);
    test.send(36, context());
    test.send(61, context());
    test.take(62);
    test.ime->forward(trigger, 0x20, false);
    require(test.take(35).payload.get32(4) == 1, "dynamic trigger off");
    test.mask(0);
    test.send(36, context());
    test.send(61, context());
    test.take(62);
    test.ime->forward(key(2), 'a', false);
    require(test.forwarded == 0 && test.unfiltered.size() == 1, "direct key after trigger off");
    std::cout << "XIM dynamic triggers / selected mask / filtering transition PASS\n";
}
}

int main()
{
    try
    {
        for (const auto [major, minor] : {std::pair{0u, 0u}, {0u, 1u}, {0u, 2u}, {1u, 0u}, {2u, 0u}, {2u, 1u}})
        {
            xim_test::roundtrip(major, minor);
        }
        xim_test::event_masks();
        xim_test::focus_boundary();
        xim_test::malformed_preedit();
        xim_test::trigger_keys();
    }
    catch (const std::exception& error)
    {
        std::cerr << "XIM verification failed: " << error.what() << '\n';
        return 1;
    }
}
