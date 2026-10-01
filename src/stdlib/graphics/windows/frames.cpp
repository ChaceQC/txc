#include "stdlib/graphics/windows/state.hpp"
#include "stdlib/gui/windows/state.hpp"
#include "stdlib/graphics/windows/assets.hpp"

namespace tx_generated::graphics
{
namespace
{

void drawing_error(window& state, HRESULT status, const char* operation)
{
    if (SUCCEEDED(status))
    {
        return;
    }
    const auto owner = state.owner.lock();
    record_native_error(owner.get(), status);
    if (status == static_cast<HRESULT>(D2DERR_RECREATE_TARGET))
    {
        state.target.reset();
        invalidate(state);
        fail("device_lost", "绘制目标已丢失，请在下一次重绘中重新提交完整帧");
    }
    platform_fail(owner.get(), operation, status);
}

} // namespace

void ensure_target(window& state)
{
    if (state.width > pixel_limit || state.height > pixel_limit)
    {
        fail("resource_limit", "客户区超出 16384 物理像素的边长限制");
    }
    if (state.target)
    {
        return;
    }
    const auto owner = state.owner.lock();
    auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_HARDWARE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        static_cast<float>(state.dpi), static_cast<float>(state.dpi));
    const auto hwnd_properties = D2D1::HwndRenderTargetProperties(state.hwnd,
        D2D1::SizeU(state.width, state.height), D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS);
    ID2D1HwndRenderTarget* target = nullptr;
    auto status = owner->factory->CreateHwndRenderTarget(properties, hwnd_properties, &target);
    state.software = false;
    if (FAILED(status))
    {
        properties.type = D2D1_RENDER_TARGET_TYPE_SOFTWARE;
        status = owner->factory->CreateHwndRenderTarget(properties, hwnd_properties, &target);
        state.software = true;
    }
    if (FAILED(status))
    {
        record_native_error(owner.get(), status);
        fail("backend_unavailable", "硬件和软件绘图目标均不可用，原生错误码：" + std::to_string(status));
    }
    state.target.reset(target);
}

std::shared_ptr<canvas> begin_frame(window& state)
{
    const auto owner = state.owner.lock();
    if (owner->frame)
    {
        fail("invalid_frame", "同一会话只能存在一个活动帧");
    }
    if (!state.visible || state.minimized || !state.width || !state.height)
    {
        return {};
    }
    gui::flush_layout(state);
    ensure_target(state);
    auto frame = std::make_shared<canvas>();
    frame->owner = owner;
    for (const auto& child : owner->windows)
    {
        if (child.get() == &state)
        {
            frame->target_window = child;
            break;
        }
    }
    ID2D1BitmapRenderTarget* target = nullptr;
    const auto status = state.target->CreateCompatibleRenderTarget(
        D2D1::SizeF(state.width * 96.0f / state.dpi, state.height * 96.0f / state.dpi),
        &target);
    drawing_error(state, status, "创建帧缓冲");
    frame->bitmap_target.reset(target);
    target->AddRef();
    frame->target.reset(target);
    ID2D1SolidColorBrush* brush = nullptr;
    const auto brush_status = target->CreateSolidColorBrush(D2D1::ColorF(0, 0.0f), &brush);
    drawing_error(state, brush_status, "创建颜色画刷");
    frame->brush.reset(brush);
    target->BeginDraw();
    target->Clear(D2D1::ColorF(0, 0.0f));
    frame->active = true;
    owner->frame = frame;
    return frame;
}

void end_frame(canvas& state)
{
    const auto owner = state.owner.lock();
    if (!state.saved.empty())
    {
        cancel_canvas(state);
        owner->frame.reset();
        fail("invalid_frame", "帧结束时 save/restore 栈不平衡");
    }
    pop_clips(state, 0);
    // 先关闭帧状态，即使提交失败，旧画布也不能再次提交。
    const auto status = state.target->EndDraw();
    state.active = false;
    owner->frame.reset();
    if (state.target_surface)
    {
        finish_surface_frame(state, status);
        return;
    }
    auto& target_window = *state.target_window;
    try
    {
        drawing_error(target_window, status, "结束帧绘制");
        ID2D1Bitmap* bitmap = nullptr;
        const auto bitmap_status = state.bitmap_target->GetBitmap(&bitmap);
        com_ptr<ID2D1Bitmap> image(bitmap);
        drawing_error(target_window, bitmap_status, "取得帧图像");
        target_window.target->BeginDraw();
        target_window.target->Clear(D2D1::ColorF(D2D1::ColorF::Black));
        target_window.target->DrawBitmap(image.get());
        const auto presented = target_window.target->EndDraw();
        drawing_error(target_window, presented, "提交窗口帧");
    }
    catch (...)
    {
        state.brush.reset();
        state.target.reset();
        state.bitmap_target.reset();
        throw;
    }
    state.brush.reset();
    state.target.reset();
    state.bitmap_target.reset();
}

ID2D1SolidColorBrush* color_brush(canvas& state, rgba value)
{
    state.brush->SetColor(checked_color(value));
    return state.brush.get();
}

} // namespace tx_generated::graphics
