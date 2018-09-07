cmake_minimum_required(VERSION 3.0.0)
project(logicmill VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory( ext/libuv )

include_directories( include ext/boost ext/libuv/include ext/doctest/include )

set(LOGICMILL_SRCS
	include/logicmill/buffer.h
	include/logicmill/bstream.h
	include/logicmill/bstream/base_classes.h
	include/logicmill/bstream/context.h
	include/logicmill/bstream/error.h
	include/logicmill/bstream/error_category_context.h
	include/logicmill/bstream/fwd_decls.h
	include/logicmill/bstream/ibfilebuf.h
	include/logicmill/bstream/ibmembuf.h
	include/logicmill/bstream/ibstream.h
	include/logicmill/bstream/ibstream_traits.h
	include/logicmill/bstream/ibstreambuf.h
	include/logicmill/bstream/ifbstream.h
	include/logicmill/bstream/imbstream.h
	include/logicmill/bstream/macros.h
	include/logicmill/bstream/msgpack.h
	include/logicmill/bstream/numstream.h
	include/logicmill/bstream/obfilebuf.h
	include/logicmill/bstream/obmembuf.h
	include/logicmill/bstream/obstream.h
	include/logicmill/bstream/obstream_traits.h
	include/logicmill/bstream/obstreambuf.h
	include/logicmill/bstream/ofbstream.h
	include/logicmill/bstream/ombstream.h
	include/logicmill/bstream/stdlib.h
	include/logicmill/bstream/typecode.h
	include/logicmill/bstream/types.h
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
	include/logicmill/bstream/utils/memory.h
	include/logicmill/bstream/utils/preprocessor.h
	include/logicmill/bstream/utils/traits.h
	include/logicmill/async/loop.h
	include/logicmill/async/timer.h
	include/logicmill/async/address.h
	include/logicmill/async/error.h
	src/bstream/error.cpp
	src/bstream/error_category_context.cpp
	src/bstream/ibfilebuf.cpp
	src/bstream/ibmembuf.cpp
	src/bstream/ibstream.cpp
	src/bstream/ibstreambuf.cpp
	src/bstream/numstream.cpp
	src/bstream/obfilebuf.cpp
	src/bstream/obmembuf.cpp
	src/bstream/obstream.cpp
	src/bstream/obstreambuf.cpp
	src/async/uv_async.cpp
	src/async/address.cpp
	src/async/error.cpp
	src/buffer.cpp)

set(LOGICMILL_TEST_SRCS
	test/bstream/bstreambuf.cpp
	test/bstream/fbstream.cpp
	test/bstream/test0.cpp
	test/bstream/test1.cpp
#	test/bstream/test2.cpp
	test/bstream/test3.cpp
	test/bstream/test4.cpp
	test/buffer.cpp
	test/async/loop.cpp
	test/async/address.cpp
	test/test_main.cpp)

add_library(logicmill ${LOGICMILL_SRCS})

add_executable(logicmill_test ${LOGICMILL_TEST_SRCS})
target_link_libraries(logicmill_test logicmill uv_a)

add_test(NAME test_bstream COMMAND logicmill_test --test-case=*bstream*)
add_test(NAME test_async COMMAND logicmill_test --test-case=*async*)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
