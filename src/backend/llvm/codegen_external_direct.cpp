#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <string_view>

namespace tx
{
namespace
{

const char* numeric_symbol(std::string_view name, bool integer)
{
    if (name == "math.abs")
    {
        return integer ? "txrt_math_abs_i64" : "txrt_math_abs_f64";
    }
    if (name == "math.min")
    {
        return integer ? "txrt_math_min_i64" : "txrt_math_min_f64";
    }
    if (name == "math.max")
    {
        return integer ? "txrt_math_max_i64" : "txrt_math_max_f64";
    }
    if (name == "math.clamp")
    {
        return integer ? "txrt_math_clamp_i64" : "txrt_math_clamp_f64";
    }
    if (name == "math.mod")
    {
        return "txrt_math_mod_i64";
    }
    if (name == "math.sqrt")
    {
        return "txrt_math_sqrt_f64";
    }
    if (name == "math.pow")
    {
        return "txrt_math_pow_f64";
    }
    if (name == "math.floor")
    {
        return "txrt_math_floor_f64";
    }
    if (name == "math.ceil")
    {
        return "txrt_math_ceil_f64";
    }
    if (name == "random.seed")
    {
        return "txrt_random_seed_context";
    }
    if (name == "random.random_int")
    {
        return "txrt_random_int_context";
    }
    if (name == "random.random_float")
    {
        return "txrt_random_float_context";
    }
    return nullptr;
}

} // namespace

llvm_code_generator::ir_value llvm_code_generator::emit_direct_external_call(
    const expression& item, const function_decl& target,
    const std::vector<ir_value>& arguments)
{
    const bool integer = !arguments.empty() &&
        arguments.front().type == value_type::int_type;
    std::string symbol;
    if (const auto* numeric = numeric_symbol(target.external_name, integer))
    {
        symbol = numeric;
    }
    else
    {
        symbol = "txrt_" + target.external_name;
        std::replace(symbol.begin(), symbol.end(), '.', '_');
        if (target.external_name.starts_with("websocket."))
        {
            symbol = "txrt_ws_" + target.external_name.substr(10);
        }
    }
    if (target.external_name == "test.assert_equal" ||
        target.external_name == "debug.dump")
    {
        const auto& type = target.parameters.front().type;
        symbol += type == value_type::int_type ? "_i64" :
                  type == value_type::float_type ? "_f64" :
                  type == value_type::bool_type ? "_bool" : "_str";
    }
    if (target.external_name == "httpx.close" ||
        target.external_name == "websocket.close")
    {
        symbol += target.parameters.front().type.name.ends_with("_listener")
            ? "_listener" : "_connection";
    }
    if (target.external_name == "socket.close")
    {
        if (target.parameters.front().type.name.ends_with("_tcp_listener"))
        {
            symbol += "_tcp_listener";
        }
        else if (target.parameters.front().type.name.ends_with("_tcp_stream"))
        {
            symbol += "_tcp_stream";
        }
        else
        {
            symbol += "_udp_socket";
        }
    }
    if (target.external_name.starts_with("algorithm."))
    {
        // 重载已由语义分析确定，元素类型直接选定 ABI，不把选择推迟到运行时。
        symbol += "_" + vector_suffix(target.parameters.front().type);
    }
    const bool typed_random = target.external_name == "random.shuffle" ||
        target.external_name == "random.sample" ||
        target.external_name == "random.choice";
    if (typed_random)
    {
        symbol += "_" + vector_suffix(arguments[1].type);
    }
    if (target.external_name.starts_with("statistics.") &&
        !target.parameters.empty() &&
        (target.parameters.front().type.is_vector() ||
         target.parameters.front().type.container_name() == "iterator"))
    {
        symbol += target.parameters.front().type.is_vector()
            ? "_vector" : "_iterator";
    }
    if (target.external_name == "env.get" && target.parameters.size() == 2)
    {
        symbol = "txrt_env_get_default";
    }
    if (target.external_name == "file_stream.flush" ||
        target.external_name == "file_stream.sync" ||
        target.external_name == "file_stream.close")
    {
        symbol += target.parameters.front().type == value_type::binary_stream_type
            ? "_binary" : "_text";
    }
    if (target.external_name == "json.read" || target.external_name == "json.reader" ||
        target.external_name == "json.write" || target.external_name == "json.writer" ||
        target.external_name == "csv.reader" || target.external_name == "csv.writer" ||
        target.external_name == "xml.reader" || target.external_name == "xml.read" ||
        target.external_name == "xml.write" || target.external_name == "xml.writer")
    {
        symbol += target.parameters.front().type == value_type::binary_stream_type
            ? "_binary" : "_text";
    }
    if (target.external_name == "json.close")
    {
        symbol += target.parameters.front().type == value_type::json_reader_type
            ? "_reader" : "_writer";
    }
    if (target.external_name == "cbor.close")
    {
        symbol += target.parameters.front().type == value_type::cbor_reader_type
            ? "_reader" : "_writer";
    }
    if (target.external_name == "csv.close")
    {
        symbol += target.parameters.front().type == value_type::csv_reader_type
            ? "_reader" : "_writer";
    }
    if (target.external_name == "xml.close" ||
        target.external_name.starts_with("xml.attribute_"))
    {
        symbol += target.parameters.front().type == value_type::xml_reader_type
            ? "_reader" : "_node";
        if (target.external_name == "xml.close" &&
            target.parameters.front().type == value_type::xml_writer_type)
        {
            symbol = "txrt_xml_close_writer";
        }
    }
    if (target.external_name == "xml.write" &&
        target.parameters.front().type == value_type::text_stream_type)
    {
        symbol = "txrt_xml_write_text_stream";
    }
    const bool dictionary_key_call = target.external_name == "dictionary.get" ||
        target.external_name == "dictionary.contains" ||
        target.external_name == "dictionary.remove";
    if (target.external_name == "string.join" && arguments.front().type.is_vector())
    {
        symbol = "txrt_string_join_vector";
    }
    ir_value boxed_key{value_type::void_type, {}};
    const bool string_key = dictionary_key_call &&
        arguments[1].type == value_type::str_type;
    const bool scalar_key = dictionary_key_call &&
        (arguments[1].type == value_type::int_type ||
         arguments[1].type == value_type::float_type ||
         arguments[1].type == value_type::bool_type);
    if (dictionary_key_call && !string_key && !scalar_key)
    {
        boxed_key = box_any(arguments[1], item.position);
    }
    if (string_key)
    {
        symbol += "_str";
    }
    else if (scalar_key)
    {
        symbol += arguments[1].type == value_type::int_type ? "_i64" :
            arguments[1].type == value_type::float_type ? "_f64" : "_bool";
    }
    std::string parameters;
    std::vector<ir_value> boxed_values;
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        auto argument = dictionary_key_call && !string_key && !scalar_key && index == 1
            ? boxed_key : arguments[index];
        if (target.parameters[index].type == value_type::any_type && !typed_random)
        {
            // 静态选定的异构容器入口只在保存值的边界进行装箱。
            argument = box_any(argument, item.position);
            boxed_values.push_back(argument);
        }
        parameters += llvm_type(argument.type, item.position) + " " + argument.text;
    }
    if ((target.external_name == "parse.try_parse_int" ||
         target.external_name == "parse.parse_int") && arguments.size() == 1)
    {
        parameters += ", i64 10";
    }
    if (target.external_name.starts_with("parse.try_") ||
        target.external_name == "json.try_parse" ||
        target.external_name.starts_with("file.try_"))
    {
        const auto& fields = structs_.at(item.type.name)->fields;
        parameters += ", ptr " + global_bytes(item.type.name) +
                      ", ptr " + global_bytes(fields.at(2).type.name);
    }
    if (target.external_name == "file_stream.read_bytes" ||
        target.external_name == "file_stream.read_chars" ||
        target.external_name == "file_stream.read_line")
    {
        parameters += ", ptr " + global_bytes(item.type.name);
    }
    if (target.external_name == "process.run" ||
        target.external_name == "process.run_with_cancel")
    {
        const auto& status_type = structs_.at(item.type.name)->fields.at(1).type;
        parameters += ", ptr " + global_bytes(item.type.name) +
                      ", ptr " + global_bytes(status_type.name);
    }
    if (target.external_name == "json.default_limits" ||
        target.external_name == "json.next" ||
        target.external_name == "cbor.default_limits" ||
        target.external_name == "cbor.next" ||
        target.external_name == "csv.default_dialect" ||
        target.external_name == "csv.next_row" ||
        target.external_name == "xml.default_limits" ||
        target.external_name == "xml.next" ||
        target.external_name == "xml.first_child" ||
        target.external_name == "xml.next_sibling" ||
        target.external_name == "process.make_options" ||
        target.external_name == "process.make_limits" ||
        target.external_name == "process.read_pipe" ||
        target.external_name == "process.write_pipe" ||
        target.external_name == "x509.verify" ||
        target.external_name == "dns.resolve" ||
        target.external_name == "dns.resolve_with_cancel" ||
        target.external_name == "socket.listen_tcp" ||
        target.external_name == "socket.connect_tcp" ||
        target.external_name == "socket.accept_tcp" ||
        target.external_name == "socket.connect_async" ||
        target.external_name == "socket.accept_async" ||
        target.external_name == "socket.read" ||
        target.external_name == "socket.read_async" ||
        target.external_name == "socket.write_async" ||
        target.external_name == "socket.bind_udp" ||
        target.external_name == "socket.receive_from" ||
        target.external_name == "socket.send_to_async" ||
        target.external_name == "socket.receive_from_async" ||
        target.external_name == "tls.system_trust" ||
        target.external_name == "tls.custom_trust" ||
        target.external_name == "tls.import_identity" ||
        target.external_name == "tls.client" ||
        target.external_name == "tls.with_client_identity" ||
        target.external_name == "tls.server" ||
        target.external_name == "tls.verify_server" ||
        target.external_name == "tls.verify_client" ||
        target.external_name == "tls.connect" ||
        target.external_name == "tls.accept" ||
        target.external_name == "tls.read" ||
        target.external_name == "process.wait" ||
        target.external_name == "process.wait_with_cancel" ||
        target.external_name == "process.try_wait" ||
        target.external_name == "fs.stat" ||
        target.external_name == "fs.lstat" ||
        target.external_name == "fs.watch_next" ||
        target.external_name == "time.duration_from_micros" ||
        target.external_name == "time.duration_from_millis" ||
        target.external_name == "time.duration_from_seconds" ||
        target.external_name == "time.duration_add" ||
        target.external_name == "time.duration_sub" ||
        target.external_name == "time.duration_negate" ||
        target.external_name == "time.instant_now" ||
        target.external_name == "time.instant_elapsed" ||
        target.external_name == "time.instant_after" ||
        target.external_name == "time.parse_local_date" ||
        target.external_name == "time.date_add_days" ||
        target.external_name == "time.date_add_months" ||
        target.external_name == "time.parse_local_time" ||
        target.external_name == "time.parse_offset_datetime" ||
        target.external_name == "time.offset_from_unix_millis" ||
        target.external_name == "time.datetime_difference" ||
        target.external_name == "time.at_zone" ||
        target.external_name == "time.resolve_local" ||
        target.external_name == "time.zoned_local_date" ||
        target.external_name == "time.zoned_local_time" ||
        target.external_name == "random.make_generator" ||
        target.external_name == "statistics.new_accumulator" ||
        target.external_name == "statistics.histogram" ||
        target.external_name == "decimal.parse" ||
        target.external_name == "decimal.from_int" ||
        target.external_name == "decimal.quantize" ||
        target.external_name == "decimal.add" ||
        target.external_name == "decimal.sub" ||
        target.external_name == "decimal.mul" ||
        target.external_name == "decimal.div")
    {
        parameters += (parameters.empty() ? "" : ", ") +
                      std::string("ptr ") + global_bytes(item.type.name);
    }
    if (target.external_name == "xml.next")
    {
        parameters += ", ptr " + global_bytes(item.type.parameters.front().name);
    }
    if (target.external_name == "dns.resolve" ||
        target.external_name == "dns.resolve_with_cancel")
    {
        parameters += ", ptr " + global_bytes(item.type.parameters.front().name);
    }
    if (target.external_name == "socket.connect_async" ||
        target.external_name == "socket.accept_async" ||
        target.external_name == "socket.read_async" ||
        target.external_name == "socket.receive_from_async")
    {
        parameters += ", ptr " + global_bytes(item.type.parameters.front().name);
    }
    if (target.external_name == "regex.search" ||
        target.external_name == "regex.match" ||
        target.external_name == "regex.full_match")
    {
        parameters += ", ptr " + global_bytes(item.type.name);
    }
    if (target.external_name == "httpx.send" ||
        target.external_name == "httpx.get" ||
        target.external_name == "httpx.post" ||
        target.external_name == "httpx.send_bytes" ||
        target.external_name == "httpx.get_bytes" ||
        target.external_name == "httpx.post_bytes" ||
        target.external_name == "httpx.send_http2" ||
        target.external_name == "httpx.get_http2" ||
        target.external_name == "httpx.post_http2" ||
        target.external_name == "httpx.send_http2_bytes" ||
        target.external_name == "httpx.get_http2_bytes" ||
        target.external_name == "httpx.post_http2_bytes" ||
        target.external_name == "httpx.send_http3" ||
        target.external_name == "httpx.get_http3" ||
        target.external_name == "httpx.post_http3" ||
        target.external_name == "httpx.send_http3_bytes" ||
        target.external_name == "httpx.get_http3_bytes" ||
        target.external_name == "httpx.post_http3_bytes" ||
        target.external_name == "httpx.send_http3_with_trust" ||
        target.external_name == "httpx.send_http3_with_trust_bytes" ||
        target.external_name == "httpx.send_http3_controlled" ||
        target.external_name == "httpx.send_http3_controlled_bytes" ||
        target.external_name == "httpx.send_negotiated" ||
        target.external_name == "httpx.send_negotiated_bytes" ||
        target.external_name == "httpx.send_stream" ||
        target.external_name == "httpx.get_stream" ||
        target.external_name == "httpx.send_http2_stream" ||
        target.external_name == "httpx.get_http2_stream" ||
        target.external_name == "httpx.open_session" ||
        target.external_name == "httpx.open_secure_session" ||
        target.external_name == "httpx.begin_request" ||
        target.external_name == "httpx.finish_request" ||
        target.external_name == "httpx.read_response_chunk" ||
        target.external_name == "httpx.match_route" ||
        target.external_name == "httpx.listen" ||
        target.external_name == "httpx.listen_with_limit" ||
        target.external_name == "httpx.listen_h2c" ||
        target.external_name == "httpx.listen_h2_tls" ||
        target.external_name == "httpx.listen_h3" ||
        target.external_name == "websocket.connect" ||
        target.external_name == "websocket.listen" ||
        target.external_name == "websocket.listen_tls" ||
        target.external_name == "websocket.accept" ||
        target.external_name == "websocket.upgrade" ||
        target.external_name == "websocket.receive" ||
        target.external_name == "websocket.receive_binary" ||
        target.external_name == "websocket.receive_binary_stream" ||
        target.external_name == "websocket.get_close_status")
    {
        parameters += ", ptr " + global_bytes(item.type.name);
    }
    if (target.external_name == "httpx.accept" ||
        target.external_name == "httpx.accept_bytes" ||
        target.external_name == "httpx.accept_stream")
    {
        const auto& connection_type = structs_.at(item.type.name)->fields.front().type;
        parameters += ", ptr " + global_bytes(item.type.name) +
                      ", ptr " + global_bytes(connection_type.name);
    }
    if (target.external_name == "httpx.serve_once" ||
        target.external_name == "httpx.serve_once_bytes" ||
        target.external_name == "httpx.serve_routes" ||
        target.external_name == "httpx.serve_routes_bytes")
    {
        const auto& request_type = target.external_name.starts_with("httpx.serve_routes")
            ? structs_.at(target.parameters[1].type.parameters.front().name)
                ->fields[2].type.parameters.front()
            : target.parameters[1].type.parameters.front();
        const auto& connection_type = structs_.at(request_type.name)->fields.front().type;
        parameters += ", ptr " + global_bytes(request_type.name) +
                      ", ptr " + global_bytes(connection_type.name);
    }
    if (target.external_name == "random.seed" ||
        target.external_name == "random.random_int" ||
        target.external_name == "random.random_float")
    {
        parameters = "ptr %tx_context" + (parameters.empty() ? "" : ", " + parameters);
    }
    if (target.external_name == "async_file.read_at" ||
        target.external_name == "async_file.write_at")
    {
        parameters += ", ptr " + global_bytes(item.type.name) +
            ", ptr " + global_bytes(item.type.parameters.front().name);
    }
    if (target.external_name == "ipc.send" ||
        target.external_name == "ipc.recv")
    {
        parameters += ", ptr " + global_bytes(item.type.name);
    }
    if (target.external_name.starts_with("debug."))
    {
        emit_stack_location();
    }
    if (target.external_name.starts_with("db.") &&
        (item.type.is_option() || structs_.contains(item.type.name)))
    {
        parameters += (parameters.empty() ? "" : ", ") +
            std::string("ptr ") + global_bytes(item.type.name);
        if (item.type.is_option() &&
            (target.external_name.ends_with("_decimal") ||
             target.external_name.ends_with("_datetime")))
        {
            parameters += ", ptr " + global_bytes(item.type.parameters.front().name);
        }
    }
    const bool returns_value = item.type != value_type::void_type;
    std::string address;
    if (returns_value)
    {
        address = allocate(item.type, item.position);
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        parameters += "ptr " + address;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + symbol + "(" + parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(boxed_key);
    for (const auto& value : boxed_values)
    {
        release(value);
    }
    for (const auto& argument : arguments)
    {
        release(argument);
    }
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    if (item.type == value_type::str_type || is_value_handle(item.type))
    {
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        return {item.type, result};
    }
    return load({item.type, address});
}

} // namespace tx
