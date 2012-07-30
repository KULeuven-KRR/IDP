cmake_minimum_required(VERSION 2.8)
		
OPTION(USEINTERACTIVE "allow user-interaction" ON)
		
addToCXX("-std=c++0x")
addToCXX("-D __STDC_FORMAT_MACROS -D __STDC_LIMIT_MACROS")
addToCXX("-std=gnu++0x") # necessary for gtest!
if(USEINTERACTIVE)
	addToCXX("-D USEINTERACTIVE")
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
set(MXINCLUDES TestUtils.hpp TestUtils.cpp testingtools.hpp testingtools.cpp testsuite.cpp mxtests/FileEnumerator.hpp mxtests/FileEnumerator.cpp)

set(TESTBINARIES "")
set(TESTCOMMANDBINARIES "")
set(EXECS "")

macro(addTest execname)
	target_link_libraries(${execname} idp gtest)
	if(UNIX)
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
	set(TESTCOMMANDBINARIES ${TESTCOMMANDBINARIES} COMMAND ${PROJECT_BINARY_DIR}/tests/${execname})
endmacro()

set(TESTINGTHREADS "1" CACHE STRING "The number of threads to run in parallel for testing")

if(${BUILDTESTS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D TESTDIR=\\\"${PROJECT_SOURCE_DIR}/tests\\\" ")
	set(FASTTESTS 
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
	)
	if(UNIX)
		add_executable(idptests
			${FASTTESTS}
			${MXINCLUDES}
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
		add_custom_target(check VERBATIM COMMAND ${PROJECT_BINARY_DIR}/lib/multiprocessrun/multiprocessrun ${TESTINGTHREADS} ${TESTBINARIES} | ${PROJECT_BINARY_DIR}/lib/gtestparser/gtestparser DEPENDS ${EXECS} multiprocessrun gtestparser)
		add_custom_target(fullcheck VERBATIM COMMAND ${PROJECT_BINARY_DIR}/lib/multiprocessrun/multiprocessrun ${TESTINGTHREADS} ${TESTBINARIES} ${PROJECT_BINARY_DIR}/tests/slow DEPENDS ${EXECS} slow multiprocessrun)
	else()
		set(LONGERTESTS
			mxtests/basic.cpp
			mxtests/bounds.cpp
			mxtests/nobounds.cpp
			mxtests/cp.cpp
			mxtests/lazy.cpp
			mxtests/symm.cpp
			mxtests/optim.cpp
			mxtests/satnobounds.cpp
			mxtests/satbounds.cpp
			mxtests/satcp.cpp
		)
		add_executable(idptests
			#${FASTTESTS}
			#${LONGERTESTS}
			mxtests/bounds.cpp
			${MXINCLUDES}
		)
		addTest(idptests)
		add_executable(slow
			${FASTTESTS}
			${LONGERTESTS}
			${MXTESTS}/slow.cpp
			${MXINCLUDES}
		)
		addTest(slow)
		
		add_custom_target(check ${PROJECT_BINARY_DIR}/tests/idptests DEPENDS idptests)
		add_custom_target(fullcheck ${PROJECT_BINARY_DIR}/tests/slow DEPENDS slow)
	endif()
endif()
