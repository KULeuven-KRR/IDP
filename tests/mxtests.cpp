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
vector<string> generateListOfMXFiles() {
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

vector<string> generateListOfLazyMXFiles() {
	vector<string> mxtests;
	DIR *dir;
	struct dirent *ent;
	vector<string> testdirs {"simplemxtests/", "applicationmxtests/" };
	for (auto currTestDir = testdirs.cbegin(); currTestDir != testdirs.cend(); ++currTestDir) {
		dir = opendir((string(TESTDIR) + "lazymxtests/" + (*currTestDir)).c_str());
		if (dir != NULL) {
			while ((ent = readdir(dir)) != NULL) {
				if (ent->d_name[0] != '.') {
					mxtests.push_back("lazymxtests/" +(*currTestDir) + ent->d_name);
				}
			}
			closedir(dir);
		} else {
			cerr << "FAIL    |  Could not open directory of lazy MX tests.\n";
		}
	}
	return mxtests;
}

class MXTest: public ::testing::TestWithParam<string> {

};

class LazyMXTest: public ::testing::TestWithParam<string> {

};

void throwexc() {
	throw exception();
}

TEST_P(MXTest, DoesMX) {
	string testfile(string(TESTDIR) + "mxnbofmodelstest.idp"); // TODO TESTDIR should be one HIGHER
	cerr << "Testing " << string(TESTDIR) + GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { string(TESTDIR) + GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

TEST_P(LazyMXTest, DoesMX) {
	string testfile(string(TESTDIR) + "mxlazynbofmodelstest.idp"); // TODO TESTDIR should be one HIGHER
	cerr << "Testing " << string(TESTDIR) + GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { string(TESTDIR) + GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

INSTANTIATE_TEST_CASE_P(ModelExpansion, MXTest, ::testing::ValuesIn(generateListOfMXFiles()));

INSTANTIATE_TEST_CASE_P(LazyModelExpansion, LazyMXTest, ::testing::ValuesIn(generateListOfLazyMXFiles()));
}
