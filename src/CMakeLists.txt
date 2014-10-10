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

OPTION(USEINTERACTIVE "allow user-interaction" ON)

addToCXX("-std=c++0x")
if(USEINTERACTIVE)
	addToCXX("-D USEINTERACTIVE")
endif()

MESSAGE(STATUS "${PROJECT_SOURCE_DIR}")

include_directories (
	"${PROJECT_SOURCE_DIR}/lib/loki/include"
	"${PROJECT_SOURCE_DIR}/lib/safeint"
	"${PROJECT_SOURCE_DIR}/lib/tinythread/source"
	"${PROJECT_SOURCE_DIR}/lib/minisatid/src/external"
	"${PROJECT_SOURCE_DIR}/lib/lua/src"
	"${PROJECT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${PROJECT_SOURCE_DIR}/lib/interactiveShell/"
	"${PROJECT_SOURCE_DIR}/lib/tinythread/"
)
addToCXX("-Wall -pedantic -Wno-overloaded-virtual -std=c++0x")
if(CMAKE_COMPILER_IS_GNUCXX)
	addToCXX("-Wextra")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D GIDLVERSION=\\\"${idp_VERSION}\\\" ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D IDPINSTALLDIR=\\\"\"${CMAKE_INSTALL_PREFIX}/\\\"\" ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D INTERNALLIBARYLUA=\\\"share/std/idp_intern.lua\\\" ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D INTERNALLIBARYIDP=\\\"share/std/idp_intern.idp\\\" ") 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D CONFIGFILENAME=\\\"var/config.idp\\\" ")
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	addToCXX("-pthread") 
endif()

set(IDPFILES "")
 
find_package(BISON)
find_package(FLEX)

BISON_TARGET(myparser parser/parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
FLEX_TARGET(myscanner parser/lex.ll  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
ADD_FLEX_BISON_DEPENDENCY(myscanner myparser)
set(IDPFILES "${IDPFILES};${BISON_myparser_OUTPUTS};${FLEX_myscanner_OUTPUTS}")

set(LIBGIDLCPPFILES "")
file(GLOB_RECURSE LIBGIDLCPPFILES "${PROJECT_SOURCE_DIR}/src/" "*.cpp")
file(GLOB_RECURSE LIBGIDLHPPFILES "${PROJECT_SOURCE_DIR}/src/" "*.hpp")
LIST(REMOVE_ITEM LIBGIDLCPPFILES "${PROJECT_SOURCE_DIR}/src/external/main.cpp")
if(NOT XSBUSED)
	file(GLOB XSBFILES "${PROJECT_SOURCE_DIR}/src/inferences/querying/xsb/*.hpp")
	file(GLOB XSBFILES "${PROJECT_SOURCE_DIR}/src/inferences/querying/xsb/*.cpp")
	LIST(REMOVE_ITEM LIBGIDLHPPFILES ${XSBFILES}) 
	LIST(REMOVE_ITEM LIBGIDLCPPFILES ${XSBFILES})
endif()
set(IDPFILES "${IDPFILES};${LIBGIDLHPPFILES};${LIBGIDLCPPFILES}")

if(XSBUSED)
	set_source_files_properties(
		${XSB_COPIED_SOURCE_DIR}/config/${XSB_CONFIG_STRING}/saved.o/xsb.o PROPERTIES
		EXTERNAL_OBJECT TRUE  # Identifies this as an object file
		GENERATED TRUE  # Avoids need for file to exist at configure-time
	)
	set(IDPFILES "${IDPFILES};${XSB_COPIED_SOURCE_DIR}/config/${XSB_CONFIG_STRING}/saved.o/xsb.o")
endif()

add_library(idp ${IDPFILES})

if(XSBUSED)
endif()

if(NOT CMAKE_COMPILER_IS_MINGW)
	target_link_libraries(idp dl)
endif()
if(GPERF)
	target_link_libraries(idp tcmalloc_and_profiler)
endif()
if(WIN32)
	target_link_libraries(idp psapi)
endif()
target_link_libraries(idp minisatid lua linenoise tinythread)

add_executable(idp-bin external/main.cpp)
target_link_libraries(idp-bin idp)

if(BUILD_IDP_STATIC)
	set_target_properties(idp-bin PROPERTIES LINK_FLAGS "-static ")
	set_target_properties(idp PROPERTIES LINK_FLAGS "-static ")
endif()

target_link_libraries(idp-bin idp minisatid lua linenoise tinythread)
if(XSBUSED)
	target_link_libraries(idp-bin dl)
endif()

set_target_properties(idp-bin PROPERTIES OUTPUT_NAME idp)

set_target_properties(idp-bin PROPERTIES 
	OUTPUT_NAME kbs
	VERSION ${idp_VERSION}
)

set(DATAFILES "")
file(GLOB DATAFILES "${PROJECT_SOURCE_DIR}/data/share/std/*.idp")

# add the install targets
install (TARGETS idp-bin DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install (TARGETS idp DESTINATION lib)
if(UNIX)
	install (FILES ${PROJECT_SOURCE_DIR}/scripts/unix/idp DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
else()
	install (FILES ${PROJECT_SOURCE_DIR}/scripts/windows/idp.bat DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

install (FILES ${PROJECT_SOURCE_DIR}/data/var/config.idp DESTINATION var)
install (FILES ${DATAFILES} ${PROJECT_SOURCE_DIR}/data/share/std/idp_intern.lua
		 DESTINATION share/std)

if(XSBUSED)
	install (FILES ${PROJECT_SOURCE_DIR}/data/share/std/xsb_compiler.P
		DESTINATION share/std)
	install (DIRECTORY ${XSB_INSTALL_DIR}/xsb-${XSB_VERSION}/bin/
		DESTINATION bin/xsb/xsb-${XSB_VERSION}/bin FILES_MATCHING PATTERN "*"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	install (DIRECTORY ${XSB_INSTALL_DIR}/xsb-${XSB_VERSION}/config/
		DESTINATION bin/xsb/xsb-${XSB_VERSION}/config FILES_MATCHING PATTERN "*"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	install (DIRECTORY ${XSB_INSTALL_DIR}/xsb-${XSB_VERSION}/syslib/
		DESTINATION bin/xsb/xsb-${XSB_VERSION}/syslib FILES_MATCHING PATTERN "*"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	install (DIRECTORY ${XSB_INSTALL_DIR}/xsb-${XSB_VERSION}/lib/
		DESTINATION bin/xsb/xsb-${XSB_VERSION}/lib FILES_MATCHING PATTERN "*"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	install (DIRECTORY ${XSB_INSTALL_DIR}/xsb-${XSB_VERSION}/build/
		DESTINATION bin/xsb/xsb-${XSB_VERSION}/build FILES_MATCHING PATTERN "*"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	install (DIRECTORY ${XSB_INSTALL_DIR}/xsb-${XSB_VERSION}/cmplib/
		DESTINATION bin/xsb/xsb-${XSB_VERSION}/cmplib FILES_MATCHING PATTERN "*"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	install (DIRECTORY ${XSB_INSTALL_DIR}/xsb-${XSB_VERSION}/etc/
		DESTINATION bin/xsb/xsb-${XSB_VERSION}/etc FILES_MATCHING PATTERN "*"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()
