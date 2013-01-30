/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "TestUtils.hpp"
#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "utils/FileManagement.hpp"


using namespace std;

std::string getTestDirectory() {
	return string(TESTDIR) + '/';
}

namespace Tests {
void runTests(const char* inferencefilename, const string& instancefile, const std::string& command) {
	string testfile(getTestDirectory() + inferencefilename);
	cerr << "Testing " << instancefile << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( {instancefile, testfile}, command););
	ASSERT_EQ(Status::SUCCESS, result);
}
void throwexc() {
	throw exception();
}
}
