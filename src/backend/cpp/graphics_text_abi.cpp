#include "backend/cpp/graphics_result.hpp"
#include "stdlib/graphics/windows/assets.hpp"
#include "stdlib/vector.hpp"

#include <algorithm>

using namespace tx_generated;
using namespace tx_generated::graphics;

extern "C" int txrt_graphics_text_create_font(resource* value, const void* family, double size, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        auto& owner = require_app(value);
        const auto name = utf16(detail::text_value(family), 65536);
        const auto height = checked_width(size);
        if (name.empty())
        {
            fail("invalid_argument", "字体名称不能为空");
        }
        auto state = own<font>(owner);
        IDWriteFactory* factory = nullptr;
        check_hr(&owner, DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&factory)), "创建 DirectWrite 工厂");
        state->factory.reset(factory);
        IDWriteTextFormat* format = nullptr;
        check_hr(&owner, factory->CreateTextFormat(name.c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, height, L"zh-CN", &format), "创建字体");
        state->format.reset(format);
        return make_handle(state);
    });
}

extern "C" int txrt_graphics_text_create_layout(resource* value, const void* text, double width, double height, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        auto& source = asset<font>(value);
        auto wide = utf16(detail::text_value(text));
        const auto w = checked_width(width), h = checked_width(height);
        const auto owner = source.owner.lock();
        auto state = own<text_layout>(*owner);
        state->text = std::move(wide);
        state->offsets = unicode_offsets(state->text);
        state->width = w;
        state->height = h;
        source.factory->AddRef();
        state->factory.reset(source.factory.get());
        source.format->AddRef();
        state->format.reset(source.format.get());
        IDWriteTextLayout* layout = nullptr;
        check_hr(owner.get(), state->factory->CreateTextLayout(state->text.data(),
            static_cast<UINT32>(state->text.size()), state->format.get(), w, h, &layout), "创建文字布局");
        state->layout.reset(layout);
        return make_handle(state);
    });
}

extern "C" int txrt_graphics_text_set_layout_options(resource* value, const void* alignment, const void* paragraph, bool wrap, const void* locale) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = asset<text_layout>(value);
        const auto& horizontal = detail::text_value(alignment);
        const auto& vertical = detail::text_value(paragraph);
        const auto language = utf16(detail::text_value(locale), 128);
        if ((horizontal != "left" && horizontal != "center" && horizontal != "right") ||
            (vertical != "top" && vertical != "center" && vertical != "bottom") || language.empty())
        {
            fail("invalid_argument", "文字对齐或 locale 参数非法");
        }
        IDWriteTextLayout* raw = nullptr;
        const auto owner = state.owner.lock();
        check_hr(owner.get(), state.factory->CreateTextLayout(state.text.data(),
            static_cast<UINT32>(state.text.size()), state.format.get(), state.width, state.height, &raw), "更新文字布局");
        com_ptr<IDWriteTextLayout> next(raw);
        check_hr(owner.get(), raw->SetTextAlignment(horizontal == "left" ? DWRITE_TEXT_ALIGNMENT_LEADING :
            horizontal == "center" ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_TRAILING), "设置文字对齐");
        check_hr(owner.get(), raw->SetParagraphAlignment(vertical == "top" ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR :
            vertical == "center" ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER : DWRITE_PARAGRAPH_ALIGNMENT_FAR), "设置段落对齐");
        check_hr(owner.get(), raw->SetWordWrapping(wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP), "设置换行");
        check_hr(owner.get(), raw->SetLocaleName(language.c_str(), {0, static_cast<UINT32>(state.text.size())}), "设置文字语言");
        state.layout = std::move(next);
    });
}

extern "C" int txrt_graphics_text_draw_layout(resource* value, resource* layout, double x, double y, double r, double g, double b, double a) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        auto& text = asset<text_layout>(layout);
        same_owner(state, text);
        const auto origin = D2D1::Point2F(checked_number(x), checked_number(y));
        state.target->DrawTextLayout(origin, text.layout.get(), color_brush(state, {r, g, b, a}),
            D2D1_DRAW_TEXT_OPTIONS_CLIP);
    });
}

extern "C" int txrt_graphics_text_measure(resource* value, const record_type* type, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = asset<text_layout>(value);
        DWRITE_TEXT_METRICS metrics{};
        check_hr(state.owner.lock().get(), state.layout->GetMetrics(&metrics), "测量文字");
        dynamic_struct record{dynamic_struct_data(type)};
        record->write_field(0, static_cast<double>(metrics.left));
        record->write_field(1, static_cast<double>(metrics.top));
        record->write_field(2, static_cast<double>(metrics.widthIncludingTrailingWhitespace));
        record->write_field(3, static_cast<double>(metrics.height));
        record->write_field(4, static_cast<std::int64_t>(metrics.lineCount));
        *result = detail::make_handle<std::any>(std::move(record));
    });
}

extern "C" int txrt_graphics_text_hit_test(resource* value, double x, double y, const record_type* type, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = asset<text_layout>(value);
        DWRITE_HIT_TEST_METRICS metrics{};
        BOOL trailing = FALSE, inside = FALSE;
        check_hr(state.owner.lock().get(), state.layout->HitTestPoint(checked_number(x), checked_number(y),
            &trailing, &inside, &metrics), "文字命中测试");
        const auto utf_position = metrics.textPosition + (trailing ? metrics.length : 0);
        const auto index = std::lower_bound(state.offsets.begin(), state.offsets.end(), utf_position) - state.offsets.begin();
        dynamic_struct record{dynamic_struct_data(type)};
        record->write_field(0, static_cast<std::int64_t>(index));
        record->write_field(1, trailing != FALSE);
        record->write_field(2, inside != FALSE);
        record->write_field(3, static_cast<double>(metrics.left));
        record->write_field(4, static_cast<double>(metrics.top));
        record->write_field(5, static_cast<double>(metrics.width));
        record->write_field(6, static_cast<double>(metrics.height));
        *result = detail::make_handle<std::any>(std::move(record));
    });
}

extern "C" int txrt_graphics_text_selection_rects(resource* value, std::int64_t start, std::int64_t end, const record_type* type, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = asset<text_layout>(value);
        if (start < 0 || end < start || static_cast<std::uint64_t>(end) >= state.offsets.size())
        {
            fail("invalid_argument", "文字选择标量范围越界");
        }
        object_vector rectangles;
        UINT32 count = 0;
        if (start != end)
        {
            const auto first = state.offsets[start], length = state.offsets[end] - first;
            const auto status = state.layout->HitTestTextRange(first, length, 0, 0, nullptr, 0, &count);
            if (status != E_NOT_SUFFICIENT_BUFFER && FAILED(status))
            {
                check_hr(state.owner.lock().get(), status, "测量文字选择");
            }
            std::vector<DWRITE_HIT_TEST_METRICS> metrics(count);
            check_hr(state.owner.lock().get(), state.layout->HitTestTextRange(first, length, 0, 0,
                metrics.data(), count, &count), "读取文字选择");
            for (const auto& item : metrics)
            {
                dynamic_struct rect{dynamic_struct_data(type)};
                rect->write_field(0, static_cast<double>(item.left));
                rect->write_field(1, static_cast<double>(item.top));
                rect->write_field(2, static_cast<double>(item.width));
                rect->write_field(3, static_cast<double>(item.height));
                rectangles.data().values.push_back(std::move(rect));
            }
        }
        rectangles.data().refresh();
        *result = detail::make_handle<std::any>(std::move(rectangles));
    });
}
