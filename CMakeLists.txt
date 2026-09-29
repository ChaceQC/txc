cmake_minimum_required(VERSION 3.21)
project(tx_compiler LANGUAGES C CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)
if(WIN32)
    FetchContent_Declare(ccache_binary
        URL https://github.com/ccache/ccache/releases/download/v4.14/ccache-4.14-windows-x86_64.zip
        URL_HASH SHA256=2568347a697e103ca1b073981c704ad76fb2507d066c38dba038dd73399d968f
        SOURCE_SUBDIR _tx_no_cmake_project
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(ccache_binary)
    set(_tx_ccache_exe "${ccache_binary_SOURCE_DIR}/ccache.exe")
    if(NOT EXISTS "${_tx_ccache_exe}")
        message(FATAL_ERROR "缺少固定版本的 ccache 编译缓存工具")
    endif()
    set(CMAKE_C_COMPILER_LAUNCHER "${_tx_ccache_exe}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${_tx_ccache_exe}")
else()
    find_program(_tx_ccache_exe NAMES ccache)
    if(_tx_ccache_exe)
        set(CMAKE_C_COMPILER_LAUNCHER "${_tx_ccache_exe}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${_tx_ccache_exe}")
    endif()
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_LIB_ONLY ON CACHE BOOL "" FORCE)
set(ENABLE_SHARED_LIB OFF CACHE BOOL "" FORCE)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
if(MINGW)
    # 让 Mbed TLS 库和包含其头文件的调用方读取同一份线程配置。
    set(MBEDTLS_USER_CONFIG_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/mbedtls_user_config.h"
        CACHE FILEPATH "TX Mbed TLS user configuration" FORCE)
