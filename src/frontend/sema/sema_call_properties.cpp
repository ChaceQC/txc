#include "frontend/sema/sema.hpp"
#include "frontend/ast/call_properties.hpp"

#include <algorithm>

namespace tx
{

void semantic_analyzer::annotate_call_properties(call_expression& call) const
{
    call.properties = {};
    auto& properties = call.properties;
    properties.arguments.resize(call.arguments.size(), argument_ownership::held);
    if (call.indirect || call.is_constructor)
    {
        return;
    }
    if (!call.receiver && !call.container_type &&
        (call.name == "len" || call.name == "is_none" ||
         call.name == "to_float") && call.arguments.size() == 1)
    {
        properties.effects = {false, false, false, false, false, false};
        if (properties.effects.allows_borrow())
        {
            properties.arguments.front() = argument_ownership::borrowed;
        }
        return;
    }
    if (call.receiver || call.container_type)
    {
        const auto& type = call.container_type ? *call.container_type : call.receiver->type;
        properties.effects = container_call_effects(type,
            call.container_type ? "new" : call.name);
        if (call.receiver && properties.effects.allows_borrow())
        {
            properties.receiver = argument_ownership::borrowed;
        }
        return;
    }
    const auto found = functions_.find(call.name);
    if (!call.overload_index || found == functions_.end())
    {
        return;
    }
    const auto& signature = found->second.at(*call.overload_index);
    if (!signature.external)
    {
        return;
    }
    const auto& name = signature.external_name;
    if (name.starts_with("graphics.") || name.starts_with("gui."))
    {
        // Win32 回调不执行 TX；参数仅在本次调用中读取，原生资源独立管理寿命。
        const bool draw = name == "graphics.clear" || name.starts_with("graphics.draw_") ||
            name.starts_with("graphics.fill_");
        properties.effects = {!draw, !draw, false, false, true, false};
        for (auto& ownership : properties.arguments)
        {
            ownership = argument_ownership::borrowed;
        }
        return;
    }
    const bool typed_format = name == "format.format" &&
        std::all_of(call.arguments.begin(), call.arguments.end(), [](const call_argument& argument)
        {
            const auto& type = argument.value->type;
            return (argument.kind == argument_kind::positional || argument.kind == argument_kind::keyword) &&
                (type == value_type::int_type || type == value_type::float_type ||
                 type == value_type::bool_type || type == value_type::str_type);
        });
    if (typed_format || name.starts_with("decimal.") || name == "statistics.mean")
    {
        // 基础类型格式化、decimal 原生运算和均值遍历不会调用用户代码。
        // 均值的迭代器入口会推进游标，但只处理 float，不能释放用户对象。
        properties.effects = {name != "statistics.mean", false, false, false,
            name == "statistics.mean" && !call.arguments.empty() &&
                call.arguments.front().value->type.is_iterator(), false};
        for (auto& ownership : properties.arguments)
        {
            ownership = argument_ownership::borrowed;
        }
        return;
    }
    if (name == "serde.serialize_json" || name == "serde.serialize_cbor" ||
        name == "serde.deserialize_json" || name == "serde.deserialize_cbor")
    {
        // 解码可以登记新对象；借用输入与是否需要 GC 安全点分别描述。
        properties.effects = {true, name.starts_with("serde.deserialize_"),
                              false, false, false, false};
        for (auto& ownership : properties.arguments)
        {
            ownership = argument_ownership::borrowed;
        }
        return;
    }
    // 这些原生入口只在调用期间读取参数；结果/错误仍可分配，但不保存借用地址。
    const bool read_only_input = name == "parse.try_parse_int" ||
        name == "parse.try_parse_float" || name == "parse.parse_int" ||
        name == "parse.parse_float" || name == "encoding.encode" ||
        name == "encoding.decode" || name == "bytes.from_hex" ||
        name == "bytes.from_base64" || name == "bytes.from_base64_url" ||
        name == "bytes.to_hex" || name == "bytes.to_base64" ||
        name == "bytes.to_base64_url" || name == "bytes.to_hex_chunk" ||
        name == "bytes.to_base64_chunk";
    // 这里只登记不会回调 TX、不会释放用户对象的入口。状态可以修改，
    // 但局部根在整个同步调用期间有效；保存的值由原生实现独立持有。
    const bool scalar_sync = name.starts_with("sync.atomic_") ||
        (!call.arguments.empty() &&
         call.arguments.front().value->type.is_sync_value() &&
         !call.arguments.front().value->type.parameters.empty() &&
         (call.arguments.front().value->type.parameters.front() == value_type::int_type ||
          call.arguments.front().value->type.parameters.front() == value_type::float_type ||
          call.arguments.front().value->type.parameters.front() == value_type::bool_type) &&
         (name == "sync.lock" || name == "sync.guard_get" ||
          name == "sync.guard_set" || name == "sync.guard_close" ||
          name == "sync.read_lock" || name == "sync.write_lock" ||
          name == "sync.read_get" || name == "sync.write_get" ||
          name == "sync.write_set" || name == "sync.read_close" ||
          name == "sync.write_close"));
    const bool native_query = name.starts_with("db.get_") ||
        name.starts_with("db.as_") || name.starts_with("db.column_") ||
        name == "db.next" || name == "db.value_kind" ||
        name == "regex.search" || name == "regex.match" ||
        name == "regex.full_match" || name == "regex.find_all" ||
        name == "regex.split" || name == "regex.replace" ||
        name == "env.get" || name == "env.contains" ||
        name == "log.enabled" || name == "log.set_level" ||
        name == "string.contains" || name == "string.starts_with" ||
        name == "string.ends_with" || name == "string.find" ||
        name == "string.slice" || name == "string.replace" ||
        name == "string.split" || name == "string.split_vector" ||
        name == "string.trim" || name == "string.lower" || name == "string.upper";
    const bool scalar_channel = name.starts_with("channel.") &&
        !call.arguments.empty() &&
        call.arguments.front().value->type.container_name() == "channel" &&
        !call.arguments.front().value->type.parameters.empty() &&
        (call.arguments.front().value->type.parameters.front() == value_type::int_type ||
         call.arguments.front().value->type.parameters.front() == value_type::float_type ||
         call.arguments.front().value->type.parameters.front() == value_type::bool_type) &&
        (name == "channel.send" || name == "channel.recv" || name == "channel.close");
    if (read_only_input || scalar_sync || native_query || scalar_channel)
    {
        const bool sync_query = name == "sync.guard_get" || name == "sync.read_get" ||
            name == "sync.write_get" || name == "sync.atomic_load";
        const bool sync_allocates = name == "sync.lock" || name == "sync.read_lock" ||
            name == "sync.write_lock";
        // 成功的标量读写不分配；失败立即退出当前表达式，仍保留错误路径。
        properties.effects = {!scalar_sync || sync_allocates, name == "db.next", false, false,
            (scalar_sync && !sync_query) || scalar_channel || name == "db.next" || name == "log.set_level",
            scalar_channel && name == "channel.send"};
        for (auto& ownership : properties.arguments)
        {
            ownership = argument_ownership::borrowed;
        }
        return;
    }
    const bool dictionary_query = (name == "dictionary.get" ||
        name == "dictionary.contains") && call.arguments.size() == 2;
    const bool cancel_query = name == "cancel.status";
    if (dictionary_query || cancel_query)
    {
        properties.effects = {name == "dictionary.get", false, false,
                              false, false, false};
        for (auto& ownership : properties.arguments)
        {
            ownership = argument_ownership::borrowed;
        }
        return;
    }
    if (name == "math.sqrt" || name == "random.seed" ||
        name == "random.random_int" || name == "random.random_float")
    {
        properties.effects = {false, false, false, false, false, false};
    }
}

} // namespace tx
