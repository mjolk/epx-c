#flatbuffer encoding/decoding tests
add_executable(fbs_test fbs.c)
target_link_libraries(fbs_test epx)
add_test(NAME message_flatbuffer COMMAND fbs_test)

#channel sync test
add_executable(chan_test channel.c)
target_link_libraries(chan_test epx)
add_test(NAME channel COMMAND chan_test)

#flatbuffer socket test
add_custom_target(
    fbs_test_built
    COMMAND flatcc_d -a tepx.fbs
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/fbs"
    BYPRODUCTS tepx_reader.h tepx_builder.h tepx_verifier.h
    )

add_executable(fbs_sock_test fbs_sock.c)
add_dependencies(fbs_sock_test fbs_test_built)
target_link_libraries(fbs_sock_test epx)
target_include_directories(fbs_sock_test INTERFACE "${CMAKE_SOURCE_DIR}/test/fbs")
add_test(NAME fbs_sock COMMAND fbs_sock_test)

add_executable(instance_test instance.c)
target_link_libraries(instance_test epx)
add_test(NAME instance COMMAND instance_test)

add_executable(index_test index.c)
target_link_libraries(index_test epx)
add_test(NAME index COMMAND index_test)

add_executable(replica_test replica.c)
target_link_libraries(replica_test epx)
add_test(NAME replica COMMAND replica_test)

add_executable(epaxos_test epaxos.c)
target_link_libraries(epaxos_test epx)
add_test(NAME epaxos COMMAND epaxos_test)
