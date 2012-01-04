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
#include "rungidl.hpp"
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
vector<string> generateListOfSlowMXsatFiles() {
	vector<string> testdirs {"satmxlongrunning/"};
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}

class MXnbTest: public ::testing::TestWithParam<string> {
};

class MXsatTest: public ::testing::TestWithParam<string> {
};

class LazyMXnbTest: public ::testing::TestWithParam<string> {
};
class SlowMXnbTest: public ::testing::TestWithParam<string> {
};

void throwexc() {
	throw exception();
}

TEST_P(MXnbTest, DoesMX) {
	string testfile(getTestDirectory() + "mxnbofmodelstest.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

/*TEST_P(MXnbTest, DoesMXWithBounds) {
	string testfile(string(TESTDIR) + "mxnbofmodelstestwithbounds.idp");
	cerr << "Testing " << string(TESTDIR) + GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { string(TESTDIR) + GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}*/

TEST_P(MXsatTest, DoesMX) {
	string testfile(getTestDirectory() + "mxsattest.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

TEST_P(SlowMXnbTest, DoesSlowMX) {
	string testfile(getTestDirectory() + "mxsattestslow.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

/*TEST_P(LazyMXnbTest, DoesMX) {
	string testfile(getTestDirectory() + "mxlazynbofmodelstest.idp");
	cerr << "Testing " << string(TESTDIR) + GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { string(TESTDIR) + GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}*/

INSTANTIATE_TEST_CASE_P(ModelExpansion, MXnbTest, ::testing::ValuesIn(generateListOfMXnbFiles()));

INSTANTIATE_TEST_CASE_P(ModelExpansion, MXsatTest, ::testing::ValuesIn(generateListOfMXsatFiles()));

INSTANTIATE_TEST_CASE_P(LazyModelExpansion, LazyMXnbTest, ::testing::ValuesIn(generateListOfMXnbFiles()));

#ifdef NDEBUG
INSTANTIATE_TEST_CASE_P(ModelExpansionLong, SlowMXnbTest, ::testing::ValuesIn(generateListOfSlowMXsatFiles()));
#endif

TEST(MakeTrueTest, Correct) {
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { getTestDirectory() + "mx/maketrue.idp" }););
	ASSERT_EQ(result, Status::SUCCESS);
}

}
