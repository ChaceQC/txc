#include "backend/cpp/deep_copy.hpp"
#include "stdlib/graphics/resource.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/concurrency_value.hpp"
#include "stdlib/cancellation.hpp"
#include "stdlib/secret.hpp"
#include "stdlib/file_stream.hpp"
#include "stdlib/encoding_incremental.hpp"
#include "stdlib/regex.hpp"
#include "stdlib/filesystem_watch.hpp"
#include "stdlib/process.hpp"
#include "stdlib/ipc.hpp"
#include "stdlib/db.hpp"
#include "stdlib/json_stream.hpp"
#include "stdlib/cbor.hpp"
#include "stdlib/csv.hpp"
#include "stdlib/xml.hpp"

#include <stdexcept>

namespace tx_generated
{

std::any copy_resource_value(const std::any& value)
{
    if (value.type() == typeid(graphics::handle))
    {
        throw runtime_failure({tx::error_kind::graphics, "invalid_argument",
            "deep_copy 不能复制图形资源，包括容器或结构体中的图形资源"});
    }
    if (value.type() == typeid(db_row) || value.type() == typeid(db_value))
    {
        return value;
    }
    if (value.type() == typeid(db_connection) ||
        value.type() == typeid(db_statement) ||
        value.type() == typeid(db_cursor) ||
        value.type() == typeid(db_transaction) || value.type() == typeid(db_pool))
    {
        db_fail("invalid_state", "deep_copy 不能复制数据库资源");
    }
    if (value.type() == typeid(secret::handle))
    {
        throw runtime_failure({tx::error_kind::security, "invalid_state",
            "secret_bytes 不能使用 deep_copy"});
    }
    if (value.type() == typeid(cancel_source) ||
        value.type() == typeid(cancel_token) ||
        value.type() == typeid(regex_pattern) ||
        is_shareable_concurrency_value(value))
    {
        // 取消令牌复制共享同一状态，不能复制成独立取消域。
        return value;
    }
    if (value.type() == typeid(binary_stream) ||
        value.type() == typeid(text_stream) ||
        value.type() == typeid(encoding_decoder) ||
        value.type() == typeid(encoding_encoder))
    {
        throw std::runtime_error("deep_copy 不支持复制文件流或增量编解码状态");
    }
    if (value.type() == typeid(process_child) ||
        value.type() == typeid(process_pipe) ||
        value.type() == typeid(ipc_listener) ||
        value.type() == typeid(ipc_stream))
    {
        throw std::runtime_error("deep_copy 不支持复制子进程或管道");
    }
    if (value.type() == typeid(json_reader) || value.type() == typeid(json_writer))
    {
        throw std::runtime_error("deep_copy 不支持复制 JSON 游标");
    }
    if (value.type() == typeid(cbor_reader) || value.type() == typeid(cbor_writer))
    {
        throw std::runtime_error("deep_copy 不支持复制 CBOR 游标");
    }
    if (value.type() == typeid(csv_reader) || value.type() == typeid(csv_writer))
    {
        throw std::runtime_error("deep_copy 不支持复制 CSV 游标");
    }
    if (value.type() == typeid(xml_reader) || value.type() == typeid(xml_writer) ||
        value.type() == typeid(xml_document) || value.type() == typeid(xml_node))
    {
        throw std::runtime_error("deep_copy 不支持复制 XML 句柄");
    }
    if (value.type() == typeid(fs_watcher))
    {
        throw std::runtime_error("deep_copy 不支持复制文件监视器");
    }
    throw std::runtime_error("deep_copy 不支持此运行时类型");
}

} // namespace tx_generated
