#include "stdlib/ipc.hpp"

#include "stdlib/bytes.hpp"
#include "stdlib/cbor.hpp"
#include "stdlib/error.hpp"

#include <algorithm>
#include <limits>
#include <new>

namespace tx_generated
{
namespace
{

constexpr std::size_t header_size = 20;

std::uint64_t read_le(const std::vector<std::uint8_t>& input,
                      std::size_t start, std::size_t count) noexcept
{
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < count; ++index)
    {
        value |= static_cast<std::uint64_t>(input[start + index]) <<
            (8 * index);
    }
    return value;
}

void append_le(std::vector<std::uint8_t>& output,
               std::uint64_t value, std::size_t count)
{
    for (std::size_t index = 0; index < count; ++index)
    {
        output.push_back(static_cast<std::uint8_t>(value >> (8 * index)));
    }
}

ipc_message stopped_message(const ipc_stream& stream,
                            const ipc_transfer& transfer)
{
    ipc_message result;
    result.received = static_cast<std::int64_t>(stream->pending.size());
    switch (transfer.state)
    {
    case ipc_transfer_state::eof:
        result.state = stream->pending.empty() ? "eof" : "incomplete";
        ipc_close(stream);
        break;
    case ipc_transfer_state::timeout:
        result.state = "timeout";
        break;
    case ipc_transfer_state::cancelled:
        result.state = "cancelled";
        break;
    case ipc_transfer_state::error:
        result.state = "error";
        result.error_code = transfer.error_code;
        ipc_close(stream);
        break;
    case ipc_transfer_state::data:
        break;
    }
    return result;
}

ipc_transfer fill_to(const ipc_stream& stream, std::size_t target,
    ipc_clock::time_point deadline,
    const std::shared_ptr<cancellation_state>& token)
{
    while (stream->pending.size() < target)
    {
        std::uint8_t block[64 * 1024];
        const auto wanted = std::min<std::size_t>(
            target - stream->pending.size(), sizeof(block));
        const auto transfer = ipc_read_some(stream, block, wanted,
            deadline, token);
        if (transfer.count > 0)
        {
            stream->pending.insert(stream->pending.end(),
                block, block + transfer.count);
        }
        if (transfer.state != ipc_transfer_state::data)
        {
            return transfer;
        }
        if (transfer.count == 0)
        {
            return {ipc_transfer_state::error, 0,
                "ipc_zero_progress"};
        }
    }
    return {};
}

cbor_limits limits_for(std::size_t max_bytes)
{
    cbor_limits limits;
    limits.max_bytes = static_cast<std::int64_t>(max_bytes);
    limits.max_value_bytes = static_cast<std::int64_t>(max_bytes);
    return limits;
}

} // namespace

