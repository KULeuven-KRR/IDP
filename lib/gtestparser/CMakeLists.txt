cmake_minimum_required(VERSION 2.8)
project (gtestparser)

# The version number.
set (GTestParser_VERSION_MAJOR 2)
set (GTestParser_VERSION_MINOR 3)

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

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra ")
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "StaticRelease")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -O3 -m32 ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -DNDEBUG ")
	set(BUILD_SHARED_LIBS OFF)
	set(BUILD_MINISATID_STATIC ON)
else()
	if(${CMAKE_BUILD_TYPE} MATCHES "Release")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG ")
		set(BUILD_SHARED_LIBS ON)
	else()
		if(${CMAKE_BUILD_TYPE} MATCHES "StaticDebug")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb -DDEBUG ")
			set(BUILD_SHARED_LIBS OFF) # to allow stepping
			set(BUILD_IDP_STATIC ON)   # to allow stepping
		else() #default debug
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb -DDEBUG ")
			set(BUILD_SHARED_LIBS ON)
			handleRPATHUnix()
		endif()
	endif()
endif()

MESSAGE(STATUS "Global CXX flags: ")
MESSAGE(STATUS "${CMAKE_CXX_FLAGS}")

find_package(BISON)
find_package(FLEX)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

BISON_TARGET(myparser ecnfparser.ypp ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
FLEX_TARGET(myscanner ecnftoken.lpp  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
ADD_FLEX_BISON_DEPENDENCY(myscanner myparser)

add_executable(gtestparser Main.cpp ${BISON_myparser_OUTPUTS} ${FLEX_myscanner_OUTPUTS})

include(CPack)

install (TARGETS gtestparser DESTINATION bin)
