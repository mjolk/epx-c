execute_process(COMMAND flatcc -a "${CMAKE_CURRENT_SOURCE_DIR}/epx.fbs")

add_library(flatbuf INTERFACE)
target_include_directories(flatbuf INTERFACE ./)