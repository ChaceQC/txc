# EDB 官方 Windows x64 二进制，只交付 libpq 的 C ABI 和它的运行时依赖。
FetchContent_Declare(postgres_binary
    URL https://get.enterprisedb.com/postgresql/postgresql-18.4-1-windows-x64-binaries.zip
    URL_HASH SHA256=7effe34c0bf89027b3f171447d351cbc460f4566c8d0f643daec67f140787858
    SOURCE_SUBDIR _tx_no_cmake_project
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(postgres_binary)
set(TX_POSTGRES_ROOT "${postgres_binary_SOURCE_DIR}/pgsql")
if(NOT EXISTS "${TX_POSTGRES_ROOT}/include/libpq-fe.h")
    # FetchContent 会去掉只有一个顶层目录的归档外壳。
    set(TX_POSTGRES_ROOT "${postgres_binary_SOURCE_DIR}")
endif()
if(NOT WIN32)
    message(FATAL_ERROR "当前固定 libpq 发行包仅支持 Windows x64")
endif()
find_program(TX_DLLTOOL NAMES dlltool REQUIRED)
# EDB 的 MSVC 短导入对象不能直接混入 GNU 静态归档，生成等价 GNU 导入库。
set(_tx_pq_def "${CMAKE_SOURCE_DIR}/third_party/postgresql/libpq.def")
add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/libtx_libpq_import.a"
    COMMAND "${TX_DLLTOOL}" -m i386:x86-64 -d "${_tx_pq_def}"
        -l "${CMAKE_BINARY_DIR}/libtx_libpq_import.a"
    DEPENDS "${_tx_pq_def}" VERBATIM)
add_custom_target(tx_libpq_import DEPENDS "${CMAKE_BINARY_DIR}/libtx_libpq_import.a")
add_library(tx_libpq STATIC IMPORTED)
set_target_properties(tx_libpq PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/libtx_libpq_import.a"
    INTERFACE_INCLUDE_DIRECTORIES "${TX_POSTGRES_ROOT}/include")
add_dependencies(tx_libpq tx_libpq_import)