endif()
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
FetchContent_Declare(nghttp3
    URL https://github.com/ngtcp2/nghttp3/releases/download/v1.18.0/nghttp3-1.18.0.tar.xz
    URL_HASH SHA256=aad782c23d3f01bd4bb52c8bac7a553b631ef8115fd1612703df6183449fef19
    SOURCE_SUBDIR _tx_no_cmake_project
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(msquic_binary
    URL https://api.nuget.org/v3-flatcontainer/microsoft.native.quic.msquic.schannel/2.6.1/microsoft.native.quic.msquic.schannel.2.6.1.nupkg
    URL_HASH SHA256=cf09771561c16dc823454c212fbf3bc1307c0a77b9d98326f1faa256685e2a3f
    SOURCE_SUBDIR _tx_no_cmake_project
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(argon2_reference
    GIT_REPOSITORY https://github.com/P-H-C/phc-winner-argon2.git
    GIT_TAG 62358ba2123abd17fccf2a108a301d4b52c01a7c)
FetchContent_Declare(icu_binary
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-icu-78.3-4-any.pkg.tar.zst
    URL_HASH SHA256=8486a018aca2e56d1fcd2eed2069011f2e1da02e03af72b38916ee06cbe7203f
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(pcre2_binary
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-pcre2-10.48-3-any.pkg.tar.zst
    URL_HASH SHA256=c67c23f448693e8c9cc3973872372dabec2abdf6afacb563096f7eb0e6837f57
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(libxml2
    URL https://download.gnome.org/sources/libxml2/2.13/libxml2-2.13.8.tar.xz
    URL_HASH SHA256=277294cb33119ab71b2bc81f2f445e9bc9435b893ad15bb2cd2b0e859a0ee84a
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(libsodium_binary
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libsodium-1.0.22-3-any.pkg.tar.zst
    URL_HASH SHA256=e8d8bc169fa122eccfc3e4252615937a62fa0bd6ca21ed4912bac48d6ed2f870
    SOURCE_SUBDIR _tx_no_cmake_project
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(LIBXML2_WITH_C14N OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_CATALOG OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_DEBUG OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_FTP OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_HTTP OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_ICONV OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_ICU OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_LZMA OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_MODULES OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_PYTHON OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_PROGRAMS OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_XINCLUDE OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(stdcpp_runtime
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libstdc%2B%2B-16.2.0-4-any.pkg.tar.zst
    URL_HASH SHA256=3d4c3faf4c2c5c7a851ff12214ddcbf8c0d6df0964fc1d3ebd8c38f227183034
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(gcc_runtime
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libgcc-16.2.0-4-any.pkg.tar.zst
    URL_HASH SHA256=d615f6a8536ca16b1f049daea1fa440a3b7a405449ec0e93ad6106db43682d54
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(winpthread_runtime
    URL https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libwinpthread-14.0.0.r426.g4564ee4b5-1-any.pkg.tar.zst
    URL_HASH SHA256=543017ce2731292b215bf1d36fd70a86d8a8d5ed0afba9d3db9fff89804cda71
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(_tx_warn_deprecated ${CMAKE_WARN_DEPRECATED})
set(CMAKE_WARN_DEPRECATED OFF)
FetchContent_MakeAvailable(nghttp2 mbedtls nghttp3 msquic_binary
    icu_binary pcre2_binary libxml2
    stdcpp_runtime gcc_runtime winpthread_runtime argon2_reference
    libsodium_binary)
if(MINGW)
    find_package(Threads REQUIRED)
    # mbedtls / mbedx509 通过 mbedcrypto 继承此链接依赖。
    target_link_libraries(mbedcrypto PUBLIC Threads::Threads)
endif()
include(ExternalProject)
set(_tx_nghttp3_archive
    "${nghttp3_BINARY_DIR}/external/lib/libnghttp3.a")
ExternalProject_Add(tx_nghttp3_external
    SOURCE_DIR "${nghttp3_SOURCE_DIR}"
    BINARY_DIR "${nghttp3_BINARY_DIR}/external"
    CMAKE_GENERATOR Ninja
    CMAKE_ARGS
        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
        "-DCMAKE_BUILD_TYPE=Release"
        "-DENABLE_LIB_ONLY=ON"
        "-DENABLE_SHARED_LIB=OFF"
        "-DENABLE_STATIC_LIB=ON"
        "-DBUILD_TESTING=OFF"
    BUILD_BYPRODUCTS "${_tx_nghttp3_archive}"
    INSTALL_COMMAND "")
add_library(nghttp3_static STATIC IMPORTED GLOBAL)
set_target_properties(nghttp3_static PROPERTIES
    IMPORTED_LOCATION "${_tx_nghttp3_archive}"
    INTERFACE_INCLUDE_DIRECTORIES "${nghttp3_SOURCE_DIR}/lib/includes"
    INTERFACE_COMPILE_DEFINITIONS NGHTTP3_STATICLIB)
add_dependencies(nghttp3_static tx_nghttp3_external)
add_library(libsodium_static STATIC IMPORTED GLOBAL)
set_target_properties(libsodium_static PROPERTIES IMPORTED_LOCATION
    "${libsodium_binary_SOURCE_DIR}/mingw64/lib/libsodium.a")
add_library(argon2_reference_static STATIC
    "${argon2_reference_SOURCE_DIR}/src/argon2.c"
    "${argon2_reference_SOURCE_DIR}/src/core.c"
    "${argon2_reference_SOURCE_DIR}/src/blake2/blake2b.c"
    "${argon2_reference_SOURCE_DIR}/src/thread.c"
    "${argon2_reference_SOURCE_DIR}/src/encoding.c"
    "${argon2_reference_SOURCE_DIR}/src/ref.c")
target_include_directories(argon2_reference_static PUBLIC
    "${argon2_reference_SOURCE_DIR}/include")
target_compile_definitions(argon2_reference_static PRIVATE
    blake2b=tx_argon2_blake2b
    blake2b_init=tx_argon2_blake2b_init
    blake2b_init_key=tx_argon2_blake2b_init_key
    blake2b_init_param=tx_argon2_blake2b_init_param
    blake2b_update=tx_argon2_blake2b_update
    blake2b_final=tx_argon2_blake2b_final
    blake2b_long=tx_argon2_blake2b_long)
target_compile_definitions(argon2_reference_static PRIVATE
    argon2_ctx=tx_argon2_ctx
    argon2_hash=tx_argon2_hash
    argon2_verify=tx_argon2_verify
    argon2i_hash_encoded=tx_argon2i_hash_encoded
    argon2i_hash_raw=tx_argon2i_hash_raw
    argon2i_verify=tx_argon2i_verify
    argon2id_hash_encoded=tx_argon2id_hash_encoded
    argon2id_hash_raw=tx_argon2id_hash_raw
    argon2id_verify=tx_argon2id_verify)
if(WIN32)
    # 上游 RC 资源在带空格的 MinGW 安装路径下无法完成预处理；静态库不需要版本资源。
    get_target_property(_tx_libxml2_sources LibXml2 SOURCES)
    list(FILTER _tx_libxml2_sources EXCLUDE REGEX "libxml2[.]rc$")
    set_property(TARGET LibXml2 PROPERTY SOURCES "${_tx_libxml2_sources}")
endif()
set(CMAKE_WARN_DEPRECATED ${_tx_warn_deprecated})

# 编译器内嵌公开接口和 ABI 的内容摘要；增删或修改文件都会触发重新配置。
file(GLOB _tx_interface_files CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/tx/stdlib/*.txh")
file(GLOB _tx_abi_files CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src/backend/cpp/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/backend/llvm/*.cpp")
list(APPEND _tx_abi_files "src/common/error_kind.hpp" "src/common/format_spec.hpp"
    "src/common/text_encoding.hpp")
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
    src/backend/llvm/codegen_iterator.cpp
    src/backend/llvm/codegen_closure.cpp
    src/backend/llvm/codegen_vector_statement.cpp
    src/backend/llvm/codegen_vector_declarations.cpp
    src/backend/llvm/codegen_algorithm_declarations.cpp
    src/backend/llvm/codegen_algorithm.cpp
    src/backend/llvm/codegen_thread.cpp
    src/backend/llvm/codegen_task.cpp
    src/backend/llvm/codegen_sync.cpp
    src/backend/llvm/codegen_channel.cpp
    src/backend/llvm/codegen_serde.cpp
    src/backend/llvm/codegen_format.cpp
    src/backend/llvm/codegen_encoding.cpp
    src/backend/llvm/codegen_format_plan.cpp
    src/backend/llvm/codegen_container.cpp
    src/backend/llvm/codegen_key.cpp
    src/backend/llvm/codegen_container_declarations.cpp
    src/backend/llvm/codegen_ordered_declarations.cpp
    src/backend/llvm/codegen_sequence_declarations.cpp
    src/backend/llvm/codegen_map.cpp
    src/backend/llvm/codegen_functions.cpp
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
    src/backend/llvm/codegen_call_borrow.cpp
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
    src/backend/cpp/runtime_context.cpp
    src/backend/cpp/test_abi.cpp
    src/backend/cpp/log_abi.cpp
    src/backend/cpp/debug_abi.cpp
    src/backend/cpp/parse_abi.cpp
    src/backend/cpp/json_abi.cpp
    src/backend/cpp/json_stream_abi.cpp
    src/backend/cpp/cbor_abi.cpp
    src/backend/cpp/serde_abi.cpp
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
    src/backend/cpp/cancel_abi.cpp
    src/backend/cpp/thread_abi.cpp
    src/backend/cpp/task_runtime.cpp
    src/backend/cpp/task_iocp.cpp
    src/backend/cpp/task_file_iocp.cpp
    src/backend/cpp/task_executor.cpp
    src/backend/cpp/task_abi.cpp
    src/backend/cpp/sync_lock_abi.cpp
    src/backend/cpp/sync_lock_generic_abi.cpp
    src/backend/cpp/sync_primitive_abi.cpp
    src/backend/cpp/sync_atomic_abi.cpp
    src/backend/cpp/channel_abi.cpp
    src/backend/cpp/concurrency_value.cpp
    src/backend/cpp/concurrency_result.cpp
    src/backend/cpp/process_abi.cpp
    src/backend/cpp/process_io_abi.cpp
    src/backend/cpp/ipc_abi.cpp
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
    src/backend/cpp/format_static_abi.cpp
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
    src/common/format_spec.cpp
    src/common/text_encoding.cpp
    src/stdlib/dictionary.cpp
    src/stdlib/conversion.cpp
    src/stdlib/parse.cpp
    src/stdlib/json_parser.cpp
    src/stdlib/json_parser_string.cpp
    src/stdlib/json_stream.cpp
    src/stdlib/json_schema.cpp
    src/stdlib/cbor_common.cpp
    src/stdlib/cbor_reader.cpp
    src/stdlib/cbor_writer.cpp
    src/stdlib/serde_schema.cpp
    src/stdlib/serde_value.cpp
    src/stdlib/serde.cpp
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
    src/stdlib/json_fields.cpp
    src/stdlib/log.cpp
    src/stdlib/test.cpp
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
    src/stdlib/tls_identity.cpp
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
target_compile_features(txstdlib PRIVATE cxx_std_23)
target_include_directories(txc PRIVATE src "${CMAKE_BINARY_DIR}/generated")
target_include_directories(txstdlib PRIVATE src)
target_include_directories(txstdlib PRIVATE
    "${icu_binary_SOURCE_DIR}/mingw64/include"
    "${pcre2_binary_SOURCE_DIR}/mingw64/include"
    "${libsodium_binary_SOURCE_DIR}/mingw64/include")
target_compile_definitions(txstdlib PRIVATE PCRE2_STATIC)
target_include_directories(txstdlib PRIVATE
    "${msquic_binary_SOURCE_DIR}/build/native/include")
target_link_libraries(txstdlib PRIVATE nghttp2_static nghttp3_static
    mbedtls mbedx509 mbedcrypto
    argon2_reference_static LibXml2::LibXml2 libsodium_static)
add_dependencies(txc txstdlib)
if(WIN32)
    target_link_libraries(txc PRIVATE shell32 bcrypt)
    target_link_libraries(txstdlib PRIVATE winhttp ws2_32 dnsapi advapi32 bcrypt crypt32 ncrypt)
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
