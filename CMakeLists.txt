cmake_minimum_required(VERSION 3.1)
project(epx VERSION 1.0
	DESCRIPTION "Egalitarian Paxos"
	LANGUAGES C)

# The version number.
set(epx_VERSION_MAJOR 1)
set(epx_VERSION_MINOR 0)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file(
	"${PROJECT_SOURCE_DIR}/src/include/epx.h.in"
	"${PROJECT_BINARY_DIR}/src/include/epx.h"
	)

# add the binary tree to the search path for include files
# so that we will find epx.h
target_include_directories(epx PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_features(epx PRIVATE c_std_11)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

add_library(imp_dill STATIC IMPORTED)
set_property(TARGET imp_dill PROPERTY
	IMPORTED_LOCATION /usr/local/lib/libdill.a)
add_library(imp_pthread STATIC IMPORTED)
set_property(TARGET imp_pthread PROPERTY
	IMPORTED_LOCATION /usr/lib/libpthread.a)
add_library(flatccrt STATIC IMPORTED)
set_property(TARGET flatccrt PROPERTY
	IMPORTED_LOCATION /usr/local/lib/libflatccrt.a)
add_subdirectory(fbs)
add_executable(epx epx.c)
target_link_libraries(epx
	PRIVATE imp_dill imp_pthread fbs flatccrt)
