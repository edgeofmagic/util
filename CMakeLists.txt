cmake_minimum_required(VERSION 3.0.0)
project(util VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "CMAKE_HOME_DIRECTORY = ${CMAKE_HOME_DIRECTORY}")

	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fsanitize=address")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
#	else()
#		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3")
	endif()

enable_testing()

set(Boost_USE_STATIC_LIBS   ON)

set(Boost_NO_SYSTEM_PATHS TRUE)
if (Boost_NO_SYSTEM_PATHS)
	set(BOOST_ROOT "${CMAKE_HOME_DIRECTORY}/../boost_1_68_0_install")
endif (Boost_NO_SYSTEM_PATHS)

find_package(Boost 1.68.0 REQUIRED system)

#LINK_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/../boost_1_68_0_install/lib)

include_directories( 
	include
	${Boost_INCLUDE_DIRS}
	ext/doctest/include)

set(UTIL_SRCS
	src/util/error_context.cpp
	src/util/buffer.cpp
	src/util/error.cpp)
#	src/fs_impl.cpp)

set(UTIL_TEST_SRCS

	test/util/buffer.cpp
	test/util/allocators.cpp
	test/util/shared_ptr.cpp
	test/util/promise.cpp
	test/util/membuf.cpp
	test/util/error_context.cpp
	test/test_main.cpp)

add_library(util ${UTIL_SRCS})

add_executable(util_test ${UTIL_TEST_SRCS})
target_link_libraries(util_test util ${Boost_LIBRARIES})

add_test(NAME util_test COMMAND util_test )
SET_TESTS_PROPERTIES(util_test
    PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=1")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(util_doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)
