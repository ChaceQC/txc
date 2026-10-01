#include "stdlib/graphics/windows/assets.hpp"

namespace tx_generated::graphics
{
namespace
{

void encode_png(image& state, const std::wstring& file)
{
    const auto owner = state.owner.lock();
    auto factory = wic_factory(*owner);
    IWICStream* stream_raw = nullptr;
    check_hr(owner.get(), factory->CreateStream(&stream_raw), "创建 PNG 输出流");
    com_ptr<IWICStream> stream(stream_raw);
    check_hr(owner.get(), stream->InitializeFromFilename(file.c_str(), GENERIC_WRITE), "打开 PNG 临时文件");
    IWICBitmapEncoder* encoder_raw = nullptr;
    check_hr(owner.get(), factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder_raw), "创建 PNG 编码器");
    com_ptr<IWICBitmapEncoder> encoder(encoder_raw);
    check_hr(owner.get(), encoder->Initialize(stream.get(), WICBitmapEncoderNoCache), "初始化 PNG 编码器");
    IWICBitmapFrameEncode* frame_raw = nullptr;
    check_hr(owner.get(), encoder->CreateNewFrame(&frame_raw, nullptr), "创建 PNG 帧");
    com_ptr<IWICBitmapFrameEncode> frame(frame_raw);
    check_hr(owner.get(), frame->Initialize(nullptr), "初始化 PNG 帧");
    check_hr(owner.get(), frame->SetSize(state.width, state.height), "设置 PNG 尺寸");
    auto format = GUID_WICPixelFormat32bppBGRA;
    check_hr(owner.get(), frame->SetPixelFormat(&format), "设置 PNG 像素格式");
    auto pixels = straight_rgba(state);
    if (format == GUID_WICPixelFormat32bppBGRA)
    {
        for (std::size_t index = 0; index < pixels.size(); index += 4)
        {
            std::swap(pixels[index], pixels[index + 2]);
        }
    }
    else if (format != GUID_WICPixelFormat32bppRGBA)
    {
        fail("backend_unavailable", "PNG 编码器不支持带 alpha 的 32 位像素");
    }
    check_hr(owner.get(), frame->WritePixels(state.height, state.width * 4,
        static_cast<UINT>(pixels.size()), pixels.data()), "编码 PNG 像素");
    check_hr(owner.get(), frame->Commit(), "提交 PNG 帧");
    check_hr(owner.get(), encoder->Commit(), "提交 PNG 编码");
}

} // namespace

void save_png(image& state, const std::string& file, bool overwrite)
{
    const auto owner = state.owner.lock();
    const auto target = utf16(file);
    if (target.empty())
    {
        fail("invalid_argument", "PNG 路径不能为空");
    }
    GUID id{};
    check_hr(owner.get(), CoCreateGuid(&id), "创建 PNG 临时文件名");
    wchar_t guid[40]{};
    StringFromGUID2(id, guid, 40);
    const auto temporary = target + L"." + guid + L".tmp";
    const auto handle = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        platform_fail(owner.get(), "创建 PNG 临时文件", GetLastError());
    }
    CloseHandle(handle);
    try
    {
        encode_png(state, temporary);
        if (!MoveFileExW(temporary.c_str(), target.c_str(),
            MOVEFILE_WRITE_THROUGH | (overwrite ? MOVEFILE_REPLACE_EXISTING : 0)))
        {
            platform_fail(owner.get(), "提交 PNG 文件", GetLastError());
        }
    }
    catch (...)
    {
        DeleteFileW(temporary.c_str());
        throw;
    }
}

} // namespace tx_generated::graphics
