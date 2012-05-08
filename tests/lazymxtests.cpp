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

#include "gtest/gtest.h"
#include "external/rungidl.hpp"
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"

#include <exception>

using namespace std;

namespace Tests {

vector<string> generateListOfMXsatFiles() {
	vector<string> testdirs {"satmx/"};
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}
vector<string> generateListOfMXnbFiles() {
	vector<string> testdirs {"simplemx/", "numberknown/"};
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}

class MXLazySATTest: public ::testing::TestWithParam<string> {
};
class MXLazyMXTest: public ::testing::TestWithParam<string> {
};

TEST_P(MXLazyMXTest, DoesLazyMX) {
	runTests("lazymodelexpansion.idp", GetParam(), "mxlazy()");
}

TEST_P(MXLazySATTest, DoesLazyMX) {
	runTests("lazymodelexpansion.idp", GetParam(), "satlazy()");
}

INSTANTIATE_TEST_CASE_P(ModelExpansion, MXLazySATTest, ::testing::ValuesIn(generateListOfMXsatFiles()));
INSTANTIATE_TEST_CASE_P(ModelExpansion, MXLazyMXTest, ::testing::ValuesIn(generateListOfMXnbFiles()));

}
