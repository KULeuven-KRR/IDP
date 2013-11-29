#-------------------------------------------------------------------------------
#  Copyright 2010-2012 Katholieke Universiteit Leuven
# 
#  Use of this software is governed by the GNU LGPLv3.0 license
# 
#  Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
#  Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
#  K.U.Leuven, Departement Computerwetenschappen,
#  Celestijnenlaan 200A, B-3001 Leuven, Belgium
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8)
project (idp)

### The version number
set (idp_VERSION_MAJOR 3)
set (idp_VERSION_MINOR 2)
set (idp_VERSION_PATCH 1)
set (idp_VERSION "${idp_VERSION_MAJOR}.${idp_VERSION_MINOR}.${idp_VERSION_PATCH}")

include(cmake/outofsourcecheck.cmake)
assureOutOfSourceBuilds()

option(BUILDTESTS  "Run the gtest tests" ON)
enable_testing()
option(BUILDDOCUMENTATION "Build the documentation" ON)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Buildtype (one of Release, Debug, StaticRelease, StaticDebug or Profile")

message(STATUS "Install path (CMAKE_INSTALL_PREFIX): ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build type (CMAKE_BUILD_TYPE): ${CMAKE_BUILD_TYPE}")

### Adding to cxx flags NOTE: do not use when escape characters matter
macro(addToCXX ARGUMENT)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARGUMENT}")
endmacro()

### Fix for newest ubuntu+gcc
addToCXX("-Wl,--no-as-needed")

### Rpath handling
include(cmake/rpathhandling.cmake)

### Target properties helper 
include(cmake/settargetproperties.cmake)

if(CMAKE_COMPILER_IS_MINGW)
	message(STATUS "Mingw compiler")
	addToCXX("-static-libgcc -static-libstdc++ ")
	addToCXX("-Wl,--stack,4194304")
endif()

### Gmp handling
option(GMP  "Enable gmp support" OFF)
if(GMP)
	addToCXX("-D GMP")
else()
	addToCXX("-D NO_GMP")
endif()

### Gecode option for minisatid
option(WITHGECODE "Use gecode for cp support" OFF)
if(WITHGECODE)
	set(WITHGECODE ON CACHE BOOL "") # the minisatid option
endif()

set(WRONGBUILD ON)
if(${CMAKE_BUILD_TYPE} MATCHES "^Profile$")
	addToCXX("-DNDEBUG -O3 -pg ")
	set(WRONGBUILD OFF)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^DebugProfile$")
	addToCXX("-O0 -ggdb -DDEBUG -pg ")
	set(WRONGBUILD OFF)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^StaticRelease$")
	addToCXX("-DNDEBUG -O3 -m32 ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -DNDEBUG ")
	set(BUILD_SHARED_LIBS OFF)
	set(BUILD_IDP_STATIC ON)
	set(WRONGBUILD OFF)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^Release$")
	addToCXX("-O3 -DNDEBUG ")
	set(BUILD_SHARED_LIBS ON)
	handleRPATHUnix()
	set(WRONGBUILD OFF)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^StaticDebug$")
	addToCXX("-O0 -ggdb -DDEBUG -D_GLIBCXX_DEBUG ")
	set(BUILD_SHARED_LIBS OFF) # to allow stepping
	set(BUILD_IDP_STATIC ON)   # to allow stepping
	set(WRONGBUILD OFF)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^Debug$")
	addToCXX("-O0 -ggdb -DDEBUG -D_GLIBCXX_DEBUG ")
	set(BUILD_SHARED_LIBS ON)
	handleRPATHUnix()
	set(WRONGBUILD OFF)
endif()
if(WRONGBUILD)
	message(FATAL_ERROR "Unsupported build-type \"${CMAKE_BUILD_TYPE}\", only Debug, DebugProfile, StaticDebug, Profile, Release and StaticRelease are allowed.")
endif()

if(UNIX)
	addToCXX("-D UNIX")
endif()

