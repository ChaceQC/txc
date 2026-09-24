cmake_minimum_required(VERSION 3.21)
project(tx_compiler LANGUAGES CXX)

add_executable(txc
    src/frontend/lexer/lexer.cpp
    src/frontend/ast/ast.cpp
    src/frontend/parser/parser.cpp
    src/frontend/parser/parser_expression.cpp
    src/frontend/resolver/module_resolver.cpp
    src/frontend/resolver/module_resolver_tree.cpp
    src/frontend/sema/sema.cpp
    src/frontend/sema/sema_expression.cpp
    src/backend/cpp/codegen.cpp
    src/backend/cpp/codegen_expression.cpp
    src/driver/module_loader.cpp
    src/driver/main.cpp
)

add_library(txstdlib STATIC
    src/stdlib/string.cpp
    src/stdlib/conversion.cpp
    src/stdlib/io.cpp
    src/stdlib/encoding.cpp
    src/stdlib/file.cpp
)

target_compile_features(txc PRIVATE cxx_std_23)
target_compile_features(txstdlib PRIVATE cxx_std_23)
target_include_directories(txc PRIVATE src)
target_include_directories(txstdlib PRIVATE src)
add_dependencies(txc txstdlib)
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
