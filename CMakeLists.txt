cmake_minimum_required(VERSION 3.21)
project(tx_compiler LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)
if(WIN32)
    include(cmake/dependencies_windows.cmake)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    include(cmake/dependencies_linux.cmake)
else()
    message(FATAL_ERROR "当前支持 Windows x64 和 Linux x86_64")
endif()

# 编译器内嵌公开接口和 ABI 的内容摘要；增删或修改文件都会触发重新配置。
file(GLOB _tx_interface_files CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tx/stdlib/*.txh")
file(GLOB _tx_abi_files CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src/backend/cpp/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/backend/llvm/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/analysis/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/analysis/*.hpp")
list(APPEND _tx_abi_files "src/common/error_kind.hpp" "src/common/format_spec.hpp"
    "src/common/record_copy_kind.hpp"
    "src/common/slot_layout.hpp" "src/common/local_resource_layout.hpp"
    "src/common/text_encoding.hpp" "src/stdlib/parse.hpp" "src/stdlib/serde_direct.hpp"
    "src/stdlib/closure.hpp" "src/stdlib/typed_heap.hpp" "src/stdlib/iterator.hpp")
list(APPEND _tx_abi_files "src/stdlib/container_scalar.hpp" "src/stdlib/typed_map.hpp")
list(APPEND _tx_abi_files "src/stdlib/db.hpp")
list(APPEND _tx_abi_files "src/common/graphics_types.hpp" "src/stdlib/graphics/resource.hpp")
list(APPEND _tx_abi_files "cmake/mbedtls_user_config.h")
list(APPEND _tx_abi_files "cmake/mbedtls_linux_config.h" "src/common/platform.hpp")
list(APPEND _tx_abi_files "src/stdlib/byte_storage.hpp" "src/stdlib/vector.hpp"
    "src/stdlib/serde_scalar.hpp" "src/stdlib/json_parser.hpp")
list(SORT _tx_interface_files)
list(SORT _tx_abi_files)
set(TX_INTERFACE_COUNT 0)
set(TX_INTERFACE_RECORDS "")
foreach(_tx_file IN LISTS _tx_interface_files)
    file(SHA256 "${CMAKE_SOURCE_DIR}/${_tx_file}" _tx_digest)
    get_filename_component(_tx_name "${_tx_file}" NAME)
    string(APPEND TX_INTERFACE_RECORDS
        "    {\"${_tx_name}\", \"${_tx_digest}\"},\n")
    math(EXPR TX_INTERFACE_COUNT "${TX_INTERFACE_COUNT} + 1")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/${_tx_file}")
endforeach()
set(_tx_abi_input "tx-abi-v1\n${CMAKE_SYSTEM_NAME}\n${CMAKE_SYSTEM_PROCESSOR}\n")
foreach(_tx_file IN LISTS _tx_abi_files)
    file(SHA256 "${CMAKE_SOURCE_DIR}/${_tx_file}" _tx_digest)
    string(APPEND _tx_abi_input "${_tx_file}:${_tx_digest}\n")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/${_tx_file}")
endforeach()
string(SHA256 TX_ABI_FINGERPRINT "${_tx_abi_input}")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated/driver")
configure_file(src/driver/compatibility_data.hpp.in
    "${CMAKE_BINARY_DIR}/generated/driver/compatibility_data.hpp" @ONLY)
configure_file(src/driver/compatibility_abi.txt.in
    "${CMAKE_BINARY_DIR}/compatibility_abi.txt" @ONLY)
if(MINGW)
    # Mbed TLS 3.6.x uses %I64d on MinGW; match the MSVCRT printf parser.
    foreach(target IN ITEMS mbedtls mbedx509 mbedcrypto)
        target_compile_definitions(${target} PRIVATE __USE_MINGW_ANSI_STDIO=0)
    endforeach()
endif()

add_executable(txc
    src/backend/analysis/program_ir.cpp
    src/backend/analysis/ir_expression.cpp
    src/backend/analysis/ir_statement.cpp
    src/backend/analysis/ir_cleanup.cpp
    src/backend/analysis/ssa.cpp
    src/backend/analysis/ir_verify.cpp
    src/backend/analysis/analysis_sets.cpp
    src/backend/analysis/program_analysis.cpp
    src/backend/analysis/call_analysis.cpp
    src/backend/analysis/builtin_analysis.cpp
    src/backend/analysis/analysis_dump.cpp
    src/common/format_spec.cpp
    src/common/text_encoding.cpp
    src/frontend/lexer/lexer.cpp
    src/frontend/ast/ast.cpp
    src/frontend/ast/call_properties.cpp
    src/frontend/parser/parser.cpp
    src/frontend/parser/parser_declarations.cpp
    src/frontend/parser/parser_serde.cpp
    src/frontend/parser/parser_expression.cpp
    src/frontend/parser/parser_class.cpp
    src/frontend/parser/parser_exception.cpp
    src/frontend/resolver/module_resolver.cpp
    src/frontend/resolver/module_resolver_tree.cpp
    src/frontend/sema/sema.cpp
    src/frontend/sema/sema_vector.cpp
    src/frontend/sema/sema_sum.cpp
    src/frontend/sema/sema_container.cpp
    src/frontend/sema/sema_algorithm.cpp
    src/frontend/sema/sema_thread.cpp
    src/frontend/sema/sema_task.cpp
    src/frontend/sema/sema_sync.cpp
    src/frontend/sema/sema_channel.cpp
    src/frontend/sema/sema_random.cpp
    src/frontend/sema/sema_serde.cpp
    src/frontend/sema/sema_exception.cpp
    src/frontend/sema/sema_registration.cpp
    src/frontend/sema/sema_expression.cpp
    src/frontend/sema/sema_operator.cpp
    src/frontend/sema/sema_call.cpp
    src/frontend/sema/sema_call_properties.cpp
    src/frontend/sema/sema_class.cpp
    src/frontend/sema/sema_class_registration.cpp
    src/frontend/sema/sema_class_call.cpp
    src/backend/llvm/codegen.cpp
    src/backend/llvm/codegen_vector.cpp
    src/backend/llvm/codegen_vector_reference.cpp
    src/backend/llvm/codegen_vector_loop.cpp
    src/backend/llvm/codegen_vector_calls.cpp
    src/backend/llvm/codegen_sum.cpp
    src/backend/llvm/codegen_native_option.cpp
    src/backend/llvm/codegen_native_parse.cpp
    src/backend/llvm/codegen_parse_error.cpp
    src/backend/llvm/codegen_iterator.cpp
    src/backend/llvm/codegen_closure.cpp
    src/backend/llvm/codegen_closure_internal.cpp
    src/backend/llvm/codegen_vector_statement.cpp
    src/backend/llvm/codegen_vector_declarations.cpp
    src/backend/llvm/codegen_algorithm_declarations.cpp
    src/backend/llvm/codegen_algorithm.cpp
    src/backend/llvm/codegen_thread.cpp
    src/backend/llvm/codegen_task.cpp
    src/backend/llvm/codegen_sync.cpp
    src/backend/llvm/codegen_log.cpp
    src/backend/llvm/codegen_channel.cpp
    src/backend/llvm/codegen_serde.cpp
    src/backend/llvm/codegen_format.cpp
    src/backend/llvm/codegen_format_execute.cpp
    src/backend/llvm/codegen_format_dynamic.cpp
    src/backend/llvm/codegen_format_constant.cpp
    src/backend/llvm/codegen_encoding.cpp
    src/backend/llvm/codegen_format_plan.cpp
    src/backend/llvm/codegen_container.cpp
    src/backend/llvm/codegen_key.cpp
    src/backend/llvm/codegen_container_declarations.cpp
    src/backend/llvm/codegen_ordered_declarations.cpp
    src/backend/llvm/codegen_sequence_declarations.cpp
    src/backend/llvm/codegen_map.cpp
    src/backend/llvm/codegen_functions.cpp
    src/backend/llvm/codegen_program_analysis.cpp
    src/backend/llvm/target_attributes.cpp
    src/backend/llvm/codegen_callback.cpp
    src/backend/llvm/codegen_exception.cpp
    src/backend/llvm/codegen_context.cpp
    src/backend/llvm/codegen_error_ownership.cpp
    src/backend/llvm/codegen_declarations.cpp
    src/backend/llvm/codegen_control.cpp
    src/backend/llvm/codegen_statement.cpp
    src/backend/llvm/codegen_unpack.cpp
    src/backend/llvm/codegen_gc.cpp
    src/backend/llvm/codegen_ownership.cpp
    src/backend/llvm/codegen_snapshot.cpp
    src/backend/llvm/codegen_local_array.cpp
    src/backend/llvm/codegen_dynamic_local_array.cpp
    src/backend/llvm/codegen_local_array_analysis.cpp
    src/backend/llvm/codegen_local_object_analysis.cpp
    src/backend/llvm/codegen_array_assignment.cpp
    src/backend/llvm/codegen_expression.cpp
    src/backend/llvm/codegen_cast.cpp
    src/backend/llvm/codegen_dict.cpp
    src/backend/llvm/codegen_expression_ops.cpp
    src/backend/llvm/codegen_division.cpp
    src/backend/llvm/codegen_integer_range.cpp
    src/backend/llvm/codegen_pure_integer.cpp
    src/backend/llvm/codegen_expression_values.cpp
    src/backend/llvm/codegen_expression_calls.cpp
    src/backend/llvm/codegen_class.cpp
    src/backend/llvm/codegen_class_local.cpp
    src/backend/llvm/codegen_fields.cpp
    src/backend/llvm/codegen_record_metadata.cpp
    src/backend/llvm/codegen_record.cpp
    src/backend/llvm/codegen_record_analysis.cpp
    src/backend/llvm/codegen_sync_local.cpp
    src/backend/llvm/codegen_result_projection.cpp
    src/backend/llvm/codegen_result_length.cpp
    src/backend/llvm/codegen_serde_static.cpp
    src/backend/llvm/codegen_record_operations.cpp
    src/backend/analysis/static_function.cpp
    src/backend/llvm/codegen_record_calls.cpp
    src/backend/llvm/codegen_record_assignment.cpp
    src/backend/llvm/codegen_record_method.cpp
    src/backend/llvm/codegen_print.cpp
    src/backend/llvm/codegen_external_direct.cpp
    src/backend/llvm/codegen_db.cpp
    src/backend/llvm/codegen_string_literal_call.cpp
    src/backend/llvm/codegen_string_concat.cpp
    src/backend/llvm/codegen_string_borrow.cpp
    src/backend/llvm/codegen_string_literal_compare.cpp
    src/backend/llvm/codegen_call_binding.cpp
    src/backend/llvm/codegen_literal_spread.cpp
    src/backend/llvm/codegen_call_borrow.cpp
    src/driver/module_loader.cpp
    src/driver/module_loader_class.cpp
    src/driver/compatibility.cpp
    src/backend/llvm/codegen_graphics.cpp
    src/driver/test_runner.cpp
    src/driver/child_process.cpp
    src/driver/json_report.cpp
    src/driver/profile_runner.cpp
    src/driver/main.cpp
)

add_library(txstdlib STATIC
    src/backend/cpp/runtime.cpp
    src/backend/cpp/runtime_abi.cpp
    src/backend/cpp/runtime_diagnostics.cpp
    src/backend/cpp/error_abi.cpp
    src/backend/cpp/runtime_context.cpp
    src/backend/cpp/test_abi.cpp
    src/backend/cpp/test_property_abi.cpp
    src/backend/cpp/test_fixture_abi.cpp
    src/backend/cpp/log_abi.cpp
    src/backend/cpp/debug_abi.cpp
    src/backend/cpp/profile_abi.cpp
    src/backend/cpp/parse_abi.cpp
    src/backend/cpp/parse_error_abi.cpp
    src/backend/cpp/json_abi.cpp
    src/backend/cpp/json_stream_abi.cpp
    src/backend/cpp/cbor_abi.cpp
    src/backend/cpp/serde_abi.cpp
    src/backend/cpp/serde_static_abi.cpp
    src/backend/cpp/csv_abi.cpp
    src/backend/cpp/xml_reader_abi.cpp
    src/backend/cpp/xml_tree_abi.cpp
    src/backend/cpp/xml_writer_abi.cpp
    src/backend/cpp/network_abi.cpp
    src/backend/cpp/dns_abi.cpp
    src/backend/cpp/socket_abi.cpp
    src/backend/cpp/socket_async_abi.cpp
    src/backend/cpp/socket_async_loop.cpp
    src/backend/cpp/socket_async_tcp.cpp
    src/backend/cpp/socket_async_udp.cpp
    src/backend/cpp/network_stream_abi.cpp
    src/backend/cpp/network_http2_abi.cpp
    src/backend/cpp/network_session_abi.cpp
    src/backend/cpp/network_router_abi.cpp
    src/backend/cpp/network_http3_abi.cpp
    src/backend/cpp/ws_abi.cpp
    src/backend/cpp/file_result_abi.cpp
    src/backend/cpp/bytes_abi.cpp
    src/backend/cpp/crypto_abi.cpp
    src/backend/cpp/crypto_stream_file_abi.cpp
    src/backend/cpp/secret_abi.cpp
    src/backend/cpp/password_abi.cpp
    src/backend/cpp/public_key_abi.cpp
    src/backend/cpp/x509_abi.cpp
    src/backend/cpp/tls_abi.cpp
    src/backend/cpp/tls_stream_abi.cpp
    src/backend/cpp/encoding_memory_abi.cpp
    src/backend/cpp/encoding_incremental_abi.cpp
    src/backend/cpp/unicode_abi.cpp
    src/backend/cpp/regex_abi.cpp
    src/backend/cpp/file_stream_abi.cpp
    src/backend/cpp/text_reference.cpp
    src/backend/cpp/string_concat_abi.cpp
    src/backend/cpp/dictionary_string_query.cpp
    src/backend/cpp/vector_i64_abi.cpp
    src/backend/cpp/vector_f64_abi.cpp
    src/backend/cpp/vector_bool_abi.cpp
    src/backend/cpp/vector_str_abi.cpp
    src/backend/cpp/vector_bytes_abi.cpp
    src/backend/cpp/vector_object_abi.cpp
    src/backend/cpp/sum_abi.cpp
    src/backend/cpp/iterator_abi.cpp
    src/backend/cpp/closure_abi.cpp
    src/backend/cpp/typed_slots.cpp
    src/backend/cpp/record_layout.cpp
    src/backend/cpp/record_abi.cpp
    src/backend/cpp/cancel_abi.cpp
    src/backend/cpp/thread_abi.cpp
    src/backend/cpp/task_runtime.cpp
    src/backend/cpp/task_iocp.cpp
    src/backend/cpp/task_file_iocp.cpp
    src/backend/cpp/task_executor.cpp
    src/backend/cpp/task_abi.cpp
    src/backend/cpp/sync_lock_abi.cpp
    src/backend/cpp/sync_local_abi.cpp
    src/backend/cpp/sync_lock_generic_abi.cpp
    src/backend/cpp/sync_primitive_abi.cpp
    src/backend/cpp/sync_atomic_abi.cpp
    src/backend/cpp/channel_abi.cpp
    src/backend/cpp/concurrency_value.cpp
    src/backend/cpp/concurrency_result.cpp
    src/backend/cpp/process_abi.cpp
    src/backend/cpp/process_io_abi.cpp
    src/backend/cpp/ipc_abi.cpp
    src/backend/cpp/db_abi.cpp
    src/backend/cpp/db_pool_abi.cpp
    src/backend/cpp/db_migration_abi.cpp
    src/backend/cpp/db_async_abi.cpp
    src/backend/cpp/db_resource_abi.cpp
    src/backend/cpp/db_value_abi.cpp
    src/backend/cpp/db_row_abi.cpp
    src/backend/cpp/db_domain_abi.cpp
    src/backend/cpp/vector_text_abi.cpp
    src/backend/cpp/algorithm_abi.cpp
    src/backend/cpp/algorithm_extended_abi.cpp
    src/backend/cpp/map_i64_abi.cpp
    src/backend/cpp/map_f64_abi.cpp
    src/backend/cpp/map_bool_abi.cpp
    src/backend/cpp/map_str_abi.cpp
    src/backend/cpp/set_abi.cpp
    src/backend/cpp/object_key_abi.cpp
    src/backend/cpp/heap_abi.cpp
    src/backend/cpp/queue_abi.cpp
    src/backend/cpp/deque_abi.cpp
    src/backend/cpp/map_entries_abi.cpp
    src/backend/cpp/map_object_abi.cpp
    src/backend/cpp/ordered_abi.cpp
    src/backend/cpp/priority_entry_abi.cpp
    src/backend/cpp/sequence_extended_abi.cpp
    src/backend/cpp/cycle_gc.cpp
    src/backend/cpp/text_direct_abi.cpp
    src/backend/cpp/array_direct_abi.cpp
    src/backend/cpp/dictionary_direct_abi.cpp
    src/backend/cpp/dictionary_scalar_abi.cpp
    src/backend/cpp/array_abi.cpp
    src/backend/cpp/local_scalar_array_abi.cpp
    src/backend/cpp/filesystem_direct_abi.cpp
    src/backend/cpp/filesystem_extended_abi.cpp
    src/backend/cpp/system_env_direct_abi.cpp
    src/backend/cpp/file_direct_abi.cpp
    src/backend/cpp/format_direct_abi.cpp
    src/backend/cpp/format_arguments_abi.cpp
    src/backend/cpp/format_static_abi.cpp
    src/backend/cpp/io_time_direct_abi.cpp
    src/backend/cpp/value_abi.cpp
    src/backend/cpp/value_snapshot_abi.cpp
    src/backend/cpp/deep_copy.cpp
    src/backend/cpp/deep_copy_abi.cpp
    src/backend/cpp/deep_copy_resources.cpp
    src/backend/cpp/class_abi.cpp
    src/backend/cpp/class_local_abi.cpp
    src/backend/cpp/value_format.cpp
    src/backend/cpp/dictionary_abi.cpp
    src/backend/cpp/dictionary_ref_abi.cpp
    src/backend/cpp/call_abi.cpp
    src/backend/cpp/external_abi.cpp
    src/backend/cpp/external_abi_time_random.cpp
    src/stdlib/string.cpp
    src/stdlib/format.cpp
    src/stdlib/format_append.cpp
    src/stdlib/format_plan_cache.cpp
    src/stdlib/format_binding_cache.cpp
    src/stdlib/format_spec.cpp
    src/common/format_spec.cpp
    src/common/text_encoding.cpp
    src/common/utf8.cpp
    src/common/utf8_bulk.cpp
    src/stdlib/dictionary.cpp
    src/stdlib/conversion.cpp
    src/stdlib/parse.cpp
    src/stdlib/json_parser.cpp
    src/stdlib/json_parser_string.cpp
    src/stdlib/json_stream.cpp
    src/stdlib/json_schema.cpp
    src/stdlib/cbor_common.cpp
    src/stdlib/cbor_reader.cpp
    src/stdlib/cbor_skip.cpp
    src/stdlib/cbor_writer.cpp
    src/stdlib/serde_schema.cpp
    src/stdlib/serde_value.cpp
    src/stdlib/serde.cpp
    src/stdlib/serde_reader.cpp
    src/stdlib/serde_writer.cpp
    src/stdlib/serde_codecs.cpp
    src/stdlib/serde_vector.cpp
    src/stdlib/serde_struct_reader.cpp
    src/stdlib/serde_scalar_reader.cpp
    src/stdlib/serde_pair.cpp
    src/stdlib/serde_struct_writer.cpp
    src/stdlib/cbor_stream.cpp
    src/stdlib/format_stream.cpp
    src/stdlib/csv.cpp
    src/stdlib/csv_reader.cpp
    src/stdlib/csv_writer.cpp
    src/stdlib/xml_common.cpp
    src/stdlib/xml_reader.cpp
    src/stdlib/xml_tree_parse.cpp
    src/stdlib/xml_tree_nodes.cpp
    src/stdlib/xml_writer.cpp
    src/stdlib/json_writer.cpp
    src/stdlib/json_scalar.cpp
    src/stdlib/json_skip.cpp
    src/stdlib/json_fields.cpp
    src/stdlib/log.cpp
    src/stdlib/log_sink.cpp
    src/stdlib/test.cpp
    src/stdlib/profile.cpp
    src/stdlib/profile_sampler.cpp
    src/stdlib/profile_report.cpp
    src/stdlib/profile_allocation.cpp
    src/stdlib/io.cpp
    src/stdlib/encoding.cpp
    src/stdlib/encoding_incremental.cpp
    src/stdlib/unicode_text.cpp
    src/stdlib/unicode_analysis.cpp
    src/stdlib/regex_compile.cpp
    src/stdlib/regex_match.cpp
    src/stdlib/regex_operations.cpp
    src/stdlib/bytes.cpp
    src/stdlib/crypto_random.cpp
    src/stdlib/crypto_hash.cpp
    src/stdlib/crypto_kdf.cpp
    src/stdlib/crypto_aead.cpp
    src/stdlib/crypto_stream_hash.cpp
    src/stdlib/crypto_file_io.cpp
    src/stdlib/crypto_file.cpp
    src/stdlib/secret.cpp
    src/stdlib/password.cpp
    src/stdlib/public_key.cpp
    src/stdlib/x509_parse.cpp
    src/stdlib/x509_pkcs12.cpp
    src/stdlib/x509_pkcs8.cpp
    src/stdlib/x509_verify.cpp
    src/stdlib/x509_verification_context.cpp
    src/stdlib/tls_identity.cpp
    src/stdlib/tls_material.cpp
    src/stdlib/tls_stream_handshake.cpp
    src/stdlib/tls_stream_io.cpp
    src/stdlib/tls_stream_registry.cpp
    src/stdlib/libsodium_compat.cpp
    src/stdlib/file_stream.cpp
    src/stdlib/file_stream_lock.cpp
    src/stdlib/file_stream_text.cpp
    src/stdlib/file.cpp
    src/stdlib/file_binary.cpp
    src/stdlib/file_atomic.cpp
    src/stdlib/math.cpp
    src/stdlib/math_extended.cpp
    src/stdlib/statistics_core.cpp
    src/stdlib/statistics_bucket.cpp
    src/stdlib/decimal_big.cpp
    src/stdlib/decimal.cpp
    src/stdlib/decimal_abi.cpp
    src/stdlib/array.cpp
    src/stdlib/algorithm_numeric.cpp
    src/stdlib/filesystem.cpp
    src/stdlib/filesystem_extended.cpp
    src/stdlib/filesystem_watch.cpp
    src/stdlib/process.cpp
    src/stdlib/process_options.cpp
    src/stdlib/process_spawn.cpp
    src/stdlib/process_pipe.cpp
    src/stdlib/process_run.cpp
    src/stdlib/ipc_connection.cpp
    src/stdlib/ipc_transport.cpp
    src/stdlib/ipc_frame.cpp
    src/stdlib/db_values.cpp
    src/stdlib/db_resources.cpp
    src/stdlib/db_connection.cpp
    src/stdlib/db_binding.cpp
    src/stdlib/db_query.cpp
    src/stdlib/db_transaction.cpp
    src/stdlib/db_postgres_connection.cpp
    src/stdlib/db_postgres_io.cpp
    src/stdlib/db_postgres_cancel.cpp
    src/stdlib/db_postgres_statement.cpp
    src/stdlib/db_postgres_rows.cpp
    src/stdlib/db_pool.cpp
    src/stdlib/db_migration.cpp
    src/stdlib/db_operation.cpp
    src/stdlib/db_sqlite_operation.cpp
    src/stdlib/db_async.cpp
    src/stdlib/db_pool_return.cpp
    src/stdlib/filesystem_listing.cpp
    src/stdlib/filesystem_operations.cpp
    src/stdlib/path.cpp
    src/stdlib/system.cpp
    src/stdlib/system_info.cpp
    src/stdlib/env.cpp
    src/stdlib/env_snapshot.cpp
    src/stdlib/time.cpp
    src/stdlib/time_values.cpp
    src/stdlib/time_calendar.cpp
    src/stdlib/time_calendar_abi.cpp
    src/stdlib/time_zone.cpp
    src/stdlib/random.cpp
    src/stdlib/random_generator.cpp
    src/stdlib/random_sampling.cpp
    src/stdlib/network_common.cpp
    src/stdlib/dns.cpp
    src/stdlib/socket_core.cpp
    src/stdlib/socket_tcp.cpp
    src/stdlib/socket_udp.cpp
    src/stdlib/network_listener.cpp
    src/stdlib/network_url.cpp
    src/stdlib/httpx_client.cpp
    src/stdlib/httpx_client_custom_tls.cpp
    src/stdlib/httpx_client_custom_tls_pool.cpp
    src/stdlib/httpx_client_custom_tls_proxy.cpp
    src/stdlib/httpx_client_custom_tls_response.cpp
    src/stdlib/httpx_client_custom_tls_transport.cpp
    src/stdlib/httpx_winhttp.cpp
    src/stdlib/httpx_client_session.cpp
    src/stdlib/httpx_client_tls.cpp
    src/stdlib/httpx_negotiation.cpp
    src/stdlib/httpx_router.cpp
    src/stdlib/httpx_server_listener.cpp
    src/stdlib/http3_quic.cpp
    src/stdlib/http3_client.cpp
    src/stdlib/http3_client_events.cpp
    src/stdlib/http3_client_headers.cpp
    src/stdlib/http3_server_listener.cpp
    src/stdlib/http3_server_connection.cpp
    src/stdlib/http3_server_events.cpp
    src/stdlib/http3_server_headers.cpp
    src/stdlib/http3_server.cpp
    src/stdlib/httpx_server.cpp
    src/stdlib/http2_transport.cpp
    src/stdlib/http2_session.cpp
    src/stdlib/http2_session_response.cpp
    src/stdlib/http2_server.cpp
    src/stdlib/http2_client.cpp
    src/stdlib/http2_client_callbacks.cpp
    src/stdlib/http2_client_stream.cpp
    src/stdlib/ws.cpp
    src/stdlib/ws_client.cpp
    src/stdlib/ws_client_stream.cpp
    src/stdlib/ws_server.cpp
    src/stdlib/ws_frames.cpp
)

target_compile_features(txc PRIVATE cxx_std_23)
include(cmake/platform_sources.cmake)
target_compile_features(txstdlib PRIVATE cxx_std_23)
target_sources(txstdlib PRIVATE src/backend/cpp/graphics_resource_abi.cpp)
target_sources(txstdlib PRIVATE
    src/stdlib/native_gui/core/path.cpp
    src/stdlib/native_gui/core/pixel_buffer.cpp
    src/stdlib/native_gui/core/raster.cpp
    src/stdlib/native_gui/core/font_reader.cpp
    src/stdlib/native_gui/core/font_cmap.cpp
    src/stdlib/native_gui/core/font_outline.cpp
    src/stdlib/native_gui/core/font_compound.cpp
    src/stdlib/native_gui/core/font_cache.cpp
    src/stdlib/native_gui/core/font_family.cpp
    src/stdlib/native_gui/core/opentype_tables.cpp
    src/stdlib/native_gui/core/opentype_processor.cpp
    src/stdlib/native_gui/core/opentype_substitution.cpp
    src/stdlib/native_gui/core/opentype_context.cpp
    src/stdlib/native_gui/core/opentype_position.cpp
    src/stdlib/native_gui/core/opentype_attachment.cpp
    src/stdlib/native_gui/core/opentype_carets.cpp
    src/stdlib/native_gui/core/shaping.cpp
    src/stdlib/native_gui/core/shaping_scripts.cpp
    src/stdlib/native_gui/core/unicode.cpp
    src/stdlib/native_gui/core/unicode_properties.cpp
    src/stdlib/native_gui/core/grapheme.cpp
    src/stdlib/native_gui/core/word_boundary.cpp
    src/stdlib/native_gui/core/text_buffer.cpp
    src/stdlib/native_gui/core/text_navigation.cpp
    src/stdlib/native_gui/core/text_layout.cpp
    src/stdlib/native_gui/core/text_shaping.cpp
    src/stdlib/native_gui/core/text_caret.cpp
    src/stdlib/native_gui/core/bidi.cpp
    src/stdlib/native_gui/core/bidi_sequence.cpp
    src/stdlib/native_gui/core/bidi_properties.cpp
    src/stdlib/native_gui/core/line_break.cpp
    src/stdlib/native_gui/core/line_break_rules.cpp
    src/stdlib/native_gui/core/image_io.cpp
    src/backend/cpp/native_gui_abi.cpp
    src/backend/cpp/native_gui_control_abi.cpp
    src/backend/cpp/native_gui_application_abi.cpp
    src/backend/cpp/native_gui_layout_abi.cpp
    src/backend/cpp/native_gui_style_abi.cpp
    src/backend/cpp/native_gui_text_abi.cpp
    src/backend/cpp/native_gui_range_abi.cpp
    src/stdlib/native_gui/application.cpp
    src/stdlib/native_gui/resources.cpp
    src/stdlib/native_gui/layout.cpp
    src/stdlib/native_gui/arrange.cpp
    src/stdlib/native_gui/grid.cpp
    src/stdlib/gui/layout.cpp
    src/stdlib/native_gui/input.cpp
    src/stdlib/native_gui/drawing.cpp)
target_sources(txstdlib PRIVATE
    src/stdlib/native_gui/text_edit.cpp
    src/stdlib/native_gui/text_keys.cpp
    src/stdlib/native_gui/text_draw.cpp
    src/stdlib/native_gui/control_actions.cpp)
target_sources(txstdlib PRIVATE
    src/stdlib/native_gui/window_actions.cpp
    src/stdlib/native_gui/combo.cpp
    src/stdlib/native_gui/combo_draw.cpp
    src/backend/cpp/native_gui_interaction_abi.cpp)
target_sources(txstdlib PRIVATE
    src/stdlib/native_gui/containers.cpp
    src/stdlib/native_gui/container_layout.cpp
    src/stdlib/native_gui/container_input.cpp
    src/stdlib/native_gui/container_draw.cpp
    src/backend/cpp/native_gui_container_abi.cpp
    src/backend/cpp/native_gui_canvas_abi.cpp)
target_sources(txstdlib PRIVATE
    src/stdlib/native_gui/data_model.cpp
    src/stdlib/native_gui/data_columns.cpp
    src/stdlib/native_gui/data_view.cpp
    src/stdlib/native_gui/data_layout.cpp
    src/stdlib/native_gui/data_input.cpp
    src/stdlib/native_gui/data_draw.cpp
    src/backend/cpp/native_gui_data_abi.cpp
    src/backend/cpp/native_gui_data_query_abi.cpp)
target_sources(txstdlib PRIVATE
    src/stdlib/native_gui/commands.cpp
    src/stdlib/native_gui/menus.cpp
    src/stdlib/native_gui/menu_input.cpp
    src/stdlib/native_gui/menu_draw.cpp
    src/stdlib/native_gui/dialogs.cpp
    src/stdlib/native_gui/theme.cpp
    src/stdlib/native_gui/accessibility.cpp
    src/stdlib/native_gui/accessibility_tree.cpp
    src/stdlib/native_gui/accessibility_text.cpp
    src/stdlib/native_gui/accessibility_menus.cpp
    src/backend/cpp/native_gui_command_abi.cpp
    src/backend/cpp/native_gui_system_abi.cpp)
if(WIN32)
    target_sources(txstdlib PRIVATE
        src/stdlib/native_gui/platform/windows_services.cpp
        src/stdlib/native_gui/platform/windows_theme.cpp
        src/stdlib/native_gui/platform/windows_accessibility.cpp
        src/stdlib/native_gui/platform/windows_accessibility_navigation.cpp
        src/stdlib/native_gui/platform/windows_accessibility_patterns.cpp
        src/stdlib/native_gui/platform/windows_accessibility_text.cpp
        src/stdlib/native_gui/platform/windows_accessibility_units.cpp
        src/stdlib/native_gui/platform/windows_window.cpp
        src/stdlib/native_gui/platform/windows_input.cpp
        src/stdlib/native_gui/platform/windows_clipboard.cpp
        src/backend/cpp/graphics_window_abi.cpp
        src/backend/cpp/graphics_event_abi.cpp
        src/backend/cpp/graphics_draw_abi.cpp
        src/stdlib/graphics/windows/application.cpp
        src/stdlib/graphics/windows/resources.cpp
        src/stdlib/graphics/windows/events.cpp
        src/stdlib/graphics/windows/frames.cpp
        src/stdlib/graphics/windows/assets.cpp
        src/stdlib/graphics/windows/images.cpp
        src/stdlib/graphics/windows/surfaces.cpp
        src/stdlib/graphics/windows/png.cpp
        src/stdlib/graphics/windows/drawing.cpp
        src/stdlib/graphics/windows/input.cpp
        src/stdlib/graphics/windows/ime.cpp
        src/backend/cpp/graphics_path_abi.cpp
        src/backend/cpp/graphics_state_abi.cpp
        src/backend/cpp/graphics_brush_abi.cpp
        src/backend/cpp/graphics_brush_shapes_abi.cpp
        src/backend/cpp/graphics_image_abi.cpp
        src/backend/cpp/graphics_asset_abi.cpp
        src/backend/cpp/graphics_text_abi.cpp
        src/backend/cpp/graphics_input_abi.cpp
        src/backend/cpp/gui_control_abi.cpp
        src/backend/cpp/gui_form_abi.cpp
        src/backend/cpp/gui_theme_abi.cpp
        src/stdlib/gui/windows/theme.cpp
        src/backend/cpp/gui_complex_abi.cpp
        src/backend/cpp/gui_canvas_abi.cpp
        src/backend/cpp/gui_accessibility_abi.cpp
        src/stdlib/gui/windows/complex.cpp
        src/stdlib/gui/windows/complex_layout.cpp
        src/stdlib/gui/windows/complex_events.cpp
        src/stdlib/gui/windows/canvas.cpp
        src/stdlib/gui/windows/accessibility.cpp
        src/stdlib/gui/windows/accessibility_provider.cpp
        src/backend/cpp/gui_length_abi.cpp
        src/stdlib/gui/windows/commands.cpp
        src/stdlib/gui/windows/menus.cpp
        src/stdlib/gui/windows/shortcuts.cpp
        src/stdlib/gui/windows/dialogs.cpp
        src/stdlib/gui/windows/clipboard.cpp
        src/backend/cpp/gui_command_abi.cpp
        src/backend/cpp/gui_dialog_abi.cpp
        src/stdlib/gui/windows/models.cpp
        src/stdlib/gui/windows/views.cpp
        src/stdlib/gui/windows/data_events.cpp
        src/backend/cpp/gui_data_create_abi.cpp
        src/backend/cpp/gui_data_edit_abi.cpp
        src/backend/cpp/gui_data_query_abi.cpp
        src/stdlib/gui/windows/resources.cpp
        src/stdlib/gui/windows/properties.cpp
        src/stdlib/gui/windows/text.cpp
        src/stdlib/gui/windows/events.cpp
        src/stdlib/gui/windows/focus.cpp
        src/stdlib/gui/windows/measure.cpp
        src/stdlib/gui/windows/arrange.cpp)
    target_link_libraries(txstdlib PRIVATE user32 gdi32 d2d1 dwrite windowscodecs imm32 ole32 uuid comctl32 shell32 comdlg32)
else()
    target_sources(txstdlib PRIVATE
        src/stdlib/native_gui/platform/x11_connection.cpp
        src/stdlib/native_gui/platform/linux_services.cpp
        src/stdlib/native_gui/platform/linux_bus.cpp
        src/stdlib/native_gui/platform/linux_theme.cpp
        src/stdlib/native_gui/platform/linux_file_dialog.cpp
        src/stdlib/native_gui/platform/linux_accessibility.cpp
        src/stdlib/native_gui/platform/linux_accessibility_properties.cpp
        src/stdlib/native_gui/platform/linux_accessibility_methods.cpp
        src/stdlib/native_gui/platform/linux_accessibility_accessible.cpp
        src/stdlib/native_gui/platform/linux_accessibility_component.cpp
        src/stdlib/native_gui/platform/linux_accessibility_selection.cpp
        src/stdlib/native_gui/platform/linux_accessibility_text.cpp
        src/stdlib/native_gui/platform/linux_accessibility_editable.cpp
        src/stdlib/native_gui/platform/linux_accessibility_text_selection.cpp
        src/stdlib/native_gui/platform/linux_accessibility_text_geometry.cpp
        src/stdlib/native_gui/platform/linux_accessibility_schema.cpp
        src/stdlib/native_gui/platform/x11_setup.cpp
        src/stdlib/native_gui/platform/x11_operations.cpp
        src/stdlib/native_gui/platform/xim_transport.cpp
        src/stdlib/native_gui/platform/xim_wire.cpp
        src/stdlib/native_gui/platform/xim_session.cpp
        src/stdlib/native_gui/platform/xim_input.cpp
        src/stdlib/native_gui/platform/xim_messages.cpp
        src/stdlib/native_gui/platform/linux_window.cpp
        src/stdlib/native_gui/platform/linux_keyboard.cpp
        src/stdlib/native_gui/platform/linux_input.cpp)
    target_sources(txstdlib PRIVATE src/stdlib/native_gui/platform/linux_clipboard.cpp)
endif()
target_include_directories(txc PRIVATE src "${CMAKE_BINARY_DIR}/generated")
target_include_directories(txstdlib PRIVATE src)
if(WIN32)
    target_sources(txc PRIVATE src/driver/native_toolchain_windows.cpp)
target_include_directories(txstdlib PRIVATE
    "${icu_binary_SOURCE_DIR}/mingw64/include"
    "${pcre2_binary_SOURCE_DIR}/mingw64/include"
    "${libsodium_binary_SOURCE_DIR}/mingw64/include")
target_compile_definitions(txstdlib PRIVATE PCRE2_STATIC)
target_include_directories(txstdlib PRIVATE
    "${msquic_binary_SOURCE_DIR}/build/native/include")
else()
    target_sources(txc PRIVATE src/driver/native_toolchain_linux.cpp)
    target_link_libraries(txc PRIVATE OpenSSL::Crypto)
    target_include_directories(txstdlib PRIVATE "${msquic_headers_SOURCE_DIR}/src/inc")
    target_link_libraries(txstdlib PRIVATE ICU::uc ICU::i18n ICU::data
        PkgConfig::TX_PCRE2 PkgConfig::TX_CURL PkgConfig::TX_CARES
        OpenSSL::SSL OpenSSL::Crypto Threads::Threads ${CMAKE_DL_LIBS})
    set_target_properties(txc PROPERTIES BUILD_RPATH "$ORIGIN/lib")
endif()
target_link_libraries(txstdlib PRIVATE nghttp2_static nghttp3_static
    mbedtls mbedx509 mbedcrypto
    argon2_reference_static LibXml2::LibXml2 libsodium_static tx_sqlite_static tx_libpq)
if(WIN32)
    add_dependencies(txc txstdlib)
    target_link_libraries(txc PRIVATE shell32 bcrypt)
    target_link_libraries(txstdlib PRIVATE winhttp ws2_32 dnsapi advapi32 bcrypt crypt32 ncrypt)
endif()
set(TX_TOOL_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/tx" CACHE PATH "Packaged compiler output directory")
set_target_properties(txstdlib PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
set_target_properties(txc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TX_TOOL_OUTPUT_DIR}"
)

if(MSVC)
    target_compile_options(txc PRIVATE /W4 /utf-8)
    target_compile_options(txstdlib PRIVATE /W4 /utf-8)
else()
    target_compile_options(txc PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(txstdlib PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # 数字键读取的 any 恢复和哈希查找应内联进静态 ABI，避免额外短调用。
    set_source_files_properties(
        src/backend/cpp/map_i64_abi.cpp src/backend/cpp/map_f64_abi.cpp
        src/backend/cpp/map_bool_abi.cpp src/backend/cpp/map_str_abi.cpp
        PROPERTIES COMPILE_OPTIONS "-finline-limit=1000")
endif()
