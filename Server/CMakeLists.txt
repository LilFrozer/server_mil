cmake_minimum_required(VERSION 3.14)

project(server_mil LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network REQUIRED)

add_executable(server_mil
    executor.h executor.cpp
    lib/core.h
    lib/core.cpp
    lib/libdspapi.h
    lib/libdspapi.cpp
    protocol.h
    main.cpp
    libViod/libdspapidata.h
    dspapiviod.h dspapiviod.cpp
    definesMil.h
    openapiviod.h openapiviod.cpp
    farbos/proto.h
    farbos/ts.h
    groupData.h
)

add_library(libdspapi SHARED IMPORTED)
set_target_properties(libdspapi PROPERTIES
    IMPORTED_LOCATION C:/tmp/5p27_NEW/Server/libdspapidata.so
)

add_library(zstd STATIC
    zstd/lib/common/debug.c
    zstd/lib/common/entropy_common.c
    zstd/lib/common/error_private.c
    zstd/lib/common/fse_decompress.c
    zstd/lib/common/pool.c
    zstd/lib/common/threading.c
    zstd/lib/common/xxhash.c
    zstd/lib/common/zstd_common.c

    zstd/lib/compress/fse_compress.c
    zstd/lib/compress/hist.c
    zstd/lib/compress/huf_compress.c
    zstd/lib/compress/zstd_compress.c
    zstd/lib/compress/zstd_compress_literals.c
    zstd/lib/compress/zstd_compress_sequences.c
    zstd/lib/compress/zstd_compress_superblock.c
    zstd/lib/compress/zstd_double_fast.c
    zstd/lib/compress/zstd_fast.c
    zstd/lib/compress/zstd_lazy.c
    zstd/lib/compress/zstd_ldm.c
    zstd/lib/compress/zstd_opt.c
    zstd/lib/compress/zstd_preSplit.c
    zstd/lib/compress/zstdmt_compress.c

    zstd/lib/decompress/huf_decompress.c
    zstd/lib/decompress/zstd_ddict.c
    zstd/lib/decompress/zstd_decompress.c
    zstd/lib/decompress/zstd_decompress_block.c
)
target_include_directories(zstd PUBLIC
    zstd/lib
)

target_link_libraries(server_mil Qt${QT_VERSION_MAJOR}::Core -lSocket Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(server_mil libdspapi -lzstd)

if(CMAKE_SYSTEM_NAME STREQUAL "KPDA")
    set(INSTALL_DESTDIR "/tmp")

    install(TARGETS server_mil
        RUNTIME DESTINATION "${INSTALL_DESTDIR}"
        BUNDLE DESTINATION "${INSTALL_DESTDIR}"
        LIBRARY DESTINATION "${INSTALL_DESTDIR}"
    )
endif()
