cmake_minimum_required(VERSION 2.8)
project (idp)

# The version number.
set (idp_VERSION_MAJOR 3)
set (idp_VERSION_MINOR 0)

OPTION(BUILDTESTS  "Run the gtest tests" ON)

ENABLE_TESTING()

OPTION(GMP  "Enable gmp support" OFF)
if(GMP)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D GMP")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D NO_GMP")
endif()

OPTION(CP  "Enable cp support" OFF)
if(CP)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D CPSUPPORT")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra ")

if(${CMAKE_BUILD_TYPE} MATCHES "StaticRelease")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
	set(BUILD_SHARED_LIBS OFF)
else()
	if(${CMAKE_BUILD_TYPE} MATCHES "Release")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
		set(BUILD_SHARED_LIBS ON)
	else() # debug
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb")
		set(BUILD_SHARED_LIBS ON)	
	endif()
endif()

MESSAGE(STATUS "Global CXX flags: ")
MESSAGE(STATUS "${CMAKE_CXX_FLAGS}")

add_subdirectory(lib/minisatid)
add_subdirectory(lib/linenoise)
add_subdirectory(lib/lua)
add_subdirectory(src)
add_subdirectory(tests)

include(CPack)
