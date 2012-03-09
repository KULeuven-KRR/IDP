/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include <cmath>

#include "gtest/gtest.h"
#include "external/rungidl.hpp"
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"

#include <exception>

using namespace std;

namespace Tests {

vector<string> generateListOfSlowMXsatFiles() {
	vector<string> testdirs {"satmxlongrunning/"};
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}
vector<string> generateListOfMXsatFiles() {
	vector<string> testdirs {"satmx/"};
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}

class MXsatTest: public ::testing::TestWithParam<string> {
};

void throwexc() {
	throw exception();
}

void runTests(const char* inferencefilename, const string& instancefile){
	string testfile(getTestDirectory() + inferencefilename);
	cerr << "Testing " << instancefile << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { instancefile, testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

class SlowMXsatTest: public ::testing::TestWithParam<string> {
};
TEST_P(SlowMXsatTest, DoesSlowMX) {
	runTests("mxsattestslow.idp", GetParam());
}
TEST_P(SlowMXsatTest, DoesSlowMXWithBounds) {
	runTests("mxsattestwithbounds.idp", GetParam());
}

TEST_P(MXsatTest, DoesMXWithBounds) {
	runTests("mxsattestwithbounds.idp", GetParam());
}

TEST_P(MXsatTest, DoesMXWithSymmetryBreaking) {
	runTests("mxsattestwithsymmetrybreaking.idp", GetParam());
}

INSTANTIATE_TEST_CASE_P(ModelExpansionLong, SlowMXsatTest, ::testing::ValuesIn(generateListOfSlowMXsatFiles()));
INSTANTIATE_TEST_CASE_P(ModelExpansionLong, MXsatTest, ::testing::ValuesIn(generateListOfMXsatFiles()));

}




