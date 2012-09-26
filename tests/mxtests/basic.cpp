/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "FileEnumerator.hpp"

using namespace std;

namespace Tests {

/*TEST_P(MXnbTest, DoesMXWithSharedTseitinsAndBounds) {
	runTests("modelexpansion.idp", GetParam(), "mxwithSharedTseitins()");
}*/

TEST_P(MXnbTest, DoesMXWithoutPushingNegationsOrFlattening) {
	runTests("modelexpansionwithoutpushingnegations.idp", GetParam());
}

TEST(MakeTrueTest, Correct) {
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { getTestDirectory() + "mx/maketrue.idp" }););
	ASSERT_EQ(Status::SUCCESS, result);
}

TEST(MXnbmodelsTest, CorrectNbOfModels) {
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { getTestDirectory() + "mx/generateRequestedNbOfModels.idp" }););
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

// TODO placed here, but should become a unit test internally
TEST(GeneratorTest, ExistsKnown) {
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { getTestDirectory() + "mx/exists_known_generator.idp" }););
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
