cmake_minimum_required(VERSION 2.8)
		
OPTION(USEINTERACTIVE "allow user-interaction" ON)
		
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x") # necessary for gtest!
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
	"${PROJECT_SOURCE_DIR}/lib/gtest-1.6.0/include/"
	"${PROJECT_SOURCE_DIR}/lib/linenoise/"
") 

if(${BUILDTESTS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D TESTDIR=\\\"${PROJECT_SOURCE_DIR}/tests\\\" ")
	add_executable(idptests
			testsuite.cpp
			mxtests.cpp
			simpletests.cpp
			testingtools.hpp testingtools.cpp
			TestUtils.hpp TestUtils.cpp
	)
	target_link_libraries(idptests idp gtest)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set_target_properties(idptests PROPERTIES LINK_FLAGS "-pthread ")
	endif()
	add_test(idptests idptests "--gtest_color=yes")
	add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V DEPENDS idptests)
endif()
