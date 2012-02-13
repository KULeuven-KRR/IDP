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

vector<string> generateListOfMXnbFiles() {
	vector<string> testdirs {"simplemx/", "numberknown/"};
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}
vector<string> generateListOfMXsatFiles() {
	vector<string> testdirs {"satmx/"};
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}

class MXnbTest: public ::testing::TestWithParam<string> {
};
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
	ASSERT_EQ(Status::SUCCESS, result);
}

TEST_P(MXnbTest, DoesMX) {
	runTests("mxnbofmodelstest.idp", GetParam());
}

TEST_P(MXnbTest, DoesMXWithSymmetryBreaking) {
	runTests("mxnbofmodelstestwithsymmetrybreaking.idp", GetParam());
}

TEST_P(MXnbTest, DoesMXWithCP) {
	runTests("mxnbofmodelstestwithcp.idp", GetParam());
}

// TODO when bdds are implemented
/*TEST_P(MXnbTest, DoesMXWithBounds) {
	runTests("mxnbofmodelstestwithbounds.idp");
}*/

TEST_P(MXsatTest, DoesMX) {
	runTests("mxsattest.idp", GetParam());
}

TEST_P(MXsatTest, DoesMXWithSymmetryBreaking) {
	runTests("mxsattestwithsymmetrybreaking.idp", GetParam());
}

TEST_P(MXsatTest, DoesMXWithCP) {
	runTests("mxsattestwithcp.idp", GetParam());
}

INSTANTIATE_TEST_CASE_P(ModelExpansion, MXnbTest, ::testing::ValuesIn(generateListOfMXnbFiles()));
INSTANTIATE_TEST_CASE_P(ModelExpansion, MXsatTest, ::testing::ValuesIn(generateListOfMXsatFiles()));

TEST(MakeTrueTest, Correct) {
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { getTestDirectory() + "mx/maketrue.idp" }););
	ASSERT_EQ(Status::SUCCESS, result);
}

}
