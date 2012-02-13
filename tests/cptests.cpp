/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "gtest/gtest.h"
#include "external/rungidl.hpp"
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"

#include <exception>

using namespace std;

namespace Tests {
/*
vector<string> generateListOfSimpleCPFiles() {
	vector<string> testdirs {"cp/"};
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}

class CPTest: public ::testing::TestWithParam<string> {
};

TEST_P(CPTest, SimpleCP) {
	string testfile(getTestDirectory() + "mxsattestwithcp.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

INSTANTIATE_TEST_CASE_P(SimpleCP, CPTest, ::testing::ValuesIn(generateListOfSimpleCPFiles()));
*/
} /* namespace Tests */
