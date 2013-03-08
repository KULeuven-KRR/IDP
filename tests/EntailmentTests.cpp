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
#include "external/runidp.hpp"
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"

#include <exception>

using namespace std;

namespace Tests {

class EntailmentTests: public ::testing::TestWithParam<std::string> {
};

vector<string> generateListOfEntailmentFiles() {
	vector<string> testdirs { "entails/" };
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}

TEST_P(EntailmentTests, Entails) {
	runTests("entailment.idp", GetParam(), "checkEntails()");
}
INSTANTIATE_TEST_CASE_P(Entailment, EntailmentTests, ::testing::ValuesIn(generateListOfEntailmentFiles()));

}
