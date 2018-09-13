execute_process(COMMAND flatcc -a "${CMAKE_CURRENT_SOURCE_DIR}/epx.fbs")

add_library(fbs "${CMAKE_CURRENT_SOURCE_DIR}/epx_builder.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/epx_reader.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/epx_verifier.h")

target_include_directories(fbs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
