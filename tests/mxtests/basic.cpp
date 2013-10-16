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

#include "FileEnumerator.hpp"

using namespace std;

namespace Tests {

/*TEST_P(MXnbTest, DoesMXWithSharedTseitinsAndBounds) {
	runTests("modelexpansion.idp", GetParam(), "mxwithSharedTseitins()");
}*/

TEST_P(MXnbTest, DoesMXWithoutPushingNegationsOrFlattening) {
	runTests("modelexpansionwithoutpushingnegations.idp", GetParam());
}

TEST_P(SimpleMXnbTest, VerifyAllStructures) {
	runTests("modelexpansion.idp", GetParam(), "checkmodelsandnonmodels()");
}

TEST_P(SimpleMXnbTest, NbModelsOption) {
	runTests("modelexpansion.idp", GetParam(), "nbmodelsOptionVerification()");
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
