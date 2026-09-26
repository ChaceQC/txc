#include "stdlib/encoding_incremental.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/error.hpp"
#include "stdlib/unicode_text.hpp"

#include <unicode/ucnv.h>
#include <unicode/ucnv_err.h>

#include <array>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tx_generated
{
namespace
{

constexpr std::size_t input_limit = 8 * 1024 * 1024;
constexpr std::size_t output_limit = 32 * 1024 * 1024;

[[noreturn]] void codec_error(tx::error_kind kind, const char* code,
                              const char* message)
{
    throw runtime_failure({kind, code, message});
}

const char* converter_name(detail::text_encoding encoding, bool decoder)
{
    using detail::text_encoding;
    switch (encoding)
    {
    case text_encoding::utf8:
    case text_encoding::utf8_sig: return "UTF-8";
    case text_encoding::utf16: return decoder ? "UTF-16" : "UTF-16LE";
    case text_encoding::utf16le: return "UTF-16LE";
    case text_encoding::utf16be: return "UTF-16BE";
    case text_encoding::utf32: return decoder ? "UTF-32" : "UTF-32LE";
    case text_encoding::utf32le: return "UTF-32LE";
    case text_encoding::utf32be: return "UTF-32BE";
    case text_encoding::gbk: return "GBK";
    case text_encoding::gb18030: return "GB18030";
    }
    codec_error(tx::error_kind::parse, "unknown_encoding", "未知增量编码名称");
}

detail::text_encoding selected_encoding(std::string_view name)
{
    try
    {
        return detail::parse_encoding(name);
    }
    catch (const std::runtime_error&)
    {
        codec_error(tx::error_kind::parse, "unknown_encoding", "未知增量编码名称");
    }
}

bool replacement_policy(std::string_view policy)
{
    if (policy == "strict")
    {
        return false;
    }
    if (policy == "replace")
    {
        return true;
    }
    codec_error(tx::error_kind::parse, "invalid_policy",
                "增量编解码策略只能是 strict 或 replace");
}

void to_callback(const void* context, UConverterToUnicodeArgs* arguments,
                 const char* units, std::int32_t length,
                 UConverterCallbackReason reason, UErrorCode* status)
{
    if (reason <= UCNV_IRREGULAR)
    {
        auto& count = *static_cast<std::int64_t*>(const_cast<void*>(context));
        if (count < std::numeric_limits<std::int64_t>::max())
        {
            ++count;
        }
    }
    UCNV_TO_U_CALLBACK_SUBSTITUTE(nullptr, arguments, units, length,
                                  reason, status);
}

void from_callback(const void* context, UConverterFromUnicodeArgs* arguments,
                   const UChar* units, std::int32_t length, UChar32 point,
                   UConverterCallbackReason reason, UErrorCode* status)
{
    if (reason <= UCNV_IRREGULAR)
    {
        auto& count = *static_cast<std::int64_t*>(const_cast<void*>(context));
        if (count < std::numeric_limits<std::int64_t>::max())
        {
            ++count;
        }
    }
    UCNV_FROM_U_CALLBACK_SUBSTITUTE(nullptr, arguments, units, length,
                                    point, reason, status);
}

std::string_view bom(detail::text_encoding encoding)
{
    using detail::text_encoding;
    switch (encoding)
    {
    case text_encoding::utf8_sig: return {"\xef\xbb\xbf", 3};
    case text_encoding::utf16:
    case text_encoding::utf16le: return {"\xff\xfe", 2};
    case text_encoding::utf16be: return {"\xfe\xff", 2};
    case text_encoding::utf32:
    case text_encoding::utf32le: return {"\xff\xfe\x00\x00", 4};
    case text_encoding::utf32be: return {"\x00\x00\xfe\xff", 4};
    default: return {};
    }
}

bool starts_with(std::string_view text, std::string_view prefix)
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

} // namespace

struct codec_state
{
    detail::text_encoding encoding;
    bool decoder;
    bool replace;
    bool ended = false;
    bool first = true;
    std::int64_t replacements = 0;
    std::string preamble;
    std::mutex mutex;
    std::unique_ptr<UConverter, decltype(&ucnv_close)> converter{nullptr, &ucnv_close};

    codec_state(detail::text_encoding selected, bool reading, bool replacing)
        : encoding(selected), decoder(reading), replace(replacing)
    {
        UErrorCode status = U_ZERO_ERROR;
        converter.reset(ucnv_open(converter_name(encoding, decoder), &status));
        if (U_FAILURE(status) || !converter)
        {
            codec_error(tx::error_kind::parse, "unknown_encoding",
                        "ICU 不支持指定增量编码");
        }
        if (decoder)
        {
            ucnv_setToUCallBack(converter.get(), replace ? to_callback :
                UCNV_TO_U_CALLBACK_STOP, replace ? &replacements : nullptr,
                nullptr, nullptr, &status);
        }
        else
        {
            ucnv_setFromUCallBack(converter.get(), replace ? from_callback :
                UCNV_FROM_U_CALLBACK_STOP, replace ? &replacements : nullptr,
                nullptr, nullptr, &status);
        }
        if (U_FAILURE(status))
        {
            codec_error(tx::error_kind::runtime, "operation_failed",
                        "无法设置 ICU 增量转换策略");
        }
    }
};

namespace
{

std::pair<std::string, bool> decoder_input(codec_state& state,
                                           const byte_value& data, bool eof)
{
    const auto* raw = reinterpret_cast<const char*>(data->data());
    std::string input = data->empty() ? std::string{} :
        std::string(raw, data->size());
    if (!state.first)
    {
        return {std::move(input), true};
    }
    state.preamble += input;
    const auto expected = bom(state.encoding);
    if (!expected.empty() && state.preamble.size() < expected.size() && !eof)
    {
        return {{}, false};
    }
    input = std::move(state.preamble);
    state.first = false;
    const auto is_utf16 = state.encoding == detail::text_encoding::utf16;
    const auto is_utf32 = state.encoding == detail::text_encoding::utf32;
    const auto opposite = is_utf16 || state.encoding == detail::text_encoding::utf16le
        ? std::string_view("\xfe\xff", 2)
        : state.encoding == detail::text_encoding::utf16be
        ? std::string_view("\xff\xfe", 2)
        : is_utf32 || state.encoding == detail::text_encoding::utf32le
        ? std::string_view("\x00\x00\xfe\xff", 4)
        : state.encoding == detail::text_encoding::utf32be
        ? std::string_view("\xff\xfe\x00\x00", 4) : std::string_view{};
    const bool own_bom = starts_with(input, expected) && !expected.empty();
    const bool other_bom = starts_with(input, opposite) && !opposite.empty();
    if ((is_utf16 || is_utf32) && !own_bom && !other_bom)
    {
        codec_error(tx::error_kind::parse, "invalid_encoding",
                    "UTF-16/UTF-32 增量解码缺少 BOM");
    }
    if (!is_utf16 && !is_utf32 && other_bom)
    {
        codec_error(tx::error_kind::parse, "invalid_encoding",
                    "增量解码的 BOM 与指定字节序不一致");
    }
    if (own_bom && !is_utf16 && !is_utf32)
    {
        input.erase(0, expected.size());
    }
    return {std::move(input), true};
}

std::string decode_bytes(codec_state& state, std::string_view input, bool eof)
{
    const char* source = input.data();
    const char* const limit = source + input.size();
    unicode_text::utf16_text wide;
    std::array<UChar, 8192> buffer{};
    bool pending = false;
    do
    {
        UChar* target = buffer.data();
        UErrorCode status = U_ZERO_ERROR;
        ucnv_toUnicode(state.converter.get(), &target, buffer.data() + buffer.size(),
                       &source, limit, nullptr, eof, &status);
        wide.append(buffer.data(), target);
        if (wide.size() * 2 > output_limit)
        {
            codec_error(tx::error_kind::runtime, "size_limit",
                        "增量解码输出超过 32 MiB");
        }
        pending = status == U_BUFFER_OVERFLOW_ERROR;
        if (!pending && U_FAILURE(status))
        {
            codec_error(tx::error_kind::parse, "invalid_encoding",
                        "增量解码遇到非法或残缺字符序列");
        }
    } while (pending || source < limit);
    auto result = unicode_text::to_utf8(wide.data(),
        static_cast<std::int32_t>(wide.size()));
    if (result.size() > output_limit)
    {
        codec_error(tx::error_kind::runtime, "size_limit",
                    "增量解码输出超过 32 MiB");
    }
    return result;
}

std::string encode_text(codec_state& state,
                        const unicode_text::utf16_text& wide, bool eof)
{
    const UChar* source = wide.data();
    const UChar* const limit = source + wide.size();
    std::string result;
    if (state.first)
    {
        state.first = false;
        if (state.encoding == detail::text_encoding::utf8_sig ||
            state.encoding == detail::text_encoding::utf16 ||
            state.encoding == detail::text_encoding::utf32)
        {
            result.append(bom(state.encoding));
        }
    }
    std::array<char, 8192> buffer{};
    bool pending = false;
    do
    {
        char* target = buffer.data();
        UErrorCode status = U_ZERO_ERROR;
        ucnv_fromUnicode(state.converter.get(), &target,
            buffer.data() + buffer.size(), &source, limit, nullptr, eof, &status);
        result.append(buffer.data(), target);
        if (result.size() > output_limit)
        {
            codec_error(tx::error_kind::runtime, "size_limit",
                        "增量编码输出超过 32 MiB");
        }
        pending = status == U_BUFFER_OVERFLOW_ERROR;
        if (!pending && U_FAILURE(status))
        {
            codec_error(tx::error_kind::parse, "unrepresentable_character",
                        "目标字符集无法表示输入文本");
        }
    } while (pending || source < limit);
    return result;
}

} // namespace

encoding_decoder new_decoder(std::string_view encoding, std::string_view policy)
{
    return {std::make_shared<codec_state>(selected_encoding(encoding), true,
                                          replacement_policy(policy))};
}

encoding_encoder new_encoder(std::string_view encoding, std::string_view policy)
{
    return {std::make_shared<codec_state>(selected_encoding(encoding), false,
                                          replacement_policy(policy))};
}

std::string decode_chunk(const encoding_decoder& source,
                         const byte_value& data, bool eof)
{
    auto& state = *source.state;
    std::lock_guard lock(state.mutex);
    if (state.ended)
    {
        codec_error(tx::error_kind::runtime, "closed_codec",
                    "增量解码器已经结束或失效");
    }
    if (data->size() > input_limit)
    {
        codec_error(tx::error_kind::runtime, "size_limit",
                    "单次增量解码输入超过 8 MiB");
    }
    try
    {
        auto [input, ready] = decoder_input(state, data, eof);
        if (!ready)
        {
            return {};
        }
        auto result = decode_bytes(state, input, eof);
        if (eof)
        {
            state.ended = true;
        }
        return result;
    }
    catch (...)
    {
        state.ended = true;
        throw;
    }
}

byte_value encode_chunk(const encoding_encoder& target,
                        std::string_view text, bool eof)
{
    if (text.size() > input_limit)
    {
        codec_error(tx::error_kind::runtime, "size_limit",
                    "单次增量编码输入超过 8 MiB");
    }
    const auto wide = unicode_text::from_utf8(text);
    auto& state = *target.state;
    std::lock_guard lock(state.mutex);
    if (state.ended)
    {
        codec_error(tx::error_kind::runtime, "closed_codec",
                    "增量编码器已经结束或失效");
    }
    try
    {
        auto result = encode_text(state, wide, eof);
        if (eof)
        {
            state.ended = true;
        }
        return make_bytes(std::vector<std::uint8_t>(result.begin(), result.end()));
    }
    catch (...)
    {
        state.ended = true;
        throw;
    }
}

std::int64_t decoder_replacements(const encoding_decoder& source)
{
    std::lock_guard lock(source.state->mutex);
    return source.state->replacements;
}

std::int64_t encoder_replacements(const encoding_encoder& target)
{
    std::lock_guard lock(target.state->mutex);
    return target.state->replacements;
}

} // namespace tx_generated
