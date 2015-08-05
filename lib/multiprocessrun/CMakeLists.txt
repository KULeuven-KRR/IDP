cmake_minimum_required(VERSION 2.8)
project (multiprocessrun)

# The version number.
set (MPR_VERSION_MAJOR 1)
set (MPR_VERSION_MINOR 0)

ENABLE_TESTING()

### RPATH HANDLING
function(handleRPATHUnix)
	# use, i.e. don't skip the full RPATH for the build tree
	SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
	
	# when building, don't use the install RPATH already (but later on when installing)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
	SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
	
	# add the automatically determined parts of the RPATH which point to directories outside the build tree to the install RPATH
	SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	
	# the RPATH to be used when installing, but only if it's not a system directory
	LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
	IF("${isSystemDir}" STREQUAL "-1")
	   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
	ENDIF()
endfunction()

# Set default compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall ")
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra ")
endif()

set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build.")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "DebugProfile" "Release" "StaticRelease" "StaticDebug" "Profile")

if(${CMAKE_BUILD_TYPE} MATCHES "^Profile$")
	addToCXX("-DNDEBUG -O3 -pg ")
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^DebugProfile$")
	addToCXX("-O0 -ggdb -DDEBUG -pg ")
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^StaticRelease$")
	addToCXX("-DNDEBUG -O3 -m32 ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -DNDEBUG ")
	set(BUILD_SHARED_LIBS OFF)
	set(BUILD_MINISATID_STATIC ON)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^Release$")
	addToCXX("-O3 -DNDEBUG ")
	set(BUILD_SHARED_LIBS ON)
	handleRPATHUnix()
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^StaticDebug$")
	addToCXX("-O0 -ggdb -DDEBUG -D_GLIBCXX_DEBUG ")
	set(BUILD_SHARED_LIBS OFF) # to allow stepping
	set(BUILD_IDP_STATIC ON)   # to allow stepping
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^Debug$")
	addToCXX("-O0 -ggdb -DDEBUG -D_GLIBCXX_DEBUG ")
	set(BUILD_SHARED_LIBS ON)
	handleRPATHUnix()
endif()

MESSAGE(STATUS "Multiprocess CXX flags: ")
MESSAGE(STATUS "${CMAKE_CXX_FLAGS}")

add_executable(multiprocessrun multiprocessrun.cpp)
target_link_libraries(multiprocessrun pthread)

install (TARGETS multiprocessrun DESTINATION bin)
