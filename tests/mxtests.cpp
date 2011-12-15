/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include <cmath>

#include "gtest/gtest.h"
#include "rungidl.hpp"

#include <dirent.h>
#include <exception>

using namespace std;

namespace Tests {

vector<string> getAllFilesInDirs(const vector<string>& testdirs){
	vector<string> mxtests;
	DIR *dir;
	struct dirent *ent;
	for (auto currTestDir = testdirs.cbegin(); currTestDir != testdirs.cend(); ++currTestDir) {
		dir = opendir((string(TESTDIR) + "mx/" + (*currTestDir)).c_str());
		if (dir != NULL) {
			while ((ent = readdir(dir)) != NULL) {
				if (ent->d_name[0] != '.') {
					mxtests.push_back("mx/" + (*currTestDir) +ent->d_name);
				}
			}
			closedir(dir);
		} else {
			cerr << "FAIL    |  Could not open directory of MX tests.\n";
		}
	}
	return mxtests;
}

vector<string> generateListOfMXnbFiles() {
	vector<string> testdirs {"simplemx/"};
	return getAllFilesInDirs(testdirs);
}
vector<string> generateListOfMXsatFiles() {
	vector<string> testdirs {"satmx/"};
	return getAllFilesInDirs(testdirs);
}
vector<string> generateListOfSlowMXsatFiles() {
	vector<string> testdirs {"satmxlongrunning/"};
	return getAllFilesInDirs(testdirs);
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
	string testfile(string(TESTDIR) + "mxnbofmodelstest.idp");
	cerr << "Testing " << string(TESTDIR) + GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { string(TESTDIR) + GetParam(), testfile }););
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
	string testfile(string(TESTDIR) + "mxsattest.idp");
	cerr << "Testing " << string(TESTDIR) + GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { string(TESTDIR) + GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

TEST_P(SlowMXnbTest, DoesSlowMX) {
	string testfile(string(TESTDIR) + "mxsattestslow.idp");
	cerr << "Testing " << string(TESTDIR) + GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { string(TESTDIR) + GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

/*TEST_P(LazyMXnbTest, DoesMX) {
	string testfile(string(TESTDIR) + "mxlazynbofmodelstest.idp");
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
}
