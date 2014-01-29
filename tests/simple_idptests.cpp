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
#include <exception>

using namespace std;

namespace Tests {


vector<string> generateListOfSimpleFiles() {
	vector<string> testdirs {"simple/"};
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}

class SimpleIDPTest: public ::testing::TestWithParam<string> {
};


TEST_P(SimpleIDPTest, Run) {
	cerr << "Testing " << GetParam() << "\n";
	ASSERT_EQ(Status::SUCCESS, test({ GetParam()}));
}

INSTANTIATE_TEST_CASE_P(Basic, SimpleIDPTest, ::testing::ValuesIn(generateListOfSimpleFiles()));


}
