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
        return "txrt_random_int_i64";
    }
    if (name == "random.random_float")
    {
        return "txrt_random_float_f64";
    }
    return nullptr;
}

} // namespace

std::string llvm_code_generator::random_context()
{
    if (random_context_slot_.empty())
    {
        random_context_slot_ = "%slot" + std::to_string(next_slot_++);
        allocations_ << "  " << random_context_slot_ << " = alloca ptr\n"
                     << "  store ptr null, ptr " << random_context_slot_ << '\n';
    }
    const auto cached = temporary();
    write_instruction(cached + " = load ptr, ptr " + random_context_slot_);
    const auto missing = temporary();
    write_instruction(missing + " = icmp eq ptr " + cached + ", null");
    const auto acquire_label = label();
    const auto ready_label = label();
    write_instruction("br i1 " + missing + ", label %" + acquire_label +
                      ", label %" + ready_label);
    start_block(acquire_label);
    const auto acquired = temporary();
    write_instruction(acquired + " = call ptr @txrt_random_context()");
    write_instruction("store ptr " + acquired + ", ptr " + random_context_slot_);
    write_instruction("br label %" + ready_label);
    start_block(ready_label);
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + random_context_slot_);
    return result;
}

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
    if (target.external_name.starts_with("algorithm."))
    {
        // 重载已由语义分析确定，元素类型直接选定 ABI，不把选择推迟到运行时。
        symbol += "_" + vector_suffix(target.parameters.front().type);
    }
    if (target.external_name == "env.get" && target.parameters.size() == 2)
    {
        symbol = "txrt_env_get_default";
    }
    if (target.external_name == "file_stream.flush" ||
        target.external_name == "file_stream.close")
    {
        symbol += target.parameters.front().type == value_type::binary_stream_type
            ? "_binary" : "_text";
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
    if (dictionary_key_call && !string_key)
    {
        boxed_key = box_any(arguments[1], item.position);
    }
    if (string_key)
    {
        symbol += "_str";
    }
    std::string parameters;
    std::vector<ir_value> boxed_values;
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        auto argument = dictionary_key_call && !string_key && index == 1
            ? boxed_key : arguments[index];
        if (target.parameters[index].type == value_type::any_type)
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
        target.external_name == "httpx.send_stream" ||
        target.external_name == "httpx.get_stream" ||
        target.external_name == "httpx.send_http2_stream" ||
        target.external_name == "httpx.get_http2_stream" ||
        target.external_name == "httpx.listen" ||
        target.external_name == "httpx.listen_h2c" ||
        target.external_name == "httpx.listen_h2_tls" ||
        target.external_name == "websocket.connect" ||
        target.external_name == "websocket.listen" ||
        target.external_name == "websocket.accept" ||
        target.external_name == "websocket.receive" ||
        target.external_name == "websocket.receive_binary" ||
        target.external_name == "websocket.receive_binary_stream")
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
        target.external_name == "httpx.serve_once_bytes")
    {
        const auto& request_type = target.parameters[1].type.parameters.front();
        const auto& connection_type = structs_.at(request_type.name)->fields.front().type;
        parameters += ", ptr " + global_bytes(request_type.name) +
                      ", ptr " + global_bytes(connection_type.name);
    }
    if (target.external_name.starts_with("random."))
    {
        const auto context = random_context();
        parameters = "ptr " + context + (parameters.empty() ? "" : ", " + parameters);
    }
    if (target.external_name == "random.random_int" ||
        target.external_name == "random.random_float")
    {
        const auto result = temporary();
        const auto* direct = target.external_name == "random.random_int"
            ? "txrt_random_int_context" : "txrt_random_float_context";
        write_instruction(result + " = call " +
                          llvm_type(item.type, item.position) + " @" + direct +
                          "(" + parameters + ")");
        for (const auto& argument : arguments)
        {
            release(argument);
        }
        return {item.type, result};
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
