# Linux 使用原生 ELF 依赖；Mbed TLS 需 3.6 API，不能使用 Ubuntu 的 2.x 开发包。
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc i18n data)
find_package(LibXml2 REQUIRED)
find_package(PostgreSQL 17 REQUIRED)
pkg_check_modules(TX_PCRE2 REQUIRED IMPORTED_TARGET libpcre2-8)
pkg_check_modules(TX_SODIUM REQUIRED IMPORTED_TARGET libsodium)
pkg_check_modules(TX_ARGON2 REQUIRED IMPORTED_TARGET libargon2)
pkg_check_modules(TX_CURL REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(TX_CARES REQUIRED IMPORTED_TARGET libcares)
find_program(_tx_ccache_exe NAMES ccache)
if(_tx_ccache_exe)
    set(CMAKE_C_COMPILER_LAUNCHER "${_tx_ccache_exe}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${_tx_ccache_exe}")
endif()
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_LIB_ONLY ON CACHE BOOL "" FORCE)
set(ENABLE_SHARED_LIB OFF CACHE BOOL "" FORCE)
set(ENABLE_STATIC_LIB ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "" FORCE)
set(MBEDTLS_USER_CONFIG_FILE "${CMAKE_SOURCE_DIR}/cmake/mbedtls_linux_config.h"
    CACHE FILEPATH "TX Mbed TLS user configuration" FORCE)
FetchContent_Declare(mbedtls
    URL https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.5/mbedtls-3.6.5.tar.bz2
    URL_HASH SHA256=4a11f1777bb95bf4ad96721cac945a26e04bf19f57d905f241fe77ebeddf46d8
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(sqlite_amalgamation
    URL https://www.sqlite.org/2026/sqlite-amalgamation-3530400.zip
    URL_HASH SHA256=1e71ddf93849c6a6ecf58b827c0692073d2dd7ee40196158068f7b29f422e87d
    SOURCE_SUBDIR _tx_no_cmake_project
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(msquic_headers
    URL https://github.com/microsoft/msquic/archive/refs/tags/v2.6.1.tar.gz
    URL_HASH SHA256=f1ec3cb72955d1d5e06d8d1bf184fdc9778e6baa98410ca480ca5d94fbc414de
    SOURCE_SUBDIR _tx_no_cmake_project
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(nghttp3
    URL https://github.com/ngtcp2/nghttp3/releases/download/v1.18.0/nghttp3-1.18.0.tar.xz
    URL_HASH SHA256=aad782c23d3f01bd4bb52c8bac7a553b631ef8115fd1612703df6183449fef19
    SOURCE_SUBDIR _tx_no_cmake_project
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(nghttp2
    URL https://github.com/nghttp2/nghttp2/releases/download/v1.68.0/nghttp2-1.68.0.tar.xz
    URL_HASH SHA256=5511d3128850e01b5b26ec92bf39df15381c767a63441438b25ad6235def902c
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(mbedtls sqlite_amalgamation msquic_headers nghttp3 nghttp2)
include(ExternalProject)
set(_tx_nghttp3_archive "${nghttp3_BINARY_DIR}/external/lib/libnghttp3.a")
ExternalProject_Add(tx_nghttp3_external
    SOURCE_DIR "${nghttp3_SOURCE_DIR}"
    BINARY_DIR "${nghttp3_BINARY_DIR}/external"
    CMAKE_GENERATOR Ninja
    CMAKE_ARGS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
        "-DCMAKE_BUILD_TYPE=Release" "-DENABLE_LIB_ONLY=ON"
        "-DENABLE_SHARED_LIB=OFF" "-DENABLE_STATIC_LIB=ON" "-DBUILD_TESTING=OFF"
    BUILD_BYPRODUCTS "${_tx_nghttp3_archive}" INSTALL_COMMAND "")
add_library(nghttp3_static STATIC IMPORTED GLOBAL)
set_target_properties(nghttp3_static PROPERTIES IMPORTED_LOCATION "${_tx_nghttp3_archive}"
    INTERFACE_INCLUDE_DIRECTORIES "${nghttp3_SOURCE_DIR}/lib/includes"
    INTERFACE_COMPILE_DEFINITIONS NGHTTP3_STATICLIB)
add_dependencies(nghttp3_static tx_nghttp3_external)
add_library(tx_sqlite_static STATIC "${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c")
target_include_directories(tx_sqlite_static PUBLIC "${sqlite_amalgamation_SOURCE_DIR}")
target_compile_definitions(tx_sqlite_static PRIVATE
    SQLITE_THREADSAFE=1 SQLITE_OMIT_LOAD_EXTENSION=1 SQLITE_ENABLE_PREUPDATE_HOOK=1)
set_source_files_properties(src/stdlib/db_connection.cpp PROPERTIES
    COMPILE_DEFINITIONS SQLITE_ENABLE_PREUPDATE_HOOK=1)
add_library(libsodium_static ALIAS PkgConfig::TX_SODIUM)
add_library(argon2_reference_static ALIAS PkgConfig::TX_ARGON2)
add_library(tx_libpq ALIAS PostgreSQL::PostgreSQL)
