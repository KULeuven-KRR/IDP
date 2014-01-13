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
#include "FileEnumerator.hpp"

#include <dirent.h>
#include <exception>

using namespace std;

namespace Tests {

vector<string> generateListOfDefXSBFiles() {
	vector<string> testdirs {"definitionWithXSBtests/", "definitiontests/"};
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}

class DefinitionWithXSBTest: public ::testing::TestWithParam<string> {

};

// Normal MX tests with xsb=true
TEST_P(MXnbTest, DoesMXWithXSB) {
	runTests("modelexpansion.idp", GetParam(), "mxwithxsb()");
}

// Dedicated calculate definition test from the definitiontests and definitionWithXSBtests
// These two directories are not put together because the tests in definitionWithXSBtests do not contain the assertions
TEST_P(DefinitionWithXSBTest, CalculatesDefinitionWithXSB) {
	string testfile(getTestDirectory() + "calculatedefinitionsWithXSBtest.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }, "checkDefinitionEvaluation(T,S)"););
	ASSERT_EQ(result, Status::SUCCESS);
}

INSTANTIATE_TEST_CASE_P(CalculateDefinitionsWithXSB, DefinitionWithXSBTest,  ::testing::ValuesIn(generateListOfDefXSBFiles()));



// Tests on all normal MX test files that uses a random structure for the opens to check
// whether the definition evaluation is correct
TEST_P(MXnbTest, DoesMXWithXSBOnRandomStruct) {
	string testfile(getTestDirectory() + "calculatedefinitionsWithXSBtest.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }, "checkForRandomInputStructure(T,S,10)"););
	ASSERT_EQ(result, Status::SUCCESS);
}


}

