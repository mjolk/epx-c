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

add_library(imp_dill STATIC IMPORTED)
set_property(TARGET imp_dill PROPERTY
	IMPORTED_LOCATION /usr/local/lib/libdill.a)

add_library(flatccrt STATIC IMPORTED)
set_property(TARGET flatccrt PROPERTY
	IMPORTED_LOCATION /usr/local/lib/libflatccrt.a)

add_library(flatccrt_debug STATIC IMPORTED)
set_property(TARGET flatccrt_debug PROPERTY
	IMPORTED_LOCATION /usr/local/lib/libflatccrt_d.a)

add_library(epx
	instance_id.c
	command.c
	message.c
	instance.c
	span.c
	replica.c
	epaxos.c
	fbs.c
	buffer.c
	node.c
	epx.c
	)

add_dependencies(epx fbs_built)
target_include_directories(epx INTERFACE "${CMAKE_SOURCE_DIR}/src/fbs")

target_compile_features(epx PRIVATE c_std_11)
target_link_libraries(epx
	imp_dill
	Threads::Threads
	debug flatccrt_debug
	optimized flatccrt
	)

add_executable(epx_service main.c)
target_link_libraries(epx_service epx)
