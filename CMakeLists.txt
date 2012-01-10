cmake_minimum_required(VERSION 2.8)
project (idp)

# The version number.
set (idp_VERSION_MAJOR 3)
set (idp_VERSION_MINOR 0)

function(AssureOutOfSourceBuilds)
	# make sure the user doesn't play dirty with symlinks
	get_filename_component(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
	get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)

	# disallow in-source builds
	if(${srcdir} STREQUAL ${bindir})
		message("######################################################")
		message("You are attempting to build in your Source Directory.")
		message("You must run cmake from a build directory.")
		message("######################################################")

		# attempt to remove cache and cache files... this actually fails to work,
		# but no hurt trying incase it starts working..
		file(REMOVE_RECURSE "${CMAKE_SOURCE_DIR}/CMakeCache.txt" "${CMAKE_SOURCE_DIR}/CMakeFiles")

		message(FATAL_ERROR "In-source builds are forbidden!")
	endif()
	
	# check for polluted source tree
	if(EXISTS ${CMAKE_SOURCE_DIR}/CMakeCache.txt OR EXISTS ${CMAKE_SOURCE_DIR}/CMakeFiles)
		message("######################################################")
		message( "Found results from an in-source build in your source directory.")
		message("######################################################")
	
		# attempt to remove cache and cache files...
		file(REMOVE_RECURSE "${CMAKE_SOURCE_DIR}/CMakeCache.txt" "${CMAKE_SOURCE_DIR}/CMakeFiles")
	
		message(FATAL_ERROR "Source Directory Cleaned, please rerun CMake.")
	endif()
endfunction()

AssureOutOfSourceBuilds()

OPTION(BUILDTESTS  "Run the gtest tests" ON)

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

### GMP HANDLING
OPTION(GMP  "Enable gmp support" OFF)
if(GMP)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D GMP")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D NO_GMP")
endif()

### CP HANDLING 
OPTION(CP  "Enable cp support" OFF)
if(CP)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D CPSUPPORT")
endif()


# Set default compile flags for GCC
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall ")
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra ")
endif()

if(CMAKE_COMPILER_IS_MINGW)
	message(STATUS "Mingw compiler")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc ")
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "StaticRelease")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -O3 -m32 ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -DNDEBUG ")
	set(BUILD_SHARED_LIBS OFF)
	set(BUILD_IDP_STATIC ON)
else()
	if(${CMAKE_BUILD_TYPE} MATCHES "Release")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG ")
		set(BUILD_SHARED_LIBS ON)
		handleRPATHUnix()
	else() #debug builds
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb -DDEBUG -D_GLIBCXX_DEBUG ")
		if(${CMAKE_BUILD_TYPE} MATCHES "StaticDebug")
			set(BUILD_SHARED_LIBS OFF) # to allow stepping
			set(BUILD_IDP_STATIC ON)   # to allow stepping
		else()
			if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
				set(BUILD_SHARED_LIBS ON)
				handleRPATHUnix()
			else()
				message(ERROR "Unsupported build-type, only Debug, StaticDebug, Release and StaticRelease are allowed.")
			endif()
		endif()
	endif()
endif()

MESSAGE(STATUS "Global CXX flags: ")
MESSAGE(STATUS "${CMAKE_CXX_FLAGS}")
MESSAGE(STATUS "build idp shared ${BUILD_SHARED_LIBS}")
if(BUILD_IDP_STATIC)
	MESSAGE(STATUS "build idp allstatic")
	set(STATIC_LUA ON)
endif()

add_subdirectory(lib/minisatid)
add_subdirectory(lib/linenoise)
if(NOT TARGET gtest AND BUILDTESTS)
	add_subdirectory(lib/gtest-1.6.0)
endif() 
add_subdirectory(lib/lua)
add_subdirectory(lib/tinythread/source)
add_subdirectory(src)
add_subdirectory(tests)

include(CPack)
