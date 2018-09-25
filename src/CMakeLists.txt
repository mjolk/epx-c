# The version number.
set(epx_VERSION_MAJOR 1)
set(epx_VERSION_MINOR 0)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file(
    "${PROJECT_SOURCE_DIR}/include/epx.h.in"
    "${PROJECT_BINARY_DIR}/include/epx.h"
    )

add_custom_target(
    fbs_built
    COMMAND flatcc -a epx.fbs
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src/fbs"
    BYPRODUCTS epx_reader.h epx_builder.h epx_verifier.h
    )
add_library(imp_dill STATIC IMPORTED)
set_property(TARGET imp_dill PROPERTY
    IMPORTED_LOCATION /usr/local/lib/libdill.a)
add_library(imp_pthread STATIC IMPORTED)
set_property(TARGET imp_pthread PROPERTY
    IMPORTED_LOCATION /usr/lib/libpthread.a)
add_library(flatccrt STATIC IMPORTED)
set_property(TARGET flatccrt PROPERTY
    IMPORTED_LOCATION /usr/local/lib/libflatccrt.a)
add_executable(epx epx.c)
add_dependencies(epx fbs_built)
target_include_directories(epx INTERFACE "${CMAKE_SOURCE_DIR}/src/fbs")
target_compile_features(epx PRIVATE c_std_11)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Werror -Wno-long-long -Wno-variadic-macros -fexceptions -DUSE_CLANG_COMPLETER")
    #target_compile_options(epx PRIVATE )
target_link_libraries(epx
    PUBLIC imp_dill imp_pthread flatccrt)
