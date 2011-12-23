cmake_minimum_required(VERSION 2.8)
		
OPTION(USEINTERACTIVE "allow user-interaction" ON)
		
if(USEINTERACTIVE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D USEINTERACTIVE")
endif()

include_directories ("
	"${PROJECT_SOURCE_DIR}/lib/loki/include"
	"${PROJECT_SOURCE_DIR}/lib/tinythread/source"
	"${PROJECT_SOURCE_DIR}/lib/minisatid/src/"
	"${PROJECT_SOURCE_DIR}/lib/linenoise/include"
	"${PROJECT_SOURCE_DIR}/lib/lua/src"
	"${PROJECT_SOURCE_DIR}/src"
	"${PROJECT_BUILD_DIR}/lua/src"
	"${PROJECT_SOURCE_DIR}/lib/pluscallback1.7/"
	"${PROJECT_SOURCE_DIR}/lib/linenoise/"
") 

if(${BUILDTESTS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D TESTDIR=\\\"${PROJECT_SOURCE_DIR}/tests\\\" ")
add_executable(idptests
		testsuite.cpp
		mxtests.cpp
		generatortests.cpp
		bddtests.cpp
		grounderfactorytests.cpp
		definitiontests.cpp
		propagationtests.cpp
		parsertests.cpp
		transformationtests.cpp
		cppinterface.hpp cppinterface.cpp
		testingtools.hpp testingtools.cpp
		TestUtils.hpp TestUtils.cpp
)
target_link_libraries(idptests idp gtest)
set_target_properties(idptests PROPERTIES LINK_FLAGS "-pthread ")
add_test(idptests idptests --test-command)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V DEPENDS idptests)
endif()