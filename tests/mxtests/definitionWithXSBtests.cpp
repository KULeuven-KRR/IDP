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
#include "errorhandling/IdpException.hpp"

#include <dirent.h>
#include <exception>

using namespace std;

namespace Tests {

// All files with theories in them that depend on twovalued opens
vector<string> generateListOfTwovaluedDefsFiles() {
	vector<string> testdirs {"definitionWithXSBtests/twovalued_opens/",
		"definitiontests/"};
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}
// All test files that should throw an exception
vector<string> generateListOfFailingDefXSBFiles() {
	vector<string> testdirs {"definitionShouldFailWithXSBTests/"};
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}
// All files with theories in them that depend on three- or twovalued opens
vector<string> generateListOfThreevaluedDefsFiles() {
	vector<string> testdirs {"definitionWithXSBtests/threevalued_opens/",
		"definitionWithXSBtests/twovalued_opens/", "definitiontests/"};
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}

class CalculateDefinitionsWithXSBTest: public ::testing::TestWithParam<string> {};
class CalculateDefinitionsForVocWithXSBTest: public ::testing::TestWithParam<string> {};
class DefinitionShouldFailWithXSBTest: public ::testing::TestWithParam<string> {};
class RefineDefinitionsWithXSBTest: public ::testing::TestWithParam<string> {};


// 1: MX With stdoptions.xsb = true
//---------------------------------

// Normal MX tests with xsb=true
TEST_P(MXnbTest, DoesMXWithXSB) {
	runTests("modelexpansion.idp", GetParam(), "mxwithxsb()");
}

// Tests on all normal MX test files that uses a random structure for the opens to check
// whether the definition evaluation is correct
TEST_P(MXnbTest, DoesMXWithXSBOnRandomStruct) {
	string testfile(getTestDirectory() + "calculatedefinitionsWithXSBtest.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }, "checkForRandomInputStructure(T,S,10)"););
	ASSERT_EQ(result, Status::SUCCESS);
}


// 2: Tests for the "calculatedefinitions(t,s)" call with stdoptions.xsb=true
//---------------------------------------------------------------------------

// Dedicated calculate definition test from the definitiontests and definitionWithXSBtests
// These two directories are not put together because the tests in definitionWithXSBtests do not contain the assertions
TEST_P(CalculateDefinitionsWithXSBTest, CalculatesDefinitionWithXSB) {
	string testfile(getTestDirectory() + "calculatedefinitionsWithXSBtest.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }, "checkDefinitionEvaluation(T,S)"););
	ASSERT_EQ(result, Status::SUCCESS);
}
INSTANTIATE_TEST_CASE_P(CalculateDefinitionsWithXSB, CalculateDefinitionsWithXSBTest,  ::testing::ValuesIn(generateListOfTwovaluedDefsFiles()));

// Test whether calculatedefinitions with the full vocabulary as third argument is the same as
// calculatedefinitions with no third argument
TEST_P(CalculateDefinitionsForVocWithXSBTest, CalculatesDefinitionForVocWithXSB) {
	string testfile(getTestDirectory() + "calculatedefinitionsWithXSBtest.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }, "checkDefinitionEvaluationWithVoc(T,S,V)"););
	ASSERT_EQ(result, Status::SUCCESS);
}
INSTANTIATE_TEST_CASE_P(CalculateDefinitionsWithXSB, CalculateDefinitionsForVocWithXSBTest,  ::testing::ValuesIn(generateListOfTwovaluedDefsFiles()));


// Separate tests for cases in which an error should be thrown when trying to use XSB
TEST_P(DefinitionShouldFailWithXSBTest, CalculatesFailingDefinitionWithXSB) {
	string testfile(getTestDirectory() + "calculatedefinitionsWithXSBtest.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }, "runCalcDefWithXSB(T,S)"););
	ASSERT_EQ(result, Status::FAIL);
}
INSTANTIATE_TEST_CASE_P(CalculateDefinitionsWithXSB, DefinitionShouldFailWithXSBTest,  ::testing::ValuesIn(generateListOfFailingDefXSBFiles()));


}
