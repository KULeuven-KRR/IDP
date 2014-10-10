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
set (idp_VERSION_MINOR 4)
set (idp_VERSION_PATCH 0)
set (idp_VERSION "${idp_VERSION_MAJOR}.${idp_VERSION_MINOR}.${idp_VERSION_PATCH}")

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

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

macro(alsoClean ARGUMENT)
    set(ADDITIONAL_CLEAN_FILES_TO_CLEAN "${ADDITIONAL_CLEAN_FILES_TO_CLEAN};${ARGUMENT}")
endmacro()

### Fix for newest ubuntu+gcc
IF("${CMAKE_SYSTEM}" MATCHES "Linux")
	addToCXX("-Wl,--no-as-needed")
endif()

### Rpath handling
include(cmake/rpathhandling.cmake)

### Target properties helper 
include(cmake/settargetproperties.cmake)

addToCXX("-static-libstdc++")

if(CMAKE_COMPILER_IS_MINGW)
	message(STATUS "Mingw compiler")
	addToCXX("-static-libgcc")
	addToCXX("-Wl,--stack,4194304")
endif()

### Gmp handling
OPTION(GMP "Enable gmp support" OFF)
if(GMP)
	addToCXX("-D GMP")
	find_package(GMP)
	if(GMP_FOUND)
		include_directories(${GMP_INCLUDE_DIR})
		set(LIBS ${LIBS} ${GMP_LIBRARIES})
		set(LIBS ${LIBS} ${GMPXX_LIBRARIES})
	else()
		message(FATAL_ERROR "GMP library not found")
	endif()
else()
	addToCXX("-D NO_GMP")
	message("WARNING: this IntegerType may not be suitable for some input file. Consider using GMP")
endif()

### Gecode option for minisatid
option(WITHGECODE "Use gecode for cp support" OFF)
if(WITHGECODE)
	set(WITHGECODE ON CACHE BOOL "") # the minisatid option
endif()

### 32 bits
option(32BITS "Compile for 32 bit architectures" OFF)
if(32BITS)
	addToCXX("-m32")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
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
	addToCXX("-DNDEBUG -O3 ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG ")
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
if(WITHXSB AND (NOT 32BITS)) # Don't use XSB for 32Bits, this results in currently unfixable segfaults
    message(STATUS "compile with XSB support: ON")
    addToCXX("-D WITHXSB")
    set(XSB_VERSION "3.5.0")
    set(XSB_INSTALL_URL "/bin/xsb/xsb-${XSB_VERSION}/")
    addToCXX("-D XSB_INSTALL_URL=\\\\\"\"${XSB_INSTALL_URL}/\\\\\"\" ") # This has a bunch of layers of \ characters, because it gets evaluated a lot... don't ask. Version below also works (less unnesting)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D XSB_INSTALL_URL2=\\\"\"${XSB_INSTALL_URL}/\\\"\" ")
    set(XSB_COPIED_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/XSB")
    set(XSB_BUILD_DIR "${XSB_COPIED_SOURCE_DIR}/build")
    add_custom_target(copy_xsb_source_code)
    file(GLOB_RECURSE XSB_SOURCE_FILES ${PROJECT_SOURCE_DIR}/lib/XSB/*)
    add_custom_command(
        TARGET copy_xsb_source_code PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/lib/XSB  ${XSB_COPIED_SOURCE_DIR})
    alsoClean(${XSB_COPIED_SOURCE_DIR})

    set(XSB_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/xsbinstall")
    alsoClean(${XSB_INSTALL_DIR})

#     if (NOT EXISTS ${XSB_COPIED_SOURCE_DIR})
#         # copy XSB source directory
#         message(STATUS "Copying XSB source directory, this might take a while...")
#         file(COPY ${PROJECT_SOURCE_DIR}/lib/XSB DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#         message(STATUS "Copying XSB source directory, this might take a while... - done")
#     endif()

    execute_process(COMMAND ${XSB_COPIED_SOURCE_DIR}/build/config.guess 
                    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/XSB_CONFIG)
    file (STRINGS "${CMAKE_CURRENT_BINARY_DIR}/XSB_CONFIG" XSB_CONFIG_STRING)
    if (32BITS)   # Currently never used, since it's filtered out in the top-level if above
      set(XSB_CONFIG_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/configure --prefix=${XSB_INSTALL_DIR} --with-bits32 -with-optimization=fPIC)
      set(XSB_BUILD_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/makexsb --config-tag=bits32)
      set(XSB_INSTALL_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/makexsb install --config-tag=bits32)
      set(XSB_CONFIG_STRING ${XSB_CONFIG_STRING}-bits32)
    else()
      set(XSB_CONFIG_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/configure --prefix=${XSB_INSTALL_DIR} -with-optimization=fPIC)
      set(XSB_BUILD_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/makexsb)
      set(XSB_INSTALL_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/makexsb install)
    endif()

    include_directories (
        "${XSB_COPIED_SOURCE_DIR}/emu"
        "${XSB_COPIED_SOURCE_DIR}/config/${XSB_CONFIG_STRING}"
    )
    file(MAKE_DIRECTORY ${XSB_BUILD_DIR}/dummy) # Needed to stop complaining from adding XSB project - souce dir specified has to be non-empty
    ExternalProject_Add(
      xsb
      SOURCE_DIR ${XSB_COPIED_SOURCE_DIR}/build
      CONFIGURE_COMMAND ${XSB_CONFIG_COMMAND}
      BUILD_COMMAND ${XSB_BUILD_COMMAND}
      INSTALL_COMMAND ${XSB_INSTALL_COMMAND}
      BUILD_IN_SOURCE 1
    )
    add_dependencies(xsb copy_xsb_source_code)
else()
    message(STATUS "compile with XSB support: OFF")
    if(WITHXSB AND 32BITS)
        message(STATUS "  .. because currently 32 bit compilation is not supported with XSB")
    endif()
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
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
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

message(STATUS "make clean cleans the following additional files:")
message(STATUS "${ADDITIONAL_CLEAN_FILES_TO_CLEAN}")
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${ADDITIONAL_CLEAN_FILES_TO_CLEAN}")