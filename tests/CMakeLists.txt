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
		
addToCXX("-Wall -pedantic -Wno-overloaded-virtual -std=c++0x")
if(CMAKE_COMPILER_IS_GNUCXX)
	addToCXX("-Wextra")
endif()
addToCXX("-D __STDC_FORMAT_MACROS -D __STDC_LIMIT_MACROS")
addToCXX("-std=gnu++0x") # necessary for gtest!
if(USEINTERACTIVE)
	addToCXX("-D USEINTERACTIVE")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STRICT_ANSI__")
endif()

include_directories (
	"${PROJECT_SOURCE_DIR}/lib/safeint"
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
)

set(MXTESTS "mxtests")
set(TESTINCLUDES TestUtils.hpp TestUtils.cpp testingtools.hpp testingtools.cpp testsuite.cpp)
set(MXINCLUDES ${TESTINCLUDES} mxtests/FileEnumerator.hpp mxtests/FileEnumerator.cpp)

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
		tabletests.cpp
		grounderfactorytests.cpp
		parsertests.cpp
		simple_idptests.cpp
		overflowtests.cpp
		transformationtests.cpp
		commandtests.cpp
		internalmxtests.cpp
		querytests.cpp
		progressiontests.cpp
		definitiontests.cpp

		propagationtests.cpp
	)
	set(PRINTERTESTS printertest.cpp)
	
	if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		add_executable(idptests
			${FASTTESTS}
			${MXINCLUDES}
		)
		add_executable(printertests 
			${PRINTERTESTS}
			${MXINCLUDES}
		)

		addTest(idptests)
		set(EXECS ${EXECS} idptests)
		set(TESTBINARIES ${TESTBINARIES} ${PROJECT_BINARY_DIR}/tests/idptests)
		
		addTest(printertests)
		set(EXECS ${EXECS} printertests)
		set(TESTBINARIES ${TESTBINARIES} ${PROJECT_BINARY_DIR}/tests/printertests)

		#PLEASE MAKE SURE THE TESTS ARE ORDERED FROM LONGRUNNING TO SHORTRUNNING!
		#this improves the efficiency of running "make check"
		#IF ADDING TESTS HERE, ALSO ADD THEM BELOW THE ELSE

		addMXTest(bounds bounds.cpp) 
		addMXTest(boundsNoLUP boundsNoLUP.cpp)
		if(XSBUSED)
		    addMXTest(xsbtest definitionWithXSBtests.cpp)
		endif()
		addMXTest(satbounds satbounds.cpp) 
		addMXTest(satcp satcp.cpp)
		addMXTest(cp cp.cpp) 
		addMXTest(lazy lazy.cpp)  
		addMXTest(verylazy verylazy.cpp)
		if(SPASS)
		    addMXTest(funcdetect funcdetect.cpp)
		endif() 
		addMXTest(satnobounds satnobounds.cpp) 
		addMXTest(basic basic.cpp) 
        addMXTest(mxIterator mxIterator.cpp)
		addMXTest(nobounds nobounds.cpp) 
		addMXTest(symm symm.cpp) 
		addMXTest(optim optim.cpp)
		addMXTest(focard focard.cpp)
		addMXTest(metarep metarep.cpp)
		

		add_executable(slow ${MXTESTS}/slow.cpp ${MXINCLUDES})
		addTest(slow)
		
		if(SPASS)
			add_executable(entailment EntailmentTests.cpp ${TESTINCLUDES})
		    addTest(entailment)
		    set(TESTBINARIES ${TESTBINARIES} ${PROJECT_BINARY_DIR}/tests/entailment)
		endif()
		
		add_custom_target(check VERBATIM COMMAND ${PROJECT_BINARY_DIR}/lib/multiprocessrun/multiprocessrun ${TESTINGTHREADS} ${TESTBINARIES} | ${PROJECT_BINARY_DIR}/lib/gtestparser/gtestparser DEPENDS ${EXECS} multiprocessrun gtestparser)
		add_custom_target(fullcheck VERBATIM COMMAND ${PROJECT_BINARY_DIR}/lib/multiprocessrun/multiprocessrun ${TESTINGTHREADS} ${TESTBINARIES} ${PROJECT_BINARY_DIR}/tests/slow DEPENDS ${EXECS} slow multiprocessrun)
	else()
		# ALSO ADD ABOVE ELSE!
		set(LONGERTESTS
			mxtests/basic.cpp
            mxtests/focard.cpp
			mxtests/bounds.cpp
			mxtests/nobounds.cpp
			mxtests/boundsNoLUP.cpp
			mxtests/cp.cpp
			mxtests/lazy.cpp
			mxtests/verylazy.cpp
			mxtests/symm.cpp
			mxtests/optim.cpp
			mxtests/satnobounds.cpp
			mxtests/satbounds.cpp
			mxtests/satcp.cpp
			mxtests/metarep.cpp
		)
        
        if(XSBUSED)
        	set(LONGERTESTS ${LONGERTESTS} mxtests/definitionWithXSBtests.cpp)
        endif()

        if(SPASS)
        	set(LONGERTESTS ${LONGERTESTS} mxtests/funcdetect.cpp)
            set(LONGERTESTS ${LONGERTESTS} EntailmentTests.cpp ${TESTINCLUDES})
        endif()

		add_executable(idptests
			${FASTTESTS}
			${PRINTERTESTS}
			${LONGERTESTS}
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
