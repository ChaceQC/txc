#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

void llvm_code_generator::write_external_declarations()
{
    for (const auto& [name, overloads] : functions_)
    {
        (void)name;
        for (std::size_t index = 0; index < overloads.size(); ++index)
        {
            const auto& function = *overloads[index];
            if (!function.external_name.starts_with("requests."))
            {
                continue;
            }
            bool method = false;
            std::string symbol;
            if (function.external_name.starts_with("requests.session."))
            {
                symbol = "m0_bridge_session_" +
                    function.external_name.substr(17);
                method = true;
            }
            else if (function.external_name.starts_with("requests.response."))
            {
                symbol = "m0_bridge_" + function.external_name.substr(18);
                method = true;
            }
            else if (function.external_name.find('.', 9) == std::string::npos)
            {
                symbol = "m0_bridge_" + function.external_name.substr(9);
            }
            else
            {
                continue;
            }
            const auto implementation = functions_.find(symbol);
            if (implementation != functions_.end() &&
                std::any_of(implementation->second.begin(),
                    implementation->second.end(),
                    [](const function_decl* item) { return !item->external; }))
            {
                continue;
            }
            module_ << "declare " << llvm_type(function.return_type,
                function.position) << ' '
                << function_name(symbol, index) << '(';
            if (method)
            {
                module_ << "ptr";
            }
            for (std::size_t parameter = 0;
                 parameter < function.parameters.size(); ++parameter)
            {
                if (parameter != 0 || method)
                {
                    module_ << ", ";
                }
                module_ << llvm_type(parameter_abi_type(
                                         function.parameters[parameter]),
                                     function.parameters[parameter].position);
            }
            module_ << ")\n";
        }
    }
    write_vector_declarations();
    write_sum_declarations();
    write_iterator_declarations();
    write_container_declarations();
    write_algorithm_declarations();
    module_ << "declare i32 @txrt_parse_try_parse_int(ptr, i64, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_call_needs_default(ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_parse_try_parse_float(ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_parse_parse_int(ptr, i64, ptr)\n"
            << "declare i32 @txrt_parse_parse_float(ptr, ptr)\n"
            << "declare i32 @txrt_json_parse(ptr, ptr)\n"
            << "declare i32 @txrt_json_parse_object(ptr, ptr)\n"
            << "declare i32 @txrt_json_try_parse(ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_json_stringify(ptr, ptr)\n"
            << "declare i32 @txrt_json_stringify_pretty(ptr, i64, ptr)\n"
            << "declare i32 @txrt_json_contains(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_json_get(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_json_get_int(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_json_get_float(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_json_get_bool(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_json_get_str(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_json_get_array(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_json_get_object(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_file_try_read_text(ptr, ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_file_try_write_text(ptr, ptr, ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_file_try_append_text(ptr, ptr, ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_bytes_empty(ptr)\n"
            << "declare i32 @txrt_bytes_from_vector(ptr, ptr)\n"
            << "declare i32 @txrt_bytes_to_vector(ptr, ptr)\n"
            << "declare i32 @txrt_bytes_concat(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_bytes_slice(ptr, i64, i64, ptr)\n"
            << "declare i32 @txrt_bytes_to_hex(ptr, ptr)\n"
            << "declare i32 @txrt_bytes_from_hex(ptr, ptr)\n"
            << "declare i32 @txrt_bytes_to_base64(ptr, ptr)\n"
            << "declare i32 @txrt_bytes_from_base64(ptr, ptr)\n"
            << "declare i32 @txrt_bytes_at(ptr, i64, ptr)\n"
            << "declare i32 @txrt_bytes_equal(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_crypto_random_bytes(i64, ptr)\n"
            << "declare i32 @txrt_crypto_generate_key(ptr)\n"
            << "declare i32 @txrt_crypto_sha256(ptr, ptr)\n"
            << "declare i32 @txrt_crypto_sha512(ptr, ptr)\n"
            << "declare i32 @txrt_crypto_hmac_sha256(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_crypto_secure_equal(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_crypto_hkdf_sha256(ptr, ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_crypto_pbkdf2_sha256(ptr, ptr, i64, i64, ptr)\n"
            << "declare i32 @txrt_crypto_encrypt(ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_crypto_decrypt(ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_encoding_encode(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_encoding_decode(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_open_binary(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_read_bytes(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_read_all_bytes(ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_write_bytes(ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_tell(ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_seek(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_flush_binary(ptr)\n"
            << "declare i32 @txrt_file_stream_close_binary(ptr)\n"
            << "declare i32 @txrt_file_stream_open_text(ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_read_chars(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_read_line(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_write_text(ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_write_line(ptr, ptr)\n"
            << "declare i32 @txrt_file_stream_flush_text(ptr)\n"
            << "declare i32 @txrt_file_stream_close_text(ptr)\n"
            << "declare i32 @txrt_error_status()\n"
            << "declare void @txrt_error_propagation(i1)\n"
            << "declare i32 @txrt_error_take(ptr, ptr)\n"
            << "declare i32 @txrt_error_fail_io(ptr, ptr)\n";
    module_ << "declare i32 @txrt_error_stack_trace(ptr)\n"
            << "declare void @txrt_stack_push(ptr, ptr, i64, i64)\n"
            << "declare void @txrt_stack_pop()\n"
            << "declare void @txrt_stack_location(ptr, i64, i64)\n";
    module_ << "declare i32 @txrt_cancel_source(ptr)\n"
            << "declare i32 @txrt_cancel_with_deadline_ms(i64, ptr)\n"
            << "declare i32 @txrt_cancel_token(ptr, ptr)\n"
            << "declare i32 @txrt_cancel_cancel(ptr, ptr)\n"
            << "declare i32 @txrt_cancel_status(ptr, ptr)\n"
            << "declare i32 @txrt_cancel_wait(ptr, i64, ptr)\n";
    module_ << "declare i32 @txrt_str_concat_literal(ptr, ptr, i64, i1, ptr)\n";
    module_ << "declare i32 @txrt_string_split_vector_literal(ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_string_join_vector_literal(ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_string_join_literal(ptr, ptr, i64, ptr)\n";
    module_ << "declare i32 @txrt_dictionary_get_concat(ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_dictionary_contains_concat(ptr, ptr, ptr, ptr)\n";
    module_ << "declare i32 @txrt_math_abs_i64(i64, ptr)\n"
            << "declare i32 @txrt_math_abs_f64(double, ptr)\n"
            << "declare i32 @txrt_math_min_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_math_min_f64(double, double, ptr)\n"
            << "declare i32 @txrt_math_max_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_math_max_f64(double, double, ptr)\n"
            << "declare i32 @txrt_math_clamp_i64(i64, i64, i64, ptr)\n"
            << "declare i32 @txrt_math_clamp_f64(double, double, double, ptr)\n"
            << "declare i32 @txrt_math_mod_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_math_sqrt_f64(double, ptr)\n"
            << "declare i32 @txrt_math_pow_f64(double, double, ptr)\n"
            << "declare i32 @txrt_math_floor_f64(double, ptr)\n"
            << "declare i32 @txrt_math_ceil_f64(double, ptr)\n"
            << "declare i32 @txrt_random_seed_i64(i64)\n"
            << "declare i32 @txrt_random_int_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_random_float_f64(ptr)\n"
            << "declare i64 @txrt_random_int_direct(i64, i64)\n"
            << "declare double @txrt_random_float_direct()\n"
            << "declare ptr @txrt_random_context()\n"
            << "declare i32 @txrt_random_seed_context(ptr, i64)\n"
            << "declare i64 @txrt_random_int_context(ptr, i64, i64)\n"
            << "declare double @txrt_random_float_context(ptr)\n\n";
    module_ << "declare i32 @txrt_string_contains(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_string_starts_with(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_string_ends_with(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_string_find(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_string_slice(ptr, i64, i64, ptr)\n"
            << "declare i32 @txrt_string_replace(ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_string_split(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_string_join(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_string_contains_literal(ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_string_starts_with_literal(ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_string_ends_with_literal(ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_string_find_literal(ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_string_split_literal(ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_string_replace_literal(ptr, ptr, i64, ptr, i64, ptr)\n"
            << "declare i32 @txrt_string_trim(ptr, ptr)\n"
            << "declare i32 @txrt_string_lower(ptr, ptr)\n"
            << "declare i32 @txrt_string_upper(ptr, ptr)\n"
            << "declare i32 @txrt_format_format(ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_array_concat(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_array_slice(ptr, i64, i64, ptr)\n"
            << "declare i32 @txrt_array_reverse(ptr, ptr)\n"
            << "declare i32 @txrt_array_push_back(ptr, ptr)\n"
            << "declare i32 @txrt_array_pop_back(ptr)\n"
            << "declare i32 @txrt_array_insert(ptr, i64, ptr)\n"
            << "declare i32 @txrt_array_erase(ptr, i64)\n"
            << "declare i32 @txrt_array_clear(ptr)\n"
            << "declare i32 @txrt_dictionary_items(ptr, ptr)\n"
            << "declare i32 @txrt_file_read_text(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_file_write_text(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_file_append_text(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_fs_exists(ptr, ptr)\n"
            << "declare i32 @txrt_fs_is_file(ptr, ptr)\n"
            << "declare i32 @txrt_fs_is_directory(ptr, ptr)\n"
            << "declare i32 @txrt_fs_create_directories(ptr)\n"
            << "declare i32 @txrt_fs_list_directory(ptr, ptr)\n"
            << "declare i32 @txrt_fs_list_directory_vector(ptr, ptr)\n"
            << "declare i32 @txrt_fs_walk_directory(ptr, ptr)\n"
            << "declare i32 @txrt_fs_copy_file(ptr, ptr, i1)\n"
            << "declare i32 @txrt_fs_rename(ptr, ptr)\n"
            << "declare i32 @txrt_fs_remove(ptr, ptr)\n"
            << "declare i32 @txrt_fs_remove_all(ptr, ptr)\n"
            << "declare i32 @txrt_fs_file_size(ptr, ptr)\n"
            << "declare i32 @txrt_fs_modified_millis(ptr, ptr)\n"
            << "declare i32 @txrt_path_join(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_path_parent(ptr, ptr)\n"
            << "declare i32 @txrt_path_file_name(ptr, ptr)\n"
            << "declare i32 @txrt_path_extension(ptr, ptr)\n"
            << "declare i32 @txrt_path_normalize(ptr, ptr)\n"
            << "declare i32 @txrt_path_is_absolute(ptr, ptr)\n"
            << "declare i32 @txrt_path_absolute(ptr, ptr)\n"
            << "declare i32 @txrt_path_relative(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_path_replace_extension(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_system_initialize()\n"
            << "declare i32 @txrt_system_args(ptr)\n"
            << "declare i32 @txrt_system_current_directory(ptr)\n"
            << "declare i32 @txrt_system_set_current_directory(ptr)\n"
            << "declare i32 @txrt_system_executable_path(ptr)\n"
            << "declare i32 @txrt_system_temp_directory(ptr)\n"
            << "declare i32 @txrt_system_home_directory(ptr)\n"
            << "declare i32 @txrt_env_contains(ptr, ptr)\n"
            << "declare i32 @txrt_env_get(ptr, ptr)\n"
            << "declare i32 @txrt_env_get_default(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_env_set(ptr, ptr)\n"
            << "declare i32 @txrt_env_remove(ptr, ptr)\n"
            << "declare i32 @txrt_io_write(ptr)\n"
            << "declare i32 @txrt_io_write_line(ptr)\n"
            << "declare i32 @txrt_io_write_error(ptr)\n"
            << "declare i32 @txrt_io_flush()\n"
            << "declare i32 @txrt_time_unix_millis(ptr)\n"
            << "declare i32 @txrt_time_monotonic_millis(ptr)\n"
            << "declare i32 @txrt_time_monotonic_micros(ptr)\n"
            << "declare i32 @txrt_time_sleep_millis(i64)\n\n";
    module_ << "declare i32 @txrt_httpx_send(ptr, ptr, ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_get(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_post(ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_send_bytes(ptr, ptr, ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_get_bytes(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_post_bytes(ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_send_http2(ptr, ptr, ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_get_http2(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_post_http2(ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_send_http2_bytes(ptr, ptr, ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_get_http2_bytes(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_post_http2_bytes(ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_send_stream(ptr, ptr, ptr, ptr, i64, ptr, i64, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_get_stream(ptr, ptr, i64, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_send_http2_stream(ptr, ptr, ptr, ptr, i64, ptr, i64, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_get_http2_stream(ptr, ptr, i64, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_listen(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_listen_h2c(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_listen_h2_tls(ptr, i64, ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_accept(ptr, i64, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_accept_bytes(ptr, i64, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_accept_stream(ptr, ptr, i64, i64, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_respond(ptr, ptr)\n"
            << "declare i32 @txrt_httpx_respond_bytes(ptr, ptr)\n"
            << "declare i32 @txrt_httpx_respond_stream(ptr, i64, ptr, ptr, ptr, i64)\n"
            << "declare i32 @txrt_httpx_serve_once(ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_serve_once_bytes(ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_httpx_close_listener(ptr)\n"
            << "declare i32 @txrt_httpx_close_connection(ptr)\n"
            << "declare i32 @txrt_ws_connect(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_ws_listen(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_ws_accept(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_ws_send_text(ptr, ptr)\n"
            << "declare i32 @txrt_ws_send_binary(ptr, ptr)\n"
            << "declare i32 @txrt_ws_send_binary_stream(ptr, ptr, i64)\n"
            << "declare i32 @txrt_ws_receive(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_ws_receive_binary(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_ws_receive_binary_stream(ptr, ptr, i64, i64, ptr, ptr)\n"
            << "declare i32 @txrt_ws_reply_once(ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_ws_reply_once_binary(ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_ws_close_listener(ptr)\n"
            << "declare i32 @txrt_ws_close_connection(ptr)\n\n";
}

} // namespace tx
