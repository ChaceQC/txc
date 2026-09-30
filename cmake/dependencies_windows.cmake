include(FetchContent)
include(cmake/postgres.cmake)
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
FetchContent_Declare(sqlite_amalgamation
    URL https://www.sqlite.org/2026/sqlite-amalgamation-3530400.zip
    URL_HASH SHA256=1e71ddf93849c6a6ecf58b827c0692073d2dd7ee40196158068f7b29f422e87d
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
    libsodium_binary sqlite_amalgamation)
add_library(tx_sqlite_static STATIC "${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c")
target_include_directories(tx_sqlite_static PUBLIC "${sqlite_amalgamation_SOURCE_DIR}")
target_compile_definitions(tx_sqlite_static PRIVATE
    SQLITE_THREADSAFE=1 SQLITE_OMIT_LOAD_EXTENSION=1 SQLITE_ENABLE_PREUPDATE_HOOK=1)
set_source_files_properties(src/stdlib/db_connection.cpp PROPERTIES
    COMPILE_DEFINITIONS SQLITE_ENABLE_PREUPDATE_HOOK=1)
set_target_properties(tx_sqlite_static PROPERTIES C_STANDARD 11)
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