ipc_message ipc_receive(const ipc_stream& stream, std::int64_t timeout_ms,
                        const std::shared_ptr<cancellation_state>& token)
{
    const auto deadline = ipc_deadline(timeout_ms);
    std::lock_guard lock(stream->read_mutex);
    if (stream->closed)
    {
        return {"closed", 0, 0, {}, 0, {}};
    }
    const auto header = fill_to(stream, header_size, deadline, token);
    if (header.state != ipc_transfer_state::data)
    {
        return stopped_message(stream, header);
    }
    const auto& input = stream->pending;
    if (input[0] != 'T' || input[1] != 'X' ||
        input[2] != 'I' || input[3] != 'P')
    {
        const auto count = input.size();
        ipc_close(stream);
        return {"invalid_frame", 0, 0, {},
            static_cast<std::int64_t>(count), "ipc_magic"};
    }
    const auto version = static_cast<std::int64_t>(read_le(input, 4, 4));
    const auto session = static_cast<std::int64_t>(read_le(input, 8, 8));
    const auto length = static_cast<std::size_t>(read_le(input, 16, 4));
    if (version == 0 || length > stream->max_bytes)
    {
        ipc_close(stream);
        return {length > stream->max_bytes ? "too_large" : "invalid_frame",
            version, session, {}, static_cast<std::int64_t>(header_size),
            length > stream->max_bytes ? "ipc_message_limit" : "ipc_version"};
    }
    const auto body = fill_to(stream, header_size + length,
        deadline, token);
    if (body.state != ipc_transfer_state::data)
    {
        auto result = stopped_message(stream, body);
        result.version = version;
        result.session = session;
        return result;
    }
    std::vector<std::uint8_t> bytes(
        stream->pending.begin() + header_size,
        stream->pending.begin() + header_size + length);
    stream->pending.erase(stream->pending.begin(),
        stream->pending.begin() + header_size + length);
    ipc_message result{"data", version, session, {},
        static_cast<std::int64_t>(header_size + length), {}};
    try
    {
        result.value = cbor_decode(make_bytes(std::move(bytes)),
            limits_for(stream->max_bytes));
    }
    catch (const std::bad_alloc&)
    {
        throw;
    }
    catch (const std::exception&)
    {
        result.state = "invalid_cbor";
        result.error_code = "ipc_invalid_cbor";
        return result;
    }
    if (stream->has_peer_session && stream->peer_session != session)
    {
        result.state = "restarted";
    }
    stream->peer_session = session;
    stream->has_peer_session = true;
    return result;
}

ipc_send_result ipc_send(const ipc_stream& stream, std::int64_t version,
    const std::any& value, std::int64_t timeout_ms,
    const std::shared_ptr<cancellation_state>& token)
{
    const auto deadline = ipc_deadline(timeout_ms);
    if (version <= 0 ||
        static_cast<std::uint64_t>(version) >
            std::numeric_limits<std::uint32_t>::max())
    {
        throw runtime_failure({tx::error_kind::runtime,
            "out_of_range", "IPC 消息版本必须在 1..4294967295 内"});
    }
    auto payload = cbor_encode(value, limits_for(stream->max_bytes));
    if (payload->size() > stream->max_bytes)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "ipc_message_limit", "IPC 消息超过接收上限"});
    }
    std::vector<std::uint8_t> frame;
    frame.reserve(header_size + payload->size());
    frame.insert(frame.end(), {'T', 'X', 'I', 'P'});
    append_le(frame, static_cast<std::uint64_t>(version), 4);
    append_le(frame, static_cast<std::uint64_t>(stream->local_session), 8);
    append_le(frame, payload->size(), 4);
    frame.insert(frame.end(), payload->begin(), payload->end());
    std::lock_guard lock(stream->write_mutex);
    if (stream->closed)
    {
        return {"closed", 0, {}};
    }
    std::size_t sent = 0;
    while (sent < frame.size())
    {
        const auto transfer = ipc_write_some(stream, frame.data() + sent,
            frame.size() - sent, deadline, token);
        sent += transfer.count;
        if (transfer.state == ipc_transfer_state::data &&
            transfer.count > 0)
        {
            continue;
        }
        if (transfer.state == ipc_transfer_state::timeout &&
            ipc_clock::now() < deadline && !ipc_cancelled(token))
        {
            continue;
        }
        const auto cause = transfer.state == ipc_transfer_state::cancelled
            ? "cancelled" : transfer.state == ipc_transfer_state::timeout
            ? "timeout" : transfer.state == ipc_transfer_state::eof
            ? "closed" : "ipc_write_failed";
        const bool partial = sent > 0;
        if (partial || transfer.state == ipc_transfer_state::eof ||
            transfer.state == ipc_transfer_state::error)
        {
            ipc_close(stream);
        }
        return {partial ? "partial" : cause,
            static_cast<std::int64_t>(sent),
            transfer.error_code.empty() ? cause : transfer.error_code};
    }
    return {"sent", static_cast<std::int64_t>(sent), {}};
}

} // namespace tx_generated
