cmake_minimum_required(VERSION 3.0.0)
project(logicmill VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)


include_directories( include )

set(LOGICMILL_SRCS
	include/logicmill/buffer.h
	src/buffer.cpp)

set(LOGICMILL_TEST_SRCS
	test/doctest.h
	test/buffer.cpp
	test/test_main.cpp)

add_library(logicmill ${LOGICMILL_SRCS})

add_executable(logicmill_test ${LOGICMILL_TEST_SRCS})
target_link_libraries(logicmill_test logicmill)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
