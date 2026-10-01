#include "backend/cpp/graphics_result.hpp"
#include "stdlib/graphics/windows/assets.hpp"
#include "stdlib/vector.hpp"

using namespace tx_generated;
using namespace tx_generated::graphics;

extern "C" int txrt_graphics_load_image(resource* value, const void* name, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        return make_handle(load_image(require_app(value), detail::text_value(name)));
    });
}

extern "C" int txrt_graphics_decode_image(resource* value, const void* data, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        const auto& bytes = std::any_cast<const byte_value&>(*static_cast<const std::any*>(data));
        return make_handle(decode_image(require_app(value), bytes->data(), bytes->size()));
    });
}

extern "C" int txrt_graphics_image_from_rgba(resource* value, std::int64_t width, std::int64_t height, const void* data, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        auto& owner = require_app(value);
        const auto count = pixel_bytes(width, height);
        const auto& bytes = std::any_cast<const byte_value&>(*static_cast<const std::any*>(data));
        if (bytes->size() != count)
        {
            fail("invalid_argument", "RGBA8 像素长度必须等于宽乘高乘四");
        }
        if (count > image_budget - owner.image_bytes)
        {
            fail("resource_limit", "会话图片预算不足");
        }
        std::vector<BYTE> pixels(count);
        for (std::size_t index = 0; index < count; index += 4)
        {
            const auto alpha = (*bytes)[index + 3];
            for (unsigned channel = 0; channel < 3; ++channel)
            {
                pixels[index + channel] = static_cast<BYTE>(((*bytes)[index + 2 - channel] * alpha + 127) / 255);
            }
            pixels[index + 3] = alpha;
        }
        return make_handle(image_pixels(owner, static_cast<UINT>(width), static_cast<UINT>(height), std::move(pixels)));
    });
}

extern "C" int txrt_graphics_image_width(resource* value, std::int64_t* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = asset<image>(value).width;
    });
}

extern "C" int txrt_graphics_image_height(resource* value, std::int64_t* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = asset<image>(value).height;
    });
}

extern "C" int txrt_graphics_image_rgba(resource* value, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        byte_value bytes = std::make_shared<byte_storage>(straight_rgba(asset<image>(value)));
        *result = detail::make_handle<std::any>(std::move(bytes));
    });
}

extern "C" int txrt_graphics_draw_image(resource* value, resource* image_value,
    double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh,
    const void* interpolation, double opacity) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_canvas(value);
        auto& image = asset<graphics::image>(image_value);
        same_owner(state, image);
        const auto source = checked_rect({sx, sy, sw, sh});
        const auto destination = checked_rect({dx, dy, dw, dh});
        const auto alpha = checked_number(opacity);
        const auto& mode = detail::text_value(interpolation);
        if (sx < 0 || sy < 0 || sx + sw > image.width || sy + sh > image.height ||
            alpha < 0 || alpha > 1 || (mode != "nearest" && mode != "linear"))
        {
            fail("invalid_argument", "图片源矩形、透明度或插值方式非法");
        }
        ID2D1Bitmap* raw = nullptr;
        check_hr(state.owner.lock().get(), state.target->CreateBitmap(D2D1::SizeU(image.width, image.height),
            image.pixels.data(), image.width * 4,
            D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96),
            &raw), "上传图片像素");
        com_ptr<ID2D1Bitmap> bitmap(raw);
        state.target->DrawBitmap(bitmap.get(), destination, alpha,
            mode == "nearest" ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            source);
    });
}

extern "C" int txrt_graphics_create_surface(resource* value, std::int64_t width, std::int64_t height, double dpi, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        return make_handle(create_surface(require_app(value), width, height, dpi));
    });
}

extern "C" int txrt_graphics_begin_surface_frame(resource* value, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        return make_handle(begin_surface_frame(asset<surface>(value)));
    });
}

extern "C" int txrt_graphics_snapshot(resource* value, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        return make_handle(snapshot(asset<surface>(value)));
    });
}

extern "C" int txrt_graphics_save_png_image(resource* value, const void* file, bool overwrite, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        save_png(asset<image>(value), detail::text_value(file), overwrite);
        return {};
    });
}

extern "C" int txrt_graphics_save_png_surface(resource* value, const void* file, bool overwrite, const char* type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        auto image = snapshot(asset<surface>(value));
        try
        {
            save_png(*image, detail::text_value(file), overwrite);
        }
        catch (...)
        {
            close_owned(*image);
            throw;
        }
        close_owned(*image);
        return {};
    });
}

extern "C" int txrt_graphics_save_png_image_safe(resource* value, const void* file, const char* type, void** result) noexcept
{
    return txrt_graphics_save_png_image(value, file, false, type, result);
}

extern "C" int txrt_graphics_save_png_surface_safe(resource* value, const void* file, const char* type, void** result) noexcept
{
    return txrt_graphics_save_png_surface(value, file, false, type, result);
}
