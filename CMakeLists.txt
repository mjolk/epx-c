cmake_minimum_required(VERSION 3.1)
project(epx VERSION 1.0
    DESCRIPTION "Egalitarian Paxos")

# The version number.
set(epx_VERSION_MAJOR 1)
set(epx_VERSION_MINOR 0)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file(
    "${PROJECT_SOURCE_DIR}/include/epx.h.in"
    "${PROJECT_BINARY_DIR}/include/epx.h"
    )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
add_subdirectory(src)

enable_testing()
add_subdirectory(test)
