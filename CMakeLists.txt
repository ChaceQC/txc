cmake_minimum_required(VERSION 3.21)
project(tx_compiler LANGUAGES C CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_LIB_ONLY ON CACHE BOOL "" FORCE)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
foreach(package IN ITEMS OpenSSL Libngtcp2 Libnghttp3 Systemd Jansson
        Libevent LibXml2 Jemalloc)
    set(CMAKE_DISABLE_FIND_PACKAGE_${package} TRUE)
endforeach()
FetchContent_Declare(nghttp2
    URL https://github.com/nghttp2/nghttp2/releases/download/v1.68.0/nghttp2-1.68.0.tar.xz
    URL_HASH SHA256=5511d3128850e01b5b26ec92bf39df15381c767a63441438b25ad6235def902c
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(mbedtls
    URL https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.5/mbedtls-3.6.5.tar.bz2
    URL_HASH SHA256=4a11f1777bb95bf4ad96721cac945a26e04bf19f57d905f241fe77ebeddf46d8
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(_tx_warn_deprecated ${CMAKE_WARN_DEPRECATED})
set(CMAKE_WARN_DEPRECATED OFF)
FetchContent_MakeAvailable(nghttp2 mbedtls)
set(CMAKE_WARN_DEPRECATED ${_tx_warn_deprecated})

# 编译器内嵌公开接口和 ABI 的内容摘要；增删或修改文件都会触发重新配置。
file(GLOB _tx_interface_files CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tx/stdlib/*.txh")
file(GLOB _tx_abi_files CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src/backend/cpp/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/backend/llvm/*.cpp")
list(APPEND _tx_abi_files "src/common/error_kind.hpp")
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
set(_tx_abi_input "tx-abi-v1\n")
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
    src/frontend/lexer/lexer.cpp
    src/frontend/ast/ast.cpp
    src/frontend/parser/parser.cpp
    src/frontend/parser/parser_declarations.cpp
    src/frontend/parser/parser_expression.cpp
    src/frontend/parser/parser_class.cpp
    src/frontend/parser/parser_exception.cpp
    src/frontend/resolver/module_resolver.cpp
    src/frontend/resolver/module_resolver_tree.cpp
    src/frontend/sema/sema.cpp
    src/frontend/sema/sema_vector.cpp
    src/frontend/sema/sema_sum.cpp
    src/frontend/sema/sema_container.cpp
    src/frontend/sema/sema_exception.cpp
    src/frontend/sema/sema_registration.cpp
    src/frontend/sema/sema_expression.cpp
    src/frontend/sema/sema_operator.cpp
    src/frontend/sema/sema_call.cpp
    src/frontend/sema/sema_class.cpp
    src/frontend/sema/sema_class_registration.cpp
    src/frontend/sema/sema_class_call.cpp
    src/backend/llvm/codegen.cpp
    src/backend/llvm/codegen_vector.cpp
    src/backend/llvm/codegen_vector_calls.cpp
    src/backend/llvm/codegen_sum.cpp
    src/backend/llvm/codegen_iterator.cpp
    src/backend/llvm/codegen_closure.cpp
    src/backend/llvm/codegen_vector_statement.cpp
    src/backend/llvm/codegen_vector_declarations.cpp
    src/backend/llvm/codegen_algorithm_declarations.cpp
    src/backend/llvm/codegen_container.cpp
    src/backend/llvm/codegen_key.cpp
    src/backend/llvm/codegen_container_declarations.cpp
    src/backend/llvm/codegen_map.cpp
    src/backend/llvm/codegen_functions.cpp
    src/backend/llvm/codegen_callback.cpp
    src/backend/llvm/codegen_exception.cpp
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
    src/backend/llvm/codegen_array_assignment.cpp
    src/backend/llvm/codegen_expression.cpp
    src/backend/llvm/codegen_cast.cpp
    src/backend/llvm/codegen_dict.cpp
    src/backend/llvm/codegen_expression_ops.cpp
    src/backend/llvm/codegen_expression_values.cpp
    src/backend/llvm/codegen_expression_calls.cpp
    src/backend/llvm/codegen_class.cpp
    src/backend/llvm/codegen_fields.cpp
    src/backend/llvm/codegen_print.cpp
    src/backend/llvm/codegen_external_direct.cpp
    src/backend/llvm/codegen_string_literal_call.cpp
    src/backend/llvm/codegen_string_concat.cpp
    src/backend/llvm/codegen_string_borrow.cpp
    src/backend/llvm/codegen_string_literal_compare.cpp
    src/backend/llvm/codegen_call_binding.cpp
    src/driver/module_loader.cpp
    src/driver/module_loader_class.cpp
    src/driver/compatibility.cpp
    src/driver/test_runner.cpp
    src/driver/main.cpp
)

add_library(txstdlib STATIC
    src/backend/cpp/runtime.cpp
    src/backend/cpp/runtime_abi.cpp
    src/backend/cpp/error_abi.cpp
    src/backend/cpp/test_abi.cpp
    src/backend/cpp/log_abi.cpp
    src/backend/cpp/debug_abi.cpp
    src/backend/cpp/parse_abi.cpp
    src/backend/cpp/json_abi.cpp
    src/backend/cpp/network_abi.cpp
    src/backend/cpp/network_stream_abi.cpp
    src/backend/cpp/network_http2_abi.cpp
    src/backend/cpp/ws_abi.cpp
    src/backend/cpp/file_result_abi.cpp
    src/backend/cpp/bytes_abi.cpp
    src/backend/cpp/crypto_abi.cpp
    src/backend/cpp/encoding_memory_abi.cpp
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
    src/backend/cpp/cancel_abi.cpp
    src/backend/cpp/vector_text_abi.cpp
    src/backend/cpp/algorithm_abi.cpp
    src/backend/cpp/map_i64_abi.cpp
    src/backend/cpp/map_f64_abi.cpp
    src/backend/cpp/map_bool_abi.cpp
    src/backend/cpp/map_str_abi.cpp
    src/backend/cpp/set_abi.cpp
    src/backend/cpp/object_key_abi.cpp
    src/backend/cpp/heap_abi.cpp
    src/backend/cpp/queue_abi.cpp
    src/backend/cpp/cycle_gc.cpp
    src/backend/cpp/text_direct_abi.cpp
    src/backend/cpp/array_direct_abi.cpp
    src/backend/cpp/dictionary_direct_abi.cpp
    src/backend/cpp/array_abi.cpp
    src/backend/cpp/local_scalar_array_abi.cpp
    src/backend/cpp/filesystem_direct_abi.cpp
    src/backend/cpp/system_env_direct_abi.cpp
    src/backend/cpp/file_direct_abi.cpp
    src/backend/cpp/format_direct_abi.cpp
    src/backend/cpp/io_time_direct_abi.cpp
    src/backend/cpp/value_abi.cpp
    src/backend/cpp/value_snapshot_abi.cpp
    src/backend/cpp/deep_copy.cpp
    src/backend/cpp/class_abi.cpp
    src/backend/cpp/value_format.cpp
    src/backend/cpp/dictionary_abi.cpp
    src/backend/cpp/dictionary_ref_abi.cpp
    src/backend/cpp/call_abi.cpp
    src/backend/cpp/external_abi.cpp
    src/backend/cpp/external_abi_time_random.cpp
    src/stdlib/string.cpp
    src/stdlib/format.cpp
    src/stdlib/format_spec.cpp
    src/stdlib/dictionary.cpp
    src/stdlib/conversion.cpp
    src/stdlib/parse.cpp
    src/stdlib/json_parser.cpp
    src/stdlib/json_writer.cpp
    src/stdlib/json_fields.cpp
    src/stdlib/log.cpp
    src/stdlib/test.cpp
    src/stdlib/io.cpp
    src/stdlib/encoding.cpp
    src/stdlib/bytes.cpp
    src/stdlib/crypto_random.cpp
    src/stdlib/crypto_hash.cpp
    src/stdlib/crypto_kdf.cpp
    src/stdlib/crypto_aead.cpp
    src/stdlib/file_stream.cpp
    src/stdlib/file_stream_text.cpp
    src/stdlib/file.cpp
    src/stdlib/math.cpp
    src/stdlib/array.cpp
    src/stdlib/algorithm_numeric.cpp
    src/stdlib/filesystem.cpp
    src/stdlib/filesystem_listing.cpp
    src/stdlib/filesystem_operations.cpp
    src/stdlib/path.cpp
    src/stdlib/system.cpp
    src/stdlib/env.cpp
    src/stdlib/time.cpp
    src/stdlib/random.cpp
    src/stdlib/network_common.cpp
    src/stdlib/network_listener.cpp
    src/stdlib/network_url.cpp
    src/stdlib/httpx_client.cpp
    src/stdlib/httpx_server.cpp
    src/stdlib/http2_transport.cpp
    src/stdlib/http2_session.cpp
    src/stdlib/http2_session_response.cpp
    src/stdlib/http2_server.cpp
    src/stdlib/http2_client.cpp
    src/stdlib/http2_client_callbacks.cpp
    src/stdlib/ws.cpp
    src/stdlib/ws_client.cpp
    src/stdlib/ws_client_stream.cpp
    src/stdlib/ws_server.cpp
    src/stdlib/ws_frames.cpp
)

target_compile_features(txc PRIVATE cxx_std_23)
target_compile_features(txstdlib PRIVATE cxx_std_23)
target_include_directories(txc PRIVATE src "${CMAKE_BINARY_DIR}/generated")
target_include_directories(txstdlib PRIVATE src)
target_link_libraries(txstdlib PRIVATE nghttp2_static mbedtls mbedx509 mbedcrypto)
add_dependencies(txc txstdlib)
if(WIN32)
    target_link_libraries(txc PRIVATE shell32 bcrypt)
    target_link_libraries(txstdlib PRIVATE winhttp ws2_32 advapi32 bcrypt)
endif()
set(TX_TOOL_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/tx")
set_target_properties(txstdlib PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${TX_TOOL_OUTPUT_DIR}"
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
