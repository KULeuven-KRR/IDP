cmake_minimum_required(VERSION 2.8)
		
OPTION(USEINTERACTIVE "allow user-interaction" ON)
		
addToCXX("-std=c++0x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x") # necessary for gtest!
if(USEINTERACTIVE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D USEINTERACTIVE")
endif()

include_directories ("
	"${PROJECT_SOURCE_DIR}/lib/loki/include"
	"${PROJECT_SOURCE_DIR}/lib/tinythread/source"
	"${PROJECT_SOURCE_DIR}/lib/minisatid/src/external"
	"${PROJECT_SOURCE_DIR}/lib/linenoise/include"
	"${PROJECT_SOURCE_DIR}/lib/lua/src"
	"${PROJECT_SOURCE_DIR}/src"
	"${PROJECT_BUILD_DIR}/lua/src"
	"${PROJECT_SOURCE_DIR}/lib/pluscallback1.7/"
	"${PROJECT_SOURCE_DIR}/lib/gtest-1.6.0/include/"
	"${PROJECT_SOURCE_DIR}/lib/linenoise/"
	"${PROJECT_SOURCE_DIR}/tests/"
")

set(MXTESTS "mxtests")
set(MXINCLUDES TestUtils.hpp TestUtils.cpp testsuite.cpp mxtests/FileEnumerator.hpp mxtests/FileEnumerator.cpp)

set(TESTBINARIES "")
set(EXECS "")

macro(addTest execname)
	target_link_libraries(${execname} idp gtest)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set_target_properties(${execname} PROPERTIES LINK_FLAGS "-pthread ")
	endif()
	add_test(${execname} ${execname} "--gtest_color=yes")
endmacro()

macro(addMXTest execname cppfile)
	add_executable(${execname}
		${MXTESTS}/${cppfile}
		${MXINCLUDES}
	)
	addTest(${execname})
	set(EXECS ${EXECS} ${execname})
	set(TESTBINARIES ${TESTBINARIES} ${PROJECT_BINARY_DIR}/tests/${execname})
endmacro()

if(${BUILDTESTS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D TESTDIR=\\\"${PROJECT_SOURCE_DIR}/tests\\\" ")
	add_executable(idptests
		simpletests.cpp
		generatortests.cpp
		bddtests.cpp
		bddestimatortests.cpp
		grounderfactorytests.cpp
		parsertests.cpp
		overflowtests.cpp
		transformationtests.cpp
		commandtests.cpp
		internalmxtests.cpp
		querytests.cpp
		definitiontests.cpp
		propagationtests.cpp
		testingtools.hpp testingtools.cpp
		TestUtils.hpp TestUtils.cpp
		testsuite.cpp
	)
	addTest(idptests)
	set(EXECS ${EXECS} idptests)
	set(TESTBINARIES ${TESTBINARIES} ${PROJECT_BINARY_DIR}/tests/idptests)
	
	addMXTest(basic basic.cpp)
	addMXTest(bounds bounds.cpp)
	addMXTest(nobounds nobounds.cpp)
	addMXTest(cp cp.cpp)
	addMXTest(lazy lazy.cpp)
	addMXTest(symm symm.cpp)
	addMXTest(optim optim.cpp)
	addMXTest(satnobounds satnobounds.cpp)
	addMXTest(satbounds satbounds.cpp)
	addMXTest(satcp satcp.cpp)
	
	add_executable(slow
		${MXTESTS}/slow.cpp
		${MXINCLUDES}
	)
	addTest(slow)
	
	add_custom_target(check VERBATIM COMMAND ${PROJECT_BINARY_DIR}/lib/multiprocessrun/multiprocessrun 4 ${TESTBINARIES} | ${PROJECT_BINARY_DIR}/lib/gtestparser/gtestparser DEPENDS ${EXECS} multiprocessrun gtestparser)
	add_custom_target(fullcheck COMMAND ${PROJECT_BINARY_DIR}/lib/multiprocessrun/multiprocessrun 3 ${TESTBINARIES} ${PROJECT_BINARY_DIR}/tests/slow DEPENDS ${EXECS} slow)
endif()
