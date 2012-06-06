cmake_minimum_required(VERSION 2.8) 
 		
OPTION(USEINTERACTIVE "allow user-interaction" ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
if(USEINTERACTIVE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D USEINTERACTIVE")
endif()

include_directories (
	"${PROJECT_SOURCE_DIR}/lib/loki/include"
	"${PROJECT_SOURCE_DIR}/lib/tinythread/source"
	"${PROJECT_SOURCE_DIR}/lib/minisatid/src/external"
	"${PROJECT_SOURCE_DIR}/lib/linenoise/include"
	"${PROJECT_SOURCE_DIR}/lib/lua/src"
	"${PROJECT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${PROJECT_SOURCE_DIR}/lib/pluscallback1.7/"
	"${PROJECT_SOURCE_DIR}/lib/linenoise/"
	"${PROJECT_SOURCE_DIR}/lib/tinythread/"
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D GIDLVERSION=\\\"${idp_VERSION}\\\" ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D IDPINSTALLDIR=\\\"\"${CMAKE_INSTALL_PREFIX}/\\\"\" ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D INTERNALLIBARYLUA=\\\"share/std/idp_intern.lua\\\" ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D INTERNALLIBARYIDP=\\\"share/std/idp_intern.idp\\\" ") 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D CONFIGFILENAME=\\\"var/.idprc\\\" ")
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread") 
endif()  

 
find_package(BISON)
find_package(FLEX)

BISON_TARGET(myparser parser/parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cc)
FLEX_TARGET(myscanner parser/lex.ll  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cc)
ADD_FLEX_BISON_DEPENDENCY(myscanner myparser)

set(LIBGIDLCPPFILES "")
file(GLOB_RECURSE LIBGIDLCPPFILES "${PROJECT_SOURCE_DIR}/src" "*.cpp")
file(GLOB_RECURSE LIBGIDLHPPFILES "${PROJECT_SOURCE_DIR}/src" "*.hpp")
LIST(REMOVE_ITEM LIBGIDLCPPFILES "${PROJECT_SOURCE_DIR}/src/external/main.cpp")

add_library(idp
	${LIBGIDLHPPFILES}
	${LIBGIDLCPPFILES}
	${BISON_myparser_OUTPUTS} ${FLEX_myscanner_OUTPUTS}
)

if(NOT CMAKE_COMPILER_IS_MINGW)
	target_link_libraries(idp dl)
endif()
target_link_libraries(idp minisatid lua linenoise tinythread)

add_executable(idp-bin external/main.cpp)
if(BUILD_IDP_STATIC)
	set_target_properties(idp-bin PROPERTIES LINK_FLAGS "-static ")
	set_target_properties(idp PROPERTIES LINK_FLAGS "-static ")
endif()
target_link_libraries(idp-bin idp)
set_target_properties(idp-bin PROPERTIES OUTPUT_NAME idp)

set_target_properties(
	idp idp-bin PROPERTIES
	VERSION ${idp_VERSION}
)

set(DATAFILES "")
file(GLOB DATAFILES "${PROJECT_SOURCE_DIR}/data/share/std/*.idp")
#message(STATUS "Datafiles ${DATAFILES}")

# add the install targets
install (TARGETS idp-bin DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install (TARGETS idp DESTINATION lib)
install (FILES ${PROJECT_SOURCE_DIR}/scripts/runidp.sh ${PROJECT_SOURCE_DIR}/scripts/runidp.bat DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install (FILES ${PROJECT_SOURCE_DIR}/data/var/.idprc DESTINATION var)
install (FILES ${DATAFILES} ${PROJECT_SOURCE_DIR}/data/share/std/idp_intern.lua 
		DESTINATION share/std)

#?SET(CPACK_PACKAGE_EXECUTABLES "${CPACK_PACKAGE_EXECUTABLES} idp runidp.bat runidp.sh")
