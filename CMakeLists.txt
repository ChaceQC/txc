cmake_minimum_required(VERSION 3.21)
project(tx_compiler LANGUAGES CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
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
    src/backend/llvm/codegen_vector_statement.cpp
    src/backend/llvm/codegen_vector_declarations.cpp
    src/backend/llvm/codegen_algorithm_declarations.cpp
    src/backend/llvm/codegen_container.cpp
    src/backend/llvm/codegen_container_declarations.cpp
    src/backend/llvm/codegen_map.cpp
    src/backend/llvm/codegen_functions.cpp
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
    src/driver/main.cpp
)

add_library(txstdlib STATIC
    src/backend/cpp/runtime.cpp
    src/backend/cpp/runtime_abi.cpp
    src/backend/cpp/error_abi.cpp
    src/backend/cpp/parse_abi.cpp
    src/backend/cpp/file_result_abi.cpp
    src/backend/cpp/text_reference.cpp
    src/backend/cpp/string_concat_abi.cpp
    src/backend/cpp/dictionary_string_query.cpp
    src/backend/cpp/vector_i64_abi.cpp
    src/backend/cpp/vector_f64_abi.cpp
    src/backend/cpp/vector_bool_abi.cpp
    src/backend/cpp/vector_str_abi.cpp
    src/backend/cpp/vector_text_abi.cpp
    src/backend/cpp/algorithm_abi.cpp
    src/backend/cpp/map_i64_abi.cpp
    src/backend/cpp/map_f64_abi.cpp
    src/backend/cpp/map_bool_abi.cpp
    src/backend/cpp/map_str_abi.cpp
    src/backend/cpp/set_abi.cpp
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
    src/stdlib/io.cpp
    src/stdlib/encoding.cpp
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
)

target_compile_features(txc PRIVATE cxx_std_23)
target_compile_features(txstdlib PRIVATE cxx_std_23)
target_include_directories(txc PRIVATE src)
target_include_directories(txstdlib PRIVATE src)
add_dependencies(txc txstdlib)
if(WIN32)
    target_link_libraries(txc PRIVATE shell32)
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
