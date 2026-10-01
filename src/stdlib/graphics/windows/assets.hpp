#pragma once

#include "stdlib/graphics/windows/state.hpp"
#include <dwrite.h>
#include <wincodec.h>

namespace tx_generated::graphics
{

constexpr std::size_t image_limit = 256 * 1024 * 1024;
constexpr std::size_t image_budget = 512 * 1024 * 1024;

struct owned_resource : resource
{
    explicit owned_resource(tx::graphics_kind kind) : resource(kind)
    {
    }
    std::weak_ptr<app> owner;
    std::weak_ptr<window> hosted_window;
    DWORD thread = GetCurrentThreadId();
    bool closed = false;
    virtual void release_native() noexcept = 0;
};

struct image : owned_resource
{
    image() : owned_resource(tx::graphics_kind::image)
    {
    }
    UINT width = 0, height = 0;
    std::vector<BYTE> pixels;
    std::size_t charge = 0;
    void release_native() noexcept override;
};

struct surface : owned_resource
{
    surface() : owned_resource(tx::graphics_kind::surface)
    {
    }
    UINT width = 0, height = 0;
    float dpi = 96;
    std::size_t charge = 0;
    com_ptr<IWICBitmap> bitmap;
    com_ptr<IWICBitmap> pending;
    void release_native() noexcept override;
};

struct path : owned_resource
{
    path() : owned_resource(tx::graphics_kind::path)
    {
    }
    com_ptr<ID2D1PathGeometry> geometry;
    com_ptr<ID2D1GeometrySink> sink;
    bool figure = false;
    bool frozen = false;
    unsigned segments = 0;
    void release_native() noexcept override;
};

struct brush : owned_resource
{
    brush() : owned_resource(tx::graphics_kind::brush)
    {
    }
    enum class mode
    {
        solid, linear, radial
    };
    mode kind = mode::solid;
    D2D1_COLOR_F color{};
    D2D1_POINT_2F first{}, second{};
    float radius_x = 0, radius_y = 0;
    std::vector<D2D1_GRADIENT_STOP> stops;
    void release_native() noexcept override;
};

struct font : owned_resource
{
    font() : owned_resource(tx::graphics_kind::font)
    {
    }
    com_ptr<IDWriteFactory> factory;
    com_ptr<IDWriteTextFormat> format;
    void release_native() noexcept override;
};

struct text_layout : owned_resource
{
    text_layout() : owned_resource(tx::graphics_kind::text_layout)
    {
    }
    std::shared_ptr<font> source;
    com_ptr<IDWriteFactory> factory;
    com_ptr<IDWriteTextFormat> format;
    float width = 0, height = 0;
    std::wstring text;
    std::vector<UINT32> offsets;
    com_ptr<IDWriteTextLayout> layout;
    void release_native() noexcept override;
};

void require_owned(owned_resource& state);
void same_owner(canvas& canvas, owned_resource& other);
void close_owned(owned_resource& state) noexcept;
void collect_resources(app& state);
template<class resource_type>
resource_type& asset(resource* value)
{
    auto& state = *static_cast<resource_type*>(value);
    require_owned(state);
    return state;
}
template<class resource_type>
std::shared_ptr<resource_type> own(app& owner)
{
    if (owner.resources.size() >= 65536)
    {
        fail("resource_limit", "会话资源超过 65536 个");
    }
    auto result = std::make_shared<resource_type>();
    result->abandoned.store(true, std::memory_order_release);
    result->owner = owner.shared_from_this();
    owner.resources.push_back(result);
    return result;
}
void check_hr(app* owner, HRESULT status, const char* operation);
std::wstring utf16(const std::string& text, std::size_t limit = 1024 * 1024);
std::string utf8(const std::wstring& text);
std::vector<UINT32> unicode_offsets(const std::wstring& text);
com_ptr<IWICImagingFactory> wic_factory(app& owner);
std::size_t pixel_bytes(std::int64_t width, std::int64_t height);
void charge_pixels(app& owner, std::size_t count);
std::shared_ptr<image> image_pixels(app& owner, UINT width, UINT height, std::vector<BYTE> pixels);
std::shared_ptr<image> decode(app& owner, IWICBitmapDecoder& decoder);
std::shared_ptr<image> load_image(app& owner, const std::string& file);
std::shared_ptr<image> decode_image(app& owner, const BYTE* bytes, std::size_t count);
std::shared_ptr<surface> create_surface(app& owner, std::int64_t width, std::int64_t height, double dpi);
std::shared_ptr<canvas> begin_surface_frame(surface& state);
std::shared_ptr<image> snapshot(surface& state);
void save_png(image& state, const std::string& file, bool overwrite);
std::vector<BYTE> straight_rgba(const image& state);
com_ptr<ID2D1Brush> native_brush(canvas& state, brush& value);
void pop_clips(canvas& state, std::size_t count) noexcept;
void save_state(canvas& state);
void restore_state(canvas& state);
void clip_rectangle(canvas& state, rectangle rect);
void clip_geometry(canvas& state, path& value);
void finish_surface_frame(canvas& state, HRESULT status);

} // namespace tx_generated::graphics
