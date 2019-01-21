#flatbuffer encoding/decoding tests
add_executable(fbs_test fbs.c)
target_link_libraries(fbs_test epx)
add_test(NAME message_flatbuffer COMMAND fbs_test)
