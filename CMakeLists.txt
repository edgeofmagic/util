cmake_minimum_required(VERSION 3.0.0)
project(logicmill VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)


	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
#	else()
#		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3")
	endif()

enable_testing()

add_subdirectory( ext/libuv )


# INCLUDE_DIRECTORIES(/Users/dcurtis/botan-install/include/botan-2)
LINK_DIRECTORIES(/Users/dcurtis/botan-install/lib)

include_directories( include ext/boost ext/libuv/include ext/doctest/include /Users/dcurtis/botan-install/include/botan-2 )

set(LOGICMILL_SRCS
	include/logicmill/bstream.h
	include/logicmill/bstream/base_classes.h
	include/logicmill/bstream/context.h
	include/logicmill/bstream/error.h
	include/logicmill/bstream/error_category_context.h
	include/logicmill/bstream/fwd_decls.h
	include/logicmill/bstream/ibstream.h
	include/logicmill/bstream/ibstream_traits.h
	include/logicmill/bstream/ifbstream.h
	include/logicmill/bstream/imbstream.h
	include/logicmill/bstream/macros.h
	include/logicmill/bstream/msgpack.h
	include/logicmill/bstream/obstream.h
	include/logicmill/bstream/obstream_traits.h
	include/logicmill/bstream/ofbstream.h
	include/logicmill/bstream/ombstream.h
	include/logicmill/bstream/stdlib.h
	include/logicmill/bstream/typecode.h
	include/logicmill/bstream/types.h
	include/logicmill/bstream/source.h
	include/logicmill/bstream/sink.h
	include/logicmill/bstream/file/source.h
	include/logicmill/bstream/file/sink.h
	include/logicmill/bstream/buffer/source.h
	include/logicmill/bstream/buffer/sink.h
	include/logicmill/bstream/bufseq/sink.h
	include/logicmill/bstream/stdlib/chrono.h
	include/logicmill/bstream/stdlib/deque.h
	include/logicmill/bstream/stdlib/forward_list.h
	include/logicmill/bstream/stdlib/list.h
	include/logicmill/bstream/stdlib/map.h
	include/logicmill/bstream/stdlib/pair.h
	include/logicmill/bstream/stdlib/set.h
	include/logicmill/bstream/stdlib/tuple.h
	include/logicmill/bstream/stdlib/unordered_map.h
	include/logicmill/bstream/stdlib/unordered_set.h
	include/logicmill/bstream/stdlib/vector.h
	include/logicmill/bstream/memory.h
	include/logicmill/async/loop.h
	include/logicmill/async/timer.h
	include/logicmill/async/address.h
	include/logicmill/async/error.h
	include/logicmill/async/event_flow.h
	include/logicmill/async/channel.h

	include/logicmill/armi/client_context.h
	include/logicmill/armi/client_context_base.h
	include/logicmill/armi/error.h
	include/logicmill/armi/interface_stub.h
	include/logicmill/armi/interface_stub_base.h
	include/logicmill/armi/interface_stub_builder.h
	include/logicmill/armi/macros.h
	include/logicmill/armi/member_func_proxy.h
	include/logicmill/armi/member_func_stub.h
	include/logicmill/armi/member_func_stub_base.h
	include/logicmill/armi/armi.h
	include/logicmill/armi/reply_handler.h
	include/logicmill/armi/reply_handler_base.h
	include/logicmill/armi/server_context.h
	include/logicmill/armi/server_context_base.h
	include/logicmill/armi/types.h
	include/logicmill/armi/adapters/async/client_adapter.h
	include/logicmill/armi/adapters/async/server_adapter.h
	include/logicmill/armi/adapters/async/channel_manager.h

	include/logicmill/laps/channel_anchor.h
	include/logicmill/laps/driver.h
	include/logicmill/laps/types.h
	include/logicmill/laps/framer.h
	include/logicmill/laps/tls.h

	include/logicmill/util/allocator.h
	include/logicmill/util/dumpster.h
	include/logicmill/util/macros.h
	include/logicmill/util/preprocessor.h
	include/logicmill/util/shareable.h
	include/logicmill/util/shared_ptr.h
	include/logicmill/util/buffer.h
	include/logicmill/util/promise.h

	src/bstream/error.cpp
	src/bstream/error_category_context.cpp
	src/bstream/ibstream.cpp
	src/bstream/obstream.cpp
	src/bstream/source.cpp
	src/bstream/sink.cpp
	src/bstream/file_source.cpp
	src/bstream/file_sink.cpp
	src/bstream/buffer_sink.cpp
	src/bstream/bufseq_sink.cpp
	src/async/loop_uv.h
	src/async/loop_uv.cpp
	src/async/timer_uv.h
	src/async/timer_uv.cpp
	src/async/tcp_uv.h
	src/async/tcp_uv.cpp
	src/async/udp_uv.h
	src/async/udp_uv.cpp
	src/async/address.cpp
	src/async/error.cpp

	src/armi/client_context_base.cpp
	src/armi/error.cpp
	src/armi/adapters/async/channel_manager.cpp
	src/armi/adapters/async/client_adapter.cpp
	src/armi/adapters/async/server_adapter.cpp

	src/laps/error.cpp
	src/laps/tls.cpp

	src/util/buffer.cpp
	
	src/fs_impl.cpp)

set(LOGICMILL_TEST_SRCS
	test/bstream/abstract_source.cpp
	test/bstream/file.cpp
	test/bstream/memory.cpp
	test/bstream/bufseq.cpp
	test/bstream/fbstream.cpp
	test/bstream/test0.cpp
	test/bstream/test1.cpp
#	test/bstream/test2.cpp
	test/bstream/test3.cpp
	test/bstream/test4.cpp
	test/buffer.cpp
	test/async/loop.cpp
	test/async/address.cpp
	test/async/endpoint.cpp
	test/async/tcp.cpp
	test/async/udp.cpp
 	test/async/event_flow.cpp
	test/armi/basic.cpp
	test/armi/example.cpp
	test/laps/layers.cpp
	test/laps/tls_layer.cpp
	test/util/shareable.cpp
	test/util/allocators.cpp
	test/util/shared_ptr.cpp
	test/util/promise.cpp
	test/test_main.cpp)

add_library(logicmill ${LOGICMILL_SRCS})

add_executable(logicmill_test ${LOGICMILL_TEST_SRCS})
target_link_libraries(logicmill_test logicmill uv_a botan-2)

#add_test(NAME test_bstream COMMAND logicmill_test --test-case=*bstream*)
#add_test(NAME test_async COMMAND logicmill_test --test-case=*async*)

add_test(NAME logicmill_test COMMAND logicmill_test )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)
