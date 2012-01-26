cmake_minimum_required(VERSION 2.8)
project (idp)

### The version number
set (idp_VERSION_MAJOR 3)
set (idp_VERSION_MINOR 0)
set (idp_VERSION_PATCH 2)

include(cmake/outofsourcecheck.cmake)
assureOutOfSourceBuilds()

option(BUILDTESTS  "Run the gtest tests" ON)
enable_testing()

### Adding to cxx flags
macro(addToCXX ARGUMENT)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARGUMENT}")
endmacro()

### Rpath handling
include(cmake/rpathhandling.cmake)

### Gmp handling
option(GMP  "Enable gmp support" OFF)
if(GMP)
	addToCXX("-D GMP")
else()
	addToCXX("-D NO_GMP")
endif()

### Cp handling 
option(CP  "Enable cp support" OFF)
if(CP)
	set(WITHCP ON CACHE BOOL "")
endif()


### Set default compile flags
addToCXX("-std=c++0x -Wall -std=c++0x ")
if(CMAKE_COMPILER_IS_GNUCXX)
	addToCXX("-Wextra -pedantic ")
endif()

if(CMAKE_COMPILER_IS_MINGW)
	message(STATUS "Mingw compiler")
	addToCXX("-static-libgcc ")
endif()

set(WRONGBUILD ON)
if(${CMAKE_BUILD_TYPE} MATCHES "^Profile")
	addToCXX("-DNDEBUG -O3 -pg ")
	set(WRONGBUILD OFF)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^StaticRelease")
	addToCXX("-DNDEBUG -O3 -m32 ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -DNDEBUG ")
	set(BUILD_SHARED_LIBS OFF)
	set(BUILD_IDP_STATIC ON)
	set(WRONGBUILD OFF)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^Release")
	addToCXX("-O3 -DNDEBUG ")
	set(BUILD_SHARED_LIBS ON)
	handleRPATHUnix()
	set(WRONGBUILD OFF)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^StaticDebug")
	addToCXX("-O0 -ggdb -DDEBUG -D_GLIBCXX_DEBUG ")
	set(BUILD_SHARED_LIBS OFF) # to allow stepping
	set(BUILD_IDP_STATIC ON)   # to allow stepping
	set(WRONGBUILD OFF)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^Debug")
	addToCXX("-O0 -ggdb -DDEBUG -D_GLIBCXX_DEBUG ")
	set(BUILD_SHARED_LIBS ON)
	handleRPATHUnix()
	set(WRONGBUILD OFF)
endif()
if(WRONGBUILD)
	message(FATAL_ERROR "Unsupported build-type \"${CMAKE_BUILD_TYPE}\", only Debug, StaticDebug, Release and StaticRelease are allowed.")
endif()

option(GPERF  "Enable profiling support with google-perftools" OFF)
if(GPERF)
	set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -lprofiler -ltcmalloc")
endif()

### Printing information
message(STATUS "Global CXX flags: ")
message(STATUS "${CMAKE_CXX_FLAGS}")
message(STATUS "build idp shared ${BUILD_SHARED_LIBS}")
if(BUILD_IDP_STATIC)
	message(STATUS "build idp allstatic")
	# TODO does not work as option( call comes later: set(STATIC_LUA ON)
endif()
message(STATUS "Rpath for building is \"${CMAKE_BUILD_RPATH}\"")
message(STATUS "Rpath for installation is \"${CMAKE_INSTALL_RPATH}\"")

### Adding subprojects
add_subdirectory(lib/minisatid)
add_subdirectory(lib/linenoise)
if(NOT TARGET gtest AND BUILDTESTS)
	add_subdirectory(lib/gtest-1.6.0)
endif() 
add_subdirectory(lib/gtestparser)
add_subdirectory(lib/lua)
add_subdirectory(lib/tinythread/source)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(docs/official)

### Allow packing
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING.LESSER")
set(CPACK_PACKAGE_VERSION_MAJOR	${idp_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${idp_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${idp_VERSION_PATCH})
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_SOURCE_GENERATOR TGZ)
# The following components are regex's to match anywhere (unless anchored)
# in absolute path + filename to find files or directories to be excluded
# from source tarball.
set(CPACK_SOURCE_IGNORE_FILES
	"^${PROJECT_SOURCE_DIR}/.*"
	"^${PROJECT_SOURCE_DIR}/.git/"
	"^${PROJECT_SOURCE_DIR}/builds/"
	"^${PROJECT_SOURCE_DIR}/build/"
	"^${PROJECT_SOURCE_DIR}/docs/visuals"
	"^${PROJECT_SOURCE_DIR}/docs/code"
	"^${PROJECT_SOURCE_DIR}/docs/technical"
)
message(STATUS "source package name: ${CPACK_SOURCE_PACKAGE_FILE_NAME}")
message(STATUS "source package ignores: ${CPACK_SOURCE_IGNORE_FILES}")
include(CPack)