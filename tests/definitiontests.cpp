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
vector<string> generateListOfDefFiles() {
	vector<string> deftests;
	DIR *dir;
	struct dirent *ent;
	std::string defdir = "definitiontests/";
	dir = opendir((string(TESTDIR) + defdir).c_str());
	if (dir != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_name[0] != '.') {
				deftests.push_back(defdir + ent->d_name);
			}
		}
		closedir(dir);
	} else {
		cerr << "FAIL    |  Could not open directory of definition tests.\n";
	}

	return deftests;
}

class DefinitionTest: public ::testing::TestWithParam<string> {

};


TEST_P(DefinitionTest, CalculatesDefinition) {
	string testfile(string(TESTDIR) + "calculatedefinitionstest.idp");
	cerr << "Testing " << string(TESTDIR) + GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { string(TESTDIR) + GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

INSTANTIATE_TEST_CASE_P(Inferences, DefinitionTest,  ::testing::ValuesIn(generateListOfDefFiles()));
}
