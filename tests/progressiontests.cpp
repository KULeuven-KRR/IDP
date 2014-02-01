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

#include <cmath>

#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"
#include "testingtools.hpp"
#include "IncludeComponents.hpp"
#include "structure/StructureComponents.hpp"

#include <exception>

using namespace std;

namespace Tests {


vector<string> generateListOfProgressionOKFiles() {
	vector<string> testdirs { "noException/" };
	return getAllFilesInDirs(getTestDirectory() + "progression/", testdirs);
}
vector<string> generateListOfProgressionFailFiles() {
	vector<string> testdirs { "exception/" };
	return getAllFilesInDirs(getTestDirectory() + "progression/", testdirs);
}
vector<string> generateListOfInvariantFiles() {
	vector<string> testdirs { "invariants/" };
	return getAllFilesInDirs(getTestDirectory() + "progression/", testdirs);
}

class ProgressionTest: public ::testing::TestWithParam<string> {
};
class ProgressionTestException: public ::testing::TestWithParam<string> {
};
class InvarTest: public ::testing::TestWithParam<string> {
};

TEST_P(ProgressionTest, DoesProgression) {
	runTests("progressiontest.idp", GetParam());
}

TEST_P(ProgressionTest, DoesProgressionWithBuiltinSimulation) {
	runTests("progressiontest.idp", GetParam(), "alternative()");
}

TEST_P(ProgressionTestException, DoesProgression) {
	cerr << "Testing " << GetParam() << "\n";
	ASSERT_EQ(Status::FAIL, test({ GetParam()}));
}
TEST_P(InvarTest, FindsInvariantWithMX) {
	runTests("invartest.idp", GetParam(),"main(false)");
}
TEST_P(InvarTest, FindsInvariantWithProver) {
	runTests("invartest.idp", GetParam(),"main(true)");
}

INSTANTIATE_TEST_CASE_P(Progression, ProgressionTest, ::testing::ValuesIn(generateListOfProgressionOKFiles()));
INSTANTIATE_TEST_CASE_P(ProgressionThrows, ProgressionTestException, ::testing::ValuesIn(generateListOfProgressionFailFiles()));
INSTANTIATE_TEST_CASE_P(Invariant, InvarTest, ::testing::ValuesIn(generateListOfInvariantFiles()));

}


