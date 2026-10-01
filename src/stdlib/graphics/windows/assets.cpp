#include "stdlib/graphics/windows/assets.hpp"

#include <algorithm>

namespace tx_generated::graphics
{

void check_hr(app* owner, HRESULT status, const char* operation)
{
    if (FAILED(status))
    {
        platform_fail(owner, operation, status);
    }
}

void require_owned(owned_resource& state)
{
    require_thread(state.thread);
    const auto owner = state.owner.lock();
    if (owner)
    {
        drain(*owner);
    }
    if (!owner || !owner->open || state.closed)
    {
        fail("closed_resource", "图形资源或所属会话已经关闭");
    }
}

void same_owner(canvas& state, owned_resource& other)
{
    require_owned(other);
    if (state.owner.lock() != other.owner.lock())
    {
        fail("wrong_owner", "不能混用不同会话的图形资源");
    }
}

void close_owned(owned_resource& state) noexcept
{
    if (!state.closed)
    {
        if (const auto owner = state.owner.lock(); owner && owner->frame &&
            owner->frame->target_surface.get() == &state)
        {
            cancel_canvas(*owner->frame);
            owner->frame.reset();
        }
        state.release_native();
        state.closed = true;
    }
}

void collect_resources(app& state)
{
    for (const auto& resource : state.resources)
    {
        if (resource->abandoned.load(std::memory_order_acquire) && resource.use_count() == 1)
        {
            close_owned(*resource);
        }
    }
    std::erase_if(state.resources, [](const auto& resource)
    {
        return resource->closed;
    });
}

void image::release_native() noexcept
{
    if (const auto state = owner.lock())
    {
        state->image_bytes -= charge;
    }
    charge = 0;
    std::vector<BYTE>().swap(pixels);
}

void surface::release_native() noexcept
{
    pending.reset();
    bitmap.reset();
    if (const auto state = owner.lock())
    {
        state->image_bytes -= charge;
    }
    charge = 0;
}

void path::release_native() noexcept
{
    sink.reset();
    geometry.reset();
}

void brush::release_native() noexcept
{
    stops.clear();
}

void font::release_native() noexcept
{
    format.reset();
    factory.reset();
}

void text_layout::release_native() noexcept
{
    layout.reset();
    source.reset();
    factory.reset();
    format.reset();
    text.clear();
    offsets.clear();
}

std::wstring utf16(const std::string& text, std::size_t limit)
{
    if (text.size() > limit || text.find('\0') != std::string::npos)
    {
        fail("invalid_argument", "图形文本包含 NUL 或超过长度限制");
    }
    if (text.empty())
    {
        return {};
    }
    const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
        static_cast<int>(text.size()), nullptr, 0);
    if (!count)
    {
        fail("invalid_argument", "图形文本不是合法 UTF-8");
    }
    std::wstring result(count, 0);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
        static_cast<int>(text.size()), result.data(), count);
    return result;
}

std::string utf8(const std::wstring& text)
{
    if (text.empty())
    {
        return {};
    }
    const auto count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
        static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (!count)
    {
        fail("invalid_argument", "原生文本包含不完整的 Unicode 代理对");
    }
    std::string result(count, 0);
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
        static_cast<int>(text.size()), result.data(), count, nullptr, nullptr);
    return result;
}

std::vector<UINT32> unicode_offsets(const std::wstring& text)
{
    std::vector<UINT32> result;
    for (UINT32 index = 0; index < text.size(); ++index)
    {
        result.push_back(index);
        if (text[index] >= 0xd800 && text[index] <= 0xdbff)
        {
            if (++index >= text.size() || text[index] < 0xdc00 || text[index] > 0xdfff)
            {
                fail("invalid_argument", "Unicode 代理对不完整");
            }
        }
        else if (text[index] >= 0xdc00 && text[index] <= 0xdfff)
        {
            fail("invalid_argument", "Unicode 代理对不完整");
        }
    }
    result.push_back(static_cast<UINT32>(text.size()));
    return result;
}

std::size_t pixel_bytes(std::int64_t width, std::int64_t height)
{
    if (width <= 0 || height <= 0)
    {
        fail("invalid_argument", "图片尺寸必须为正");
    }
    if (width > pixel_limit || height > pixel_limit ||
        static_cast<std::uint64_t>(width) * height > image_limit / 4)
    {
        fail("resource_limit", "图片超过 16384 边长或 256 MiB 限额");
    }
    return static_cast<std::size_t>(width * height * 4);
}

void charge_pixels(app& owner, std::size_t count)
{
    if (count > image_budget - owner.image_bytes)
    {
        fail("resource_limit", "会话图片超过 512 MiB 预算");
    }
    owner.image_bytes += count;
}

com_ptr<IWICImagingFactory> wic_factory(app& owner)
{
    IWICImagingFactory* value = nullptr;
    check_hr(&owner, CoCreateInstance(CLSID_WICImagingFactory, nullptr,
        CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, reinterpret_cast<void**>(&value)),
        "创建 WIC 工厂");
    return com_ptr<IWICImagingFactory>(value);
}

std::shared_ptr<image> image_pixels(app& owner, UINT width, UINT height, std::vector<BYTE> pixels)
{
    auto result = own<image>(owner);
    const auto count = pixel_bytes(width, height);
    charge_pixels(owner, count);
    result->charge = count;
    result->width = width;
    result->height = height;
    result->pixels = std::move(pixels);
    return result;
}

} // namespace tx_generated::graphics
