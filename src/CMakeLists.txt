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

# TODO this is a debug version, add optimized version
add_library(imp_dill STATIC IMPORTED)
set_property(TARGET imp_dill PROPERTY
    IMPORTED_LOCATION /usr/local/lib/libdill.a)

add_library(flatccrt STATIC IMPORTED)
set_property(TARGET flatccrt PROPERTY
    IMPORTED_LOCATION /usr/local/lib/libflatccrt.a)

add_library(flatccrt_debug STATIC IMPORTED)
set_property(TARGET flatccrt_debug PROPERTY
    IMPORTED_LOCATION /usr/local/lib/libflatccrt_d.a)

add_library(rocksdb STATIC IMPORTED)
set_property(TARGET rocksdb PROPERTY
    IMPORTED_LOCATION /usr/local/lib/librocksdb.a)

add_library(zlib STATIC IMPORTED)
set_property(TARGET zlib PROPERTY
    IMPORTED_LOCATION /usr/local/lib/libz.a)

add_library(bzip2 STATIC IMPORTED)
set_property(TARGET bzip2 PROPERTY
    IMPORTED_LOCATION /usr/local/lib/libbz2.a)

add_library(snappy STATIC IMPORTED)
set_property(TARGET snappy PROPERTY
    IMPORTED_LOCATION /usr/local/lib/libsnappy.a)

add_library(lz4 STATIC IMPORTED)
set_property(TARGET lz4 PROPERTY
    IMPORTED_LOCATION /usr/local/lib/liblz4.a)

add_library(zstandard STATIC IMPORTED)
set_property(TARGET zstandard PROPERTY
    IMPORTED_LOCATION /usr/local/lib/libzstd.a)

add_library(jemalloc STATIC IMPORTED)
set_property(TARGET jemalloc PROPERTY
    IMPORTED_LOCATION /usr/local/lib/libjemalloc.a)

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