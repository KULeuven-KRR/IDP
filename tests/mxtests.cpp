/*
 * Copyright 2007-2011 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat and Maarten Mariën, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <cmath>

#include "gtest/gtest.h"
#include "rungidl.hpp"

#include <dirent.h>
#include <exception>

using namespace std;

namespace Tests {

// TODO prevent infinite running bugs
// TODO on parsing error of one of the files, a lot of later ones will also fail!
vector<string> generateListOfMXnbFiles() {
	vector<string> mxtests;
	DIR *dir;
	struct dirent *ent;
	vector<string> testdirs {"simplemxtests/", "applicationmxtests/" }; //TODO automize
	for (auto currTestDir = testdirs.cbegin(); currTestDir != testdirs.cend(); ++currTestDir) {
		dir = opendir((string(TESTDIR) + "mxtests/" + (*currTestDir)).c_str());
		if (dir != NULL) {
			while ((ent = readdir(dir)) != NULL) {
				if (ent->d_name[0] != '.') {
					mxtests.push_back("mxtests/" + (*currTestDir) +ent->d_name);
				}
			}
			closedir(dir);
		} else {
			cerr << "FAIL    |  Could not open directory of MX tests.\n";
		}
	}
	return mxtests;
}
vector<string> generateListOfMXsatFiles() {
	vector<string> mxtests;
	DIR *dir;
	struct dirent *ent;
	vector<string> testdirs {"SATmxtests/" }; //TODO automize
	for (auto currTestDir = testdirs.cbegin(); currTestDir != testdirs.cend(); ++currTestDir) {
		dir = opendir((string(TESTDIR) + "mxtests/" + (*currTestDir)).c_str());
		if (dir != NULL) {
			while ((ent = readdir(dir)) != NULL) {
				if (ent->d_name[0] != '.') {
					mxtests.push_back("mxtests/" + (*currTestDir) +ent->d_name);
				}
			}
			closedir(dir);
		} else {
			cerr << "FAIL    |  Could not open directory of MX tests.\n";
		}
	}
	return mxtests;
}

vector<string> generateListOfLazyMXnbFiles() {
	vector<string> mxtests;
	DIR *dir;
	struct dirent *ent;
	vector<string> testdirs {"simplemxtests/", "applicationmxtests/" };
	for (auto currTestDir = testdirs.cbegin(); currTestDir != testdirs.cend(); ++currTestDir) {
		dir = opendir((string(TESTDIR) + "mxtests/" + (*currTestDir)).c_str());
		if (dir != NULL) {
			while ((ent = readdir(dir)) != NULL) {
				if (ent->d_name[0] != '.') {
					mxtests.push_back("mxtests/" +(*currTestDir) + ent->d_name);
				}
			}
			closedir(dir);
		} else {
			cerr << "FAIL    |  Could not open directory of lazy MX tests.\n";
		}
	}
	return mxtests;
}

class MXnbTest: public ::testing::TestWithParam<string> {

};

class MXsatTest: public ::testing::TestWithParam<string> {

};

class LazyMXnbTest: public ::testing::TestWithParam<string> {

};

void throwexc() {
	throw exception();
}

TEST(ParsingTest, Fail){
	string testfile1(string(TESTDIR) + "mxtests/parseerror.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, OverloadedType){
	string testfile1(string(TESTDIR) + "mxtests/parseerror2.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, IdenticalType){
	string testfile1(string(TESTDIR) + "mxtests/parseerror3.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, IdenticalFunction){
	string testfile1(string(TESTDIR) + "mxtests/parseerror4.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, IdenticalPredicate){
	string testfile1(string(TESTDIR) + "mxtests/parseerror5.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, FailAndContinue){
	string testfile1(string(TESTDIR) + "mxtests/parseerror.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(RestartTest, FailAndContinue){
	string testfile1(string(TESTDIR) + "mxtests/parseerror.idp");
	string testfile2(string(TESTDIR) + "mxtests/simplemxtests/atom.idp");
	string testfilemx(string(TESTDIR) + "mxlazynbofmodelstest.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1, testfilemx }));
	ASSERT_EQ(Status::SUCCESS, test( { testfile2, testfilemx }));
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

/*TEST_P(LazyMXnbTest, DoesMX) {
	string testfile(string(TESTDIR) + "mxlazynbofmodelstest.idp");
	cerr << "Testing " << string(TESTDIR) + GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { string(TESTDIR) + GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}*/

INSTANTIATE_TEST_CASE_P(ModelExpansion, MXnbTest, ::testing::ValuesIn(generateListOfMXnbFiles()));

INSTANTIATE_TEST_CASE_P(ModelExpansion, MXsatTest, ::testing::ValuesIn(generateListOfMXsatFiles()));

INSTANTIATE_TEST_CASE_P(LazyModelExpansion, LazyMXnbTest, ::testing::ValuesIn(generateListOfLazyMXnbFiles()));
}
