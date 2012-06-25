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
#include <cstdio>

#include "gtest/gtest.h"
#include "external/rungidl.hpp"
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"

#include <exception>

using namespace std;

namespace Tests {

vector<string> generateListOfMXnbFiles() {
	vector<string> testdirs { "simplemx/", "numberknown/", "nontotal/" };
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}
vector<string> generateListOfMXsatFiles() {
	vector<string> testdirs { "satmx/" };
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}

class MXnbTest: public ::testing::TestWithParam<string> {
};
class MXsatTest: public ::testing::TestWithParam<string> {
};

TEST_P(MXnbTest, DoesMX) {
	runTests("modelexpansion.idp", GetParam(), "mxnobounds()");
}

TEST_P(MXnbTest, DoesMXWithBounds) {
	runTests("modelexpansion.idp", GetParam(), "mxwithbounds()");
}

/*TEST_P(MXnbTest, DoesMXWithSharedTseitinsAndBounds) {
	runTests("modelexpansion.idp", GetParam(), "mxwithSharedTseitins()");
}*/

TEST_P(MXnbTest, DoesMXWithSymmetryBreaking) {
	runTests("modelexpansion.idp", GetParam(), "mxwithsymm()");
}

TEST_P(MXnbTest, DoesMXWithLazyTseitinDelaying) {
	runTests("modelexpansion.idp", GetParam(), "mxlazy()");
}

/*
TEST_P(MXnbTest, DoesMXWithCP) {
	runTests("modelexpansion.idp", GetParam(), "mxwithcp()");
}
*/

TEST_P(MXnbTest, DoesMXWithoutPushingNegationsOrFlattening) {
	runTests("modelexpansionwithoutpushingnegations.idp", GetParam());
}

TEST_P(MXsatTest, DoesMX) {
	runTests("satisfiability.idp", GetParam(), "satnobounds()");
}

TEST_P(MXsatTest, DoesMXWithBounds) {
	runTests("satisfiability.idp", GetParam(), "satwithbounds()");
}

/*
TEST_P(MXsatTest, DoesMXWithCP) {
	runTests("satisfiability.idp", GetParam(), "satwithcp()");
}
*/

INSTANTIATE_TEST_CASE_P(ModelExpansion, MXnbTest, ::testing::ValuesIn(generateListOfMXnbFiles()));
INSTANTIATE_TEST_CASE_P(ModelExpansion, MXsatTest, ::testing::ValuesIn(generateListOfMXsatFiles()));

TEST(MakeTrueTest, Correct) {
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { getTestDirectory() + "mx/maketrue.idp" }););
	ASSERT_EQ(Status::SUCCESS, result);
}

TEST(TrailTest, NonEmptyTrail) {
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { getTestDirectory() + "trailtest.idp" }););
	ASSERT_EQ(Status::SUCCESS, result);
}

TEST(MXnbmodelsTest, DoesMX) {
	string testfile(getTestDirectory() + "mx/nbmodels.idp");
	cerr << "Testing " << testfile << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { testfile }););
	ASSERT_EQ(Status::SUCCESS, result);
}

/**
 * TODO
 */
/*TEST_P(MXnbTest, WriteOutAndSolve) {
	string testfile(getTestDirectory() + GetParam());
	cerr << "Testing " << testfile << "\n";
	Status result = Status::FAIL;

	auto tempfile = string(tmpnam(NULL));
	stringstream ss;
	ss << "main(" << tempfile << ")";
	ASSERT_NO_THROW( result = test({GetParam(), testfile}, ss.str()););
}*/

}
