#flatbuffer encoding/decoding tests
add_executable(fbs_test fbs.c)
target_link_libraries(fbs_test epx)
add_test(NAME message_flatbuffer COMMAND fbs_test)

add_executable(chan_test channel.c)
target_link_libraries(chan_test epx)
add_test(NAME channel COMMAND chan_test)