option(GPERF  "Enable profiling support with google-perftools" OFF)
if(GPERF)
	addToCXX("-fno-omit-frame-pointer")
endif()

### Printing information
message(STATUS "IDP CXX flags: ")
message(STATUS "${CMAKE_CXX_FLAGS}")
message(STATUS "build idp shared ${BUILD_SHARED_LIBS}")
if(BUILD_IDP_STATIC)
	message(STATUS "build idp allstatic")
	SET(STATIC_LUA ON CACHE BOOL "Build lua static library") # only this works correctly
endif()
message(STATUS "Rpath for building is \"${CMAKE_BUILD_RPATH}\"")
message(STATUS "Rpath for installation is \"${CMAKE_INSTALL_RPATH}\"")


### Adding subprojects
include(ExternalProject)

#SPASS
option(SPASS  "Compile and install the SPASS theorem prover" ON)
if(SPASS)
    ExternalProject_Add(
      spass
      SOURCE_DIR ${PROJECT_SOURCE_DIR}/lib/SPASS-3.7
      CONFIGURE_COMMAND ${PROJECT_SOURCE_DIR}/lib/SPASS-3.7/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/spass
      BUILD_IN_SOURCE 0
    )
    install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/spass/bin/ DESTINATION bin FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
else()
    message(STATUS "SPASS will not be installed automatically. To be able to use it, install it manually and change the appropriate call in your config.idp file.")
endif()

#XSB
option(WITHXSB  "Compile with XSB support" ON)
if(WITHXSB)
    execute_process(COMMAND ${PROJECT_SOURCE_DIR}/lib/XSB/build/config.guess 
    				OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/XSB_CONFIG)
    file (STRINGS "${CMAKE_CURRENT_BINARY_DIR}/XSB_CONFIG" XSB_CONFIG_STRING)
    set(XSB_BUILD_DIR "${PROJECT_SOURCE_DIR}/lib/XSB")
    ExternalProject_Add(
      xsb
      SOURCE_DIR ${PROJECT_SOURCE_DIR}/lib/XSB/build
      CONFIGURE_COMMAND ${PROJECT_SOURCE_DIR}/lib/XSB/build/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/xsbinstall/ -with-optimization=fPIC
      BUILD_COMMAND ${PROJECT_SOURCE_DIR}/lib/XSB/build/makexsb
      INSTALL_COMMAND ${PROJECT_SOURCE_DIR}/lib/XSB/build/makexsb install
      BUILD_IN_SOURCE 1
    )
    install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/xsbinstall/ DESTINATION bin/xsb FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    set(XSB_INSTALL_URL "/bin/xsb/3.3.7/")
endif()

#REST
add_subdirectory(lib/minisatid)
add_subdirectory(lib/interactiveShell)
if(NOT TARGET gtest AND BUILDTESTS)
	add_subdirectory(lib/gtest-1.6.0)
endif() 
add_subdirectory(lib/gtestparser)
add_subdirectory(lib/lua)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	add_subdirectory(lib/multiprocessrun)
endif()
add_subdirectory(lib/tinythread/source)
add_subdirectory(src)
add_subdirectory(tests)
if(BUILDDOCUMENTATION)
	add_subdirectory(docs/official)
endif()

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
# note: escape char is \\\\
set(CPACK_SOURCE_IGNORE_FILES
	"^${PROJECT_SOURCE_DIR}/\\\\..*"
	".*/.git/"
	"^${PROJECT_SOURCE_DIR}/builds/"
	"^${PROJECT_SOURCE_DIR}/build/"
	"^${PROJECT_SOURCE_DIR}/docs/visuals"
	"^${PROJECT_SOURCE_DIR}/docs/code"
	"^${PROJECT_SOURCE_DIR}/docs/technical"
)
message(STATUS "source package name: ${CPACK_SOURCE_PACKAGE_FILE_NAME}")
message(STATUS "source package ignores: ${CPACK_SOURCE_IGNORE_FILES}")
include(CPack)
