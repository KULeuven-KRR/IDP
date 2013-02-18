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
#include "TestUtils.hpp"
#include "utils/FileManagement.hpp"

#include <dirent.h>
#include <exception>

using namespace std;

namespace Tests {

vector<string> generateListOfDefFiles() {
	vector<string> testdirs {"definitiontests/"};
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}

class DefinitionTest: public ::testing::TestWithParam<string> {

};


TEST_P(DefinitionTest, CalculatesDefinition) {
	string testfile(getTestDirectory() + "calculatedefinitionstest.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }););
	ASSERT_EQ(result, Status::SUCCESS);
}

INSTANTIATE_TEST_CASE_P(CalculateDefinitions, DefinitionTest,  ::testing::ValuesIn(generateListOfDefFiles()));
}
