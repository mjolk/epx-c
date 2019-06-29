#debug flags
set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Werror -Wno-long-long -Wno-variadic-macros -fexceptions -DUSE_CLANG_COMPLETER")

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    message("debug mode")
    option(FLATCC_DEBUG_VERIFY ON)
    option(FLATCC_TRACE_VERIFY ON)
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

string(
    APPEND _cmd
    "$<IF:$<CONFIG:Debug>,"
    "flatcc_d;-a;epx.fbs,"
    "flatcc;-a;epx.fbs"
    ">"
    )

add_custom_target(
    fbs_built
    COMMAND "${_cmd}"
    COMMAND_EXPAND_LISTS
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src/fbs"
    BYPRODUCTS epx_reader.h epx_builder.h epx_verifier.h
    )

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

#TODO add optimized build of libdill
find_library(
    dill
    NAMES libdill.a
)
add_library(imp_dill STATIC IMPORTED)
set_target_properties(imp_dill PROPERTIES
    IMPORTED_LOCATION ${dill})

find_library(
    flatcc
    NAMES libflatccrt.a
)
add_library(flatccrt STATIC IMPORTED)
set_target_properties(flatccrt PROPERTIES
    IMPORTED_LOCATION ${flatcc})

find_library(
    flatcc_debug
    NAMES libflatccrt_d.a
)
add_library(flatccrt_debug STATIC IMPORTED)
set_target_properties(flatccrt_debug PROPERTIES
    IMPORTED_LOCATION ${flatcc_debug})

find_library(
    rocks
    NAMES librocksdb.a
)
add_library(rocksdb STATIC IMPORTED)
set_target_properties(rocksdb PROPERTIES
    IMPORTED_LOCATION ${rocks})

find_library(
    libz
    NAMES libz.a
)
add_library(zlib STATIC IMPORTED)
set_target_properties(zlib PROPERTIES
    IMPORTED_LOCATION ${libz})

find_library(
    bz2
    NAMES libbz2.a
)
add_library(bzip2 STATIC IMPORTED)
set_target_properties(bzip2 PROPERTIES
    IMPORTED_LOCATION ${bz2})

find_library(
    snap
    NAMES libsnappy.a
)
add_library(snappy STATIC IMPORTED)
set_target_properties(snappy PROPERTIES
    IMPORTED_LOCATION ${snap})

find_library(
    lz4p
    NAMES liblz4.a
)
add_library(lz4 STATIC IMPORTED)
set_target_properties(lz4 PROPERTIES
    IMPORTED_LOCATION ${lz4p})

find_library(
    zstd
    NAMES libzstd.a
)
add_library(zstandard STATIC IMPORTED)
set_target_properties(zstandard PROPERTIES
    IMPORTED_LOCATION ${zstd})

find_library(
    jem
    NAMES libjemalloc.a
)
add_library(jemalloc STATIC IMPORTED)
set_target_properties(jemalloc PROPERTIES
    IMPORTED_LOCATION ${jem})

add_library(epx
    instance_id.c
    command.c
    message.c
    instance.c
    index.c
    replica.c
    epaxos.c
    fbs_sock.c
    channel.c
    storage.cpp
    execute.c
    io.c
    node.c
    main.c
    )
add_dependencies(epx fbs_built)

target_link_libraries(epx
    imp_dill
    Threads::Threads
    debug flatccrt_debug
    optimized flatccrt
    zlib
    bzip2
    snappy
    lz4
    zstandard
    jemalloc
    rocksdb
    )

add_executable(epx_service main.c)
target_link_libraries(epx_service epx)