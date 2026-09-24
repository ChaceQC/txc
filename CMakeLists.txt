cmake_minimum_required(VERSION 3.21)
project(tx_compiler LANGUAGES CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(txc
    src/frontend/lexer/lexer.cpp
    src/frontend/ast/ast.cpp
    src/frontend/parser/parser.cpp
    src/frontend/parser/parser_expression.cpp
    src/frontend/resolver/module_resolver.cpp
    src/frontend/resolver/module_resolver_tree.cpp
    src/frontend/sema/sema.cpp
    src/frontend/sema/sema_expression.cpp
    src/frontend/sema/sema_call.cpp
    src/backend/llvm/codegen.cpp
    src/backend/llvm/codegen_declarations.cpp
    src/backend/llvm/codegen_control.cpp
    src/backend/llvm/codegen_statement.cpp
    src/backend/llvm/codegen_expression.cpp
    src/backend/llvm/codegen_expression_ops.cpp
    src/backend/llvm/codegen_expression_values.cpp
    src/backend/llvm/codegen_expression_calls.cpp
    src/backend/llvm/codegen_print.cpp
    src/backend/llvm/codegen_external_direct.cpp
    src/backend/llvm/codegen_call_binding.cpp
    src/driver/module_loader.cpp
    src/driver/main.cpp
)

add_library(txstdlib STATIC
    src/backend/cpp/runtime.cpp
    src/backend/cpp/runtime_abi.cpp
    src/backend/cpp/text_direct_abi.cpp
    src/backend/cpp/array_direct_abi.cpp
    src/backend/cpp/filesystem_direct_abi.cpp
    src/backend/cpp/file_direct_abi.cpp
    src/backend/cpp/io_time_direct_abi.cpp
    src/backend/cpp/value_abi.cpp
    src/backend/cpp/value_format.cpp
    src/backend/cpp/dictionary_abi.cpp
    src/backend/cpp/call_abi.cpp
    src/backend/cpp/external_abi.cpp
    src/backend/cpp/external_abi_time_random.cpp
    src/stdlib/string.cpp
    src/stdlib/dictionary.cpp
    src/stdlib/conversion.cpp
    src/stdlib/io.cpp
    src/stdlib/encoding.cpp
    src/stdlib/file.cpp
    src/stdlib/math.cpp
    src/stdlib/array.cpp
    src/stdlib/filesystem.cpp
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
