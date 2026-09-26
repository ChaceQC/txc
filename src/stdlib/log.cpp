#include "stdlib/log.hpp"

#include "stdlib/array.hpp"
#include "stdlib/error.hpp"
#include "stdlib/json.hpp"
#include "stdlib/stdlib.hpp"

#include <algorithm>
#include <any>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>

namespace tx_generated
{
namespace
{

thread_local std::string current_task_id;
thread_local std::string current_thread_id;

std::string lower_ascii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

bool secret_name(const std::string& key,
                 const std::unordered_set<std::string>& extra)
{
    const auto normalized = lower_ascii(key);
    if (extra.contains(normalized))
    {
        return true;
    }
    for (const std::string_view part : {"password", "token", "secret",
        "api_key", "authorization", "credential", "cookie"})
    {
        if (normalized.find(part) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

std::any redact_value(const std::any& value,
                      const std::unordered_set<std::string>& extra,
                      int depth)
{
    if (depth >= 16)
    {
        return std::string("[DEPTH_LIMIT]");
    }
    if (const auto* object = std::any_cast<tx_dict>(&value))
    {
        tx_dict sanitized;
        object->for_each([&](const std::any& key, const std::any& item)
        {
            const auto* name = std::any_cast<std::string>(&key);
            if (!name)
            {
                throw runtime_failure({tx::error_kind::runtime,
                    "invalid_field", "日志字段键必须是 str"});
            }
            const auto safe = secret_name(*name, extra)
                ? std::any(std::string("[REDACTED]"))
                : redact_value(item, extra, depth + 1);
            sanitized.emplace_back(*name, safe);
        });
        return sanitized;
    }
    if (const auto* values = std::any_cast<tx_array>(&value))
    {
        tx_array sanitized;
        for (const auto& item : *values)
        {
            sanitized.push_back(redact_value(item, extra, depth + 1));
        }
        return sanitized;
    }
    return value;
}

} // namespace

void tx_log_event(const std::string& level, const std::string& message,
                  const tx_dict& fields,
                  const std::vector<std::string>& secret_keys)
{
    if (level != "trace" && level != "debug" && level != "info" &&
        level != "warn" && level != "error")
    {
        throw runtime_failure({tx::error_kind::runtime,
            "invalid_level", "日志级别必须是 trace/debug/info/warn/error"});
    }
    std::unordered_set<std::string> extra;
    for (const auto& key : secret_keys)
    {
        extra.insert(lower_ascii(key));
    }
    auto safe_fields = redact_value(std::any(fields), extra, 0);
    tx_dict context;
    context.emplace_back(std::string("task_id"), current_task_id);
    context.emplace_back(std::string("thread_id"), current_thread_id);
    tx_dict event;
    event.emplace_back(std::string("timestamp_ms"), tx_fn_unix_millis());
    event.emplace_back(std::string("level"), level);
    event.emplace_back(std::string("message"), message);
    event.emplace_back(std::string("context"), context);
    event.emplace_back(std::string("fields"), std::move(safe_fields));
    // 先完成遮蔽与序列化，再一次写出完整事件。
    tx_fn_write_error(json_stringify(std::any(event)) + "\n");
}

void tx_log_set_context(const std::string& task_id,
                        const std::string& thread_id)
{
    current_task_id = task_id;
    current_thread_id = thread_id;
}

} // namespace tx_generated
