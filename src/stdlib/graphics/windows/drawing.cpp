#include "stdlib/graphics/windows/assets.hpp"

namespace tx_generated::graphics
{

com_ptr<ID2D1Brush> native_brush(canvas& state, brush& value)
{
    same_owner(state, value);
    const auto owner = state.owner.lock();
    if (value.kind == brush::mode::solid)
    {
        ID2D1SolidColorBrush* raw = nullptr;
        check_hr(owner.get(), state.target->CreateSolidColorBrush(value.color, &raw), "创建纯色画刷");
        return com_ptr<ID2D1Brush>(raw);
    }
    ID2D1GradientStopCollection* raw = nullptr;
    check_hr(owner.get(), state.target->CreateGradientStopCollection(value.stops.data(),
        static_cast<UINT>(value.stops.size()), &raw), "创建渐变色标");
    com_ptr<ID2D1GradientStopCollection> stops(raw);
    if (value.kind == brush::mode::linear)
    {
        ID2D1LinearGradientBrush* result = nullptr;
        check_hr(owner.get(), state.target->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(value.first, value.second), stops.get(), &result), "创建线性渐变");
        return com_ptr<ID2D1Brush>(result);
    }
    ID2D1RadialGradientBrush* result = nullptr;
    check_hr(owner.get(), state.target->CreateRadialGradientBrush(
        D2D1::RadialGradientBrushProperties(value.first, value.second, value.radius_x, value.radius_y),
        stops.get(), &result), "创建径向渐变");
    return com_ptr<ID2D1Brush>(result);
}

namespace
{

void push_clip(canvas& state, const canvas::clip_state& clip)
{
    state.target->SetTransform(clip.transform);
    if (clip.layer)
    {
        const auto parameters = D2D1::LayerParameters(D2D1::InfiniteRect(), clip.geometry.get());
        state.target->PushLayer(parameters, clip.layer.get());
    }
    else
    {
        state.target->PushAxisAlignedClip(clip.rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    }
}

void pop_native_clip(canvas& state, const canvas::clip_state& clip) noexcept
{
    if (clip.layer)
    {
        state.target->PopLayer();
    }
    else
    {
        state.target->PopAxisAlignedClip();
    }
}

} // namespace

void pop_clips(canvas& state, std::size_t count) noexcept
{
    while (state.clips.size() > count)
    {
        pop_native_clip(state, state.clips.back());
        state.clips.pop_back();
    }
}

void clear_canvas(canvas& state, rgba value)
{
    const auto color = checked_color(value);
    D2D1_MATRIX_3X2_F transform{};
    state.target->GetTransform(&transform);
    for (auto iterator = state.clips.rbegin(); iterator != state.clips.rend(); ++iterator)
    {
        pop_native_clip(state, *iterator);
    }
    state.target->SetTransform(D2D1::Matrix3x2F::Identity());
    state.target->Clear(color);
    for (const auto& clip : state.clips)
    {
        push_clip(state, clip);
    }
    state.target->SetTransform(transform);
}

void save_state(canvas& state)
{
    if (state.saved.size() >= 64)
    {
        fail("resource_limit", "绘图状态栈超过 64 层");
    }
    D2D1_MATRIX_3X2_F transform{};
    state.target->GetTransform(&transform);
    state.saved.push_back({transform, state.clips.size()});
}

void restore_state(canvas& state)
{
    if (state.saved.empty())
    {
        fail("invalid_frame", "绘图状态栈下溢");
    }
    const auto saved = state.saved.back();
    pop_clips(state, saved.clips);
    state.target->SetTransform(saved.transform);
    state.saved.pop_back();
}

void clip_rectangle(canvas& state, rectangle rect)
{
    if (state.clips.size() >= 64)
    {
        fail("resource_limit", "裁剪栈超过 64 层");
    }
    canvas::clip_state clip;
    clip.rect = checked_rect(rect);
    state.target->GetTransform(&clip.transform);
    state.clips.push_back(std::move(clip));
    push_clip(state, state.clips.back());
}

void clip_geometry(canvas& state, path& value)
{
    same_owner(state, value);
    if (!value.frozen)
    {
        fail("invalid_argument", "裁剪要求已冻结路径");
    }
    if (state.clips.size() >= 64)
    {
        fail("resource_limit", "裁剪栈超过 64 层");
    }
    canvas::clip_state clip;
    ID2D1Layer* raw = nullptr;
    check_hr(state.owner.lock().get(), state.target->CreateLayer(&raw), "创建路径裁剪层");
    clip.layer.reset(raw);
    value.geometry->AddRef();
    clip.geometry.reset(value.geometry.get());
    state.target->GetTransform(&clip.transform);
    state.clips.push_back(std::move(clip));
    push_clip(state, state.clips.back());
}

} // namespace tx_generated::graphics
