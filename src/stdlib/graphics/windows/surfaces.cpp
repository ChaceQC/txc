#include "stdlib/graphics/windows/assets.hpp"
#include <algorithm>

namespace tx_generated::graphics
{

std::shared_ptr<surface> create_surface(app& owner, std::int64_t width, std::int64_t height, double dpi)
{
    const auto count = pixel_bytes(width, height);
    const auto scale = checked_width(dpi);
    auto result = own<surface>(owner);
    // 同时预留已提交图和帧缓冲，失败/取消不破坏已有内容。
    charge_pixels(owner, count * 2);
    result->charge = count * 2;
    result->width = static_cast<UINT>(width);
    result->height = static_cast<UINT>(height);
    result->dpi = scale;
    auto factory = wic_factory(owner);
    IWICBitmap* raw = nullptr;
    check_hr(&owner, factory->CreateBitmap(result->width, result->height,
        GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &raw), "创建离屏位图");
    result->bitmap.reset(raw);
    check_hr(&owner, raw->SetResolution(dpi, dpi), "设置离屏 DPI");
    IWICBitmapLock* lock_raw = nullptr;
    check_hr(&owner, raw->Lock(nullptr, WICBitmapLockWrite, &lock_raw), "初始化离屏像素");
    com_ptr<IWICBitmapLock> lock(lock_raw);
    UINT bytes = 0;
    BYTE* pixels = nullptr;
    check_hr(&owner, lock->GetDataPointer(&bytes, &pixels), "读取离屏存储");
    std::fill_n(pixels, bytes, BYTE{0});
    return result;
}

std::shared_ptr<canvas> begin_surface_frame(surface& state)
{
    const auto owner = state.owner.lock();
    if (owner->frame)
    {
        fail("invalid_frame", "同一会话只能存在一个活动帧");
    }
    auto factory = wic_factory(*owner);
    IWICBitmap* raw = nullptr;
    check_hr(owner.get(), factory->CreateBitmap(state.width, state.height,
        GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &raw), "创建离屏帧");
    state.pending.reset(raw);
    auto result = std::make_shared<canvas>();
    result->owner = owner;
    for (const auto& resource : owner->resources)
    {
        if (resource.get() == &state)
        {
            result->target_surface = std::static_pointer_cast<surface>(resource);
            break;
        }
    }
    ID2D1RenderTarget* target = nullptr;
    auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        state.dpi, state.dpi);
    check_hr(owner.get(), owner->factory->CreateWicBitmapRenderTarget(raw, properties, &target), "创建离屏绘图目标");
    result->target.reset(target);
    ID2D1SolidColorBrush* brush = nullptr;
    check_hr(owner.get(), target->CreateSolidColorBrush(D2D1::ColorF(0, 0.0f), &brush), "创建离屏画刷");
    result->brush.reset(brush);
    target->BeginDraw();
    target->Clear(D2D1::ColorF(0, 0.0f));
    result->active = true;
    owner->frame = result;
    return result;
}

void finish_surface_frame(canvas& state, HRESULT status)
{
    auto& surface = *state.target_surface;
    state.brush.reset();
    state.target.reset();
    if (FAILED(status))
    {
        surface.pending.reset();
        check_hr(state.owner.lock().get(), status, "提交离屏绘制");
    }
    surface.bitmap = std::move(surface.pending);
}

std::shared_ptr<image> snapshot(surface& state)
{
    const auto owner = state.owner.lock();
    if (owner->frame && owner->frame->target_surface.get() == &state)
    {
        fail("invalid_frame", "不能快照正在绘制的离屏目标");
    }
    const auto count = pixel_bytes(state.width, state.height);
    if (count > image_budget - owner->image_bytes)
    {
        fail("resource_limit", "快照超过会话图片预算");
    }
    std::vector<BYTE> pixels(count);
    check_hr(owner.get(), state.bitmap->CopyPixels(nullptr, state.width * 4,
        static_cast<UINT>(count), pixels.data()), "读取离屏像素");
    return image_pixels(*owner, state.width, state.height, std::move(pixels));
}

} // namespace tx_generated::graphics
