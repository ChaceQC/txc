#include "stdlib/graphics/windows/assets.hpp"

#include <algorithm>

namespace tx_generated::graphics
{
namespace
{

void decode_status(app& owner, HRESULT status)
{
    if (FAILED(status))
    {
        record_native_error(&owner, status);
        fail("decode_failed", "WIC 图片解码或颜色转换失败，原生错误码：" + std::to_string(status));
    }
}

unsigned orientation(IWICBitmapFrameDecode& frame)
{
    IWICMetadataQueryReader* raw = nullptr;
    if (FAILED(frame.GetMetadataQueryReader(&raw)))
    {
        return 1;
    }
    com_ptr<IWICMetadataQueryReader> reader(raw);
    PROPVARIANT value{};
    unsigned result = 1;
    if (SUCCEEDED(reader->GetMetadataByName(L"/app1/ifd/{ushort=274}", &value)) &&
        value.vt == VT_UI2 && value.uiVal >= 1 && value.uiVal <= 8)
    {
        result = value.uiVal;
    }
    PropVariantClear(&value);
    return result;
}

std::vector<BYTE> orient_pixels(const std::vector<BYTE>& input, UINT width, UINT height, unsigned mode)
{
    const auto out_width = mode >= 5 ? height : width;
    std::vector<BYTE> output(input.size());
    for (UINT y = 0; y < height; ++y)
    {
        for (UINT x = 0; x < width; ++x)
        {
            UINT dx = x, dy = y;
            switch (mode)
            {
            case 2: dx = width - 1 - x; break;
            case 3: dx = width - 1 - x; dy = height - 1 - y; break;
            case 4: dy = height - 1 - y; break;
            case 5: dx = y; dy = x; break;
            case 6: dx = height - 1 - y; dy = x; break;
            case 7: dx = height - 1 - y; dy = width - 1 - x; break;
            case 8: dx = y; dy = width - 1 - x; break;
            }
            std::copy_n(input.data() + (static_cast<std::size_t>(y) * width + x) * 4, 4,
                output.data() + (static_cast<std::size_t>(dy) * out_width + dx) * 4);
        }
    }
    return output;
}

} // namespace

std::shared_ptr<image> decode(app& owner, IWICBitmapDecoder& decoder)
{
    GUID format{};
    decode_status(owner, decoder.GetContainerFormat(&format));
    if (format != GUID_ContainerFormatPng && format != GUID_ContainerFormatJpeg &&
        format != GUID_ContainerFormatBmp)
    {
        fail("decode_failed", "仅支持 PNG、JPEG、BMP 图片");
    }
    IWICBitmapFrameDecode* raw = nullptr;
    decode_status(owner, decoder.GetFrame(0, &raw));
    com_ptr<IWICBitmapFrameDecode> frame(raw);
    UINT width = 0, height = 0;
    decode_status(owner, frame->GetSize(&width, &height));
    const auto count = pixel_bytes(width, height);
    if (count > image_budget - owner.image_bytes)
    {
        fail("resource_limit", "会话图片预算不足");
    }
    auto factory = wic_factory(owner);
    IWICFormatConverter* converter_raw = nullptr;
    decode_status(owner, factory->CreateFormatConverter(&converter_raw));
    com_ptr<IWICFormatConverter> converter(converter_raw);
    IWICBitmapSource* source = frame.get();
    com_ptr<IWICColorTransform> transform;
    com_ptr<IWICColorContext> source_context, target_context;
    UINT contexts = 0;
    const auto context_status = frame->GetColorContexts(0, nullptr, &contexts);
    if (SUCCEEDED(context_status) && contexts)
    {
        IWICColorContext* first = nullptr;
        IWICColorContext* second = nullptr;
        decode_status(owner, factory->CreateColorContext(&first));
        source_context.reset(first);
        decode_status(owner, factory->CreateColorContext(&second));
        target_context.reset(second);
        decode_status(owner, frame->GetColorContexts(1, &first, &contexts));
        decode_status(owner, second->InitializeFromExifColorSpace(1));
        IWICColorTransform* color = nullptr;
        decode_status(owner, factory->CreateColorTransformer(&color));
        transform.reset(color);
        decode_status(owner, color->Initialize(frame.get(), first, second, GUID_WICPixelFormat32bppBGRA));
        source = color;
    }
    decode_status(owner, converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom));
    std::vector<BYTE> pixels(count);
    decode_status(owner, converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(count), pixels.data()));
    const auto mode = orientation(*frame);
    if (mode != 1)
    {
        pixels = orient_pixels(pixels, width, height, mode);
        if (mode >= 5)
        {
            std::swap(width, height);
        }
    }
    return image_pixels(owner, width, height, std::move(pixels));
}

std::shared_ptr<image> load_image(app& owner, const std::string& file)
{
    auto factory = wic_factory(owner);
    IWICBitmapDecoder* decoder = nullptr;
    const auto name = utf16(file);
    decode_status(owner, factory->CreateDecoderFromFilename(name.c_str(), nullptr,
        GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder));
    com_ptr<IWICBitmapDecoder> value(decoder);
    return decode(owner, *value);
}

std::shared_ptr<image> decode_image(app& owner, const BYTE* bytes, std::size_t count)
{
    if (!count || count > image_limit)
    {
        fail("resource_limit", "图片编码数据为空或超过 256 MiB");
    }
    auto factory = wic_factory(owner);
    IWICStream* raw = nullptr;
    decode_status(owner, factory->CreateStream(&raw));
    com_ptr<IWICStream> stream(raw);
    decode_status(owner, stream->InitializeFromMemory(const_cast<BYTE*>(bytes), static_cast<DWORD>(count)));
    IWICBitmapDecoder* decoder = nullptr;
    decode_status(owner, factory->CreateDecoderFromStream(stream.get(), nullptr,
        WICDecodeMetadataCacheOnDemand, &decoder));
    com_ptr<IWICBitmapDecoder> value(decoder);
    return decode(owner, *value);
}

std::vector<BYTE> straight_rgba(const image& state)
{
    std::vector<BYTE> result(state.pixels.size());
    for (std::size_t index = 0; index < result.size(); index += 4)
    {
        const auto alpha = state.pixels[index + 3];
        for (unsigned channel = 0; channel < 3; ++channel)
        {
            result[index + channel] = alpha ? static_cast<BYTE>(std::min(255u,
                (state.pixels[index + 2 - channel] * 255u + alpha / 2) / alpha)) : 0;
        }
        result[index + 3] = alpha;
    }
    return result;
}

} // namespace tx_generated::graphics
